#include "playlistswidget.h"
#include "translator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDateTime>
#include <QComboBox>
#include <QScrollArea>
#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QDirIterator>
#include <QTimer>

static QImage extractCover(const QString &filePath) {
    QImage img;
    if (filePath.endsWith(".mp3", Qt::CaseInsensitive)) {
        TagLib::MPEG::File file(filePath.toUtf8().data());
        if (file.ID3v2Tag()) {
            auto list = file.ID3v2Tag()->frameList("APIC");
            if (!list.isEmpty()) {
                auto *pic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(list.front());
                if (pic && pic->picture().size() > 0) {
                    img.loadFromData((const uchar*)pic->picture().data(), pic->picture().size());
                }
            }
        }
    }
    else if (filePath.endsWith(".flac", Qt::CaseInsensitive)) {
        TagLib::FLAC::File file(filePath.toUtf8().data());
        if (file.pictureList().size() > 0) {
            TagLib::FLAC::Picture *pic = file.pictureList().front();
            if (pic && pic->data().size() > 0) {
                img.loadFromData((const uchar*)pic->data().data(), pic->data().size());
            }
        }
    }
    return img;
}

static QString extractTitle(const QString &filePath) {
    QString title = QFileInfo(filePath).baseName();
    TagLib::FileRef f(filePath.toUtf8().data());
    if (!f.isNull() && f.tag()) {
        if (!f.tag()->title().isEmpty()) {
            title = f.tag()->title().toCString(true);
        }
    }
    return title;
}

static QString extractArtist(const QString &filePath) {
    TagLib::FileRef f(filePath.toUtf8().data());
    if (!f.isNull() && f.tag()) {
        QString artist = f.tag()->artist().toCString(true);
        if (!artist.isEmpty()) return artist;
    }
    return QString();
}

// Translated display name for well-known built-in cluster identifiers. The
// stored cluster.name (used for paths/comparisons) stays as the Russian
// identifier; only what the user sees is translated.
static QString clusterDisplayName(const QString &name) {
    if (name == QString::fromUtf8("по исполнителю")) return ztr("по исполнителю");
    return name;
}

// ----------------------------------------------------------------
//  ClustersPanel
// ----------------------------------------------------------------
ClustersPanel::ClustersPanel(QWidget *parent) : QWidget(parent) {
    setFixedWidth(200);
    setStyleSheet("background-color: #2b2b2b;");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 10, 5, 10);

    m_clusterList = new QListWidget;
    m_clusterList->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { background: transparent; padding: 10px; "
        "border-radius: 6px; margin: 2px 0; }"
        "QListWidget::item:selected { background: rgba(255,255,255,0.15); }"
        "QListWidget::item:hover { background: rgba(255,255,255,0.05); }"
    );
    m_clusterList->setSpacing(2);
    layout->addWidget(m_clusterList, 1);

    connect(m_clusterList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0 && m_clusterList->item(row)) {
            QString name = m_clusterList->item(row)->data(Qt::UserRole).toString();
            emit clusterSelected(name);
        }
    });

    // Refresh cluster labels ("(без кластера)", translated built-in names) live.
    connect(&Translator::instance(), &Translator::languageChanged, this, [this]{ loadClusters(); });
}

void ClustersPanel::loadClusters() {
    m_clusterList->clear();
    if (!m_playlistsWidget) return;

    for (const ClusterInfo &ci : m_playlistsWidget->m_clusters) {
        const QString disp = clusterDisplayName(ci.name);
        QString label = ci.color.isValid()
            ? QString("● %1").arg(disp)
            : disp;
        QListWidgetItem *item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, ci.name);
        if (ci.color.isValid()) {
            item->setForeground(ci.color);
        }
        m_clusterList->addItem(item);
    }

    // Always show "(без кластера)" at the bottom
    QListWidgetItem *noClusterItem = new QListWidgetItem(ztr("(без кластера)"));
    noClusterItem->setData(Qt::UserRole, PlaylistsWidget::UnclusteredFilter);
    noClusterItem->setForeground(QColor(150, 150, 150));
    m_clusterList->addItem(noClusterItem);

    // Select first if nothing selected
    if (m_clusterList->count() > 0 && m_clusterList->currentRow() < 0) {
        m_clusterList->setCurrentRow(0);
    }
}

void ClustersPanel::onToggleVisibility() {
    setVisible(!isVisible());
}

void ClustersPanel::onAddCluster() {
    if (!m_playlistsWidget) return;
    CreateClusterDialog dlg(m_playlistsWidget);
    if (dlg.exec() == QDialog::Accepted) {
        QString name = dlg.clusterName();
        if (name.isEmpty()) return;
        ClusterInfo ci;
        ci.name = name;
        ci.color = dlg.clusterColor();

        // Create cluster folder
        QDir().mkpath(PlaylistsWidget::clusterPath(name));

        // Move selected playlists into cluster
        QStringList playlists = dlg.selectedPlaylists();
        for (const QString &pl : playlists) {
            m_playlistsWidget->addPlaylistToCluster(pl, name);
        }

        m_playlistsWidget->m_clusters.append(ci);
        m_playlistsWidget->saveClusters();
        m_playlistsWidget->loadPlaylists();
        loadClusters();
    }
}

void ClustersPanel::onEditCluster() {
    if (!m_playlistsWidget) return;
    int row = m_clusterList->currentRow();
    if (row < 0) return;
    QString clusterName = m_clusterList->item(row)->data(Qt::UserRole).toString();
    if (clusterName.isEmpty() || clusterName == PlaylistsWidget::UnclusteredFilter) return; // "(без кластера)" has sentinel

    EditClusterDialog dlg(clusterName, m_playlistsWidget);
    if (dlg.exec() == QDialog::Accepted) {
        for (ClusterInfo &ci : m_playlistsWidget->m_clusters) {
            if (ci.name == clusterName) {
                ci.color = dlg.clusterColor();
                break;
            }
        }
        m_playlistsWidget->saveClusters();

        // Handle playlist changes
        QStringList oldPlaylists = m_playlistsWidget->getPlaylistsInCluster(clusterName);
        QStringList newPlaylists = dlg.selectedPlaylists();

        // Remove playlists that were unchecked
        for (const QString &pl : oldPlaylists) {
            if (!newPlaylists.contains(pl)) {
                QString src = PlaylistsWidget::clusterPath(clusterName) + "/" + pl;
                QString dst = PlaylistsWidget::basePath() + "/" + pl;
                QDir().rename(src, dst);
            }
        }
        // Add playlists that were newly checked
        for (const QString &pl : newPlaylists) {
            if (!oldPlaylists.contains(pl)) {
                m_playlistsWidget->addPlaylistToCluster(pl, clusterName);
            }
        }

        m_playlistsWidget->loadPlaylists();
        loadClusters();
    }
}

// ----------------------------------------------------------------
//  PlaylistsWidget
// ----------------------------------------------------------------
PlaylistsWidget::PlaylistsWidget(QWidget *parent) : QWidget(parent) {
    QHBoxLayout *outerLayout = new QHBoxLayout(this);
    outerLayout->setSpacing(0);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *mainArea = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(mainArea);
    layout->setContentsMargins(0, 0, 0, 0);

    // Top bar with main buttons and cluster toggle
    QHBoxLayout *top = new QHBoxLayout;
    QPushButton *addBtn = new QPushButton("+");
    addBtn->setFixedSize(40,40);
    QPushButton *editBtn = ztrButton(m_retrans, "Редактировать");
    editBtn->setFixedSize(100,40);
    QPushButton *delBtn = new QPushButton("Delete");
    delBtn->setFixedSize(70,40);

    QPushButton *clusterToggleBtn = ztrButton(m_retrans, "Кластеры");
    clusterToggleBtn->setFixedSize(90,40);
    clusterToggleBtn->setCursor(Qt::PointingHandCursor);
    clusterToggleBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #aaa; border: 1px solid #555; "
        "border-radius: 6px; font-size: 12px; }"
        "QPushButton:hover { color: white; border-color: #888; }"
    );

    QPushButton *addClusterBtn = ztrButton(m_retrans, "+ кластер");
    addClusterBtn->setFixedSize(80,40);
    addClusterBtn->setCursor(Qt::PointingHandCursor);
    addClusterBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #4CAF50; border: 1px solid #4CAF50; "
        "border-radius: 6px; font-size: 12px; }"
        "QPushButton:hover { background: rgba(76,175,80,0.2); }"
    );

    QPushButton *editClusterBtn = ztrButton(m_retrans, "ред. кластер");
    editClusterBtn->setFixedSize(90,40);
    editClusterBtn->setCursor(Qt::PointingHandCursor);
    editClusterBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #aaa; border: 1px solid #555; "
        "border-radius: 6px; font-size: 12px; }"
        "QPushButton:hover { color: white; border-color: #888; }"
    );

    top->addWidget(addBtn);
    top->addWidget(editBtn);
    top->addWidget(delBtn);
    top->addStretch();
    top->addWidget(clusterToggleBtn);
    top->addWidget(editClusterBtn);
    top->addWidget(addClusterBtn);
    layout->addLayout(top);

    m_tilesFrame = new QFrame;
    m_tilesFrame->setFrameShape(QFrame::NoFrame);
    m_tilesFrame->setStyleSheet("QFrame { border: 2px solid transparent; border-radius: 6px; }");

    m_listWidget = new QListWidget;
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setGridSize(QSize(190, 240));
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setStyleSheet("QListWidget::item { background: transparent; border: none; } QListWidget::item:selected { background: transparent; }");
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);

    QVBoxLayout *frameLayout = new QVBoxLayout(m_tilesFrame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->addWidget(m_listWidget);

    layout->addWidget(m_tilesFrame, 1);

    outerLayout->addWidget(mainArea, 1);

    // Right clusters panel
    m_clustersPanel = new ClustersPanel;
    m_clustersPanel->setPlaylistsWidget(this);
    m_clustersPanel->setVisible(false);
    outerLayout->addWidget(m_clustersPanel);

    connect(addBtn, &QPushButton::clicked, this, &PlaylistsWidget::onAddClicked);
    connect(editBtn, &QPushButton::clicked, this, &PlaylistsWidget::onEditClicked);
    connect(delBtn, &QPushButton::clicked, this, &PlaylistsWidget::onDeleteClicked);
    connect(this, &PlaylistsWidget::playlistSelected, this, &PlaylistsWidget::onPlaylistPlaying);
    connect(clusterToggleBtn, &QPushButton::clicked, this, [this, clusterToggleBtn]() {
        m_clustersPanel->onToggleVisibility();
        clusterToggleBtn->setText(m_clustersPanel->isVisible() ? ztr("Кластеры") : ztr("Кластеры"));
    });
    connect(addClusterBtn, &QPushButton::clicked, m_clustersPanel, &ClustersPanel::onAddCluster);
    connect(editClusterBtn, &QPushButton::clicked, m_clustersPanel, &ClustersPanel::onEditCluster);
    connect(m_clustersPanel, &ClustersPanel::clusterSelected, this, &PlaylistsWidget::filterByCluster);

    loadClusters();
    loadPlaylists();
    m_clustersPanel->loadClusters();

    // First run: setup default clusters
    if (isFirstRun()) {
        setupDefaultClusters();
    }

    connect(&Translator::instance(), &Translator::languageChanged, this, &PlaylistsWidget::retranslateUi);
}

void PlaylistsWidget::retranslateUi() {
    runRetrans(m_retrans);
}

const QString PlaylistsWidget::UnclusteredFilter = QStringLiteral("__unclustered__");

QString PlaylistsWidget::basePath() {
    QString path = QDir::homePath() + "/zmp_playlists";
    if (!QDir(path).exists()) QDir().mkpath(path);
    return path;
}

QStringList PlaylistsWidget::supportedExts() {
    return {".mp3", ".wav", ".flac", ".aac", ".aiff"};
}

QString PlaylistsWidget::clusterPath(const QString &clusterName) {
    return basePath() + "/" + clusterFolderName(clusterName);
}

QString PlaylistsWidget::clusterFolderName(const QString &clusterName) {
    QString safe = clusterName;
    safe.replace(QRegularExpression("[^a-zA-Zа-яА-Я0-9_ \\-]"), "_");
    safe.replace(' ', '_');
    return "cls_" + safe;
}

QStringList PlaylistsWidget::getPlaylistsInCluster(const QString &clusterName) const {
    QStringList result;
    QDir dir(clusterPath(clusterName));
    for (const QString &folder : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir pd(dir.absolutePath() + "/" + folder);
        bool hasAudio = false;
        for (const QString &f : pd.entryList(QDir::Files)) {
            if (supportedExts().contains(QFileInfo(f).suffix().toLower().prepend('.'))) {
                hasAudio = true;
                break;
            }
        }
        if (hasAudio) result.append(folder);
    }
    return result;
}

QStringList PlaylistsWidget::getUnclusteredPlaylists() const {
    QStringList result;
    QDir dir(basePath());
    for (const QString &folder : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (folder == "featured" || folder.startsWith("cls_") || folder == ".backup") continue;
        QDir pd(dir.absolutePath() + "/" + folder);
        bool hasAudio = false;
        for (const QString &f : pd.entryList(QDir::Files)) {
            if (supportedExts().contains(QFileInfo(f).suffix().toLower().prepend('.'))) {
                hasAudio = true;
                break;
            }
        }
        if (hasAudio) result.append(folder);
    }
    return result;
}

QStringList PlaylistsWidget::allPlaylistFolders() const {
    QStringList result;
    QDir dir(basePath());
    for (const QString &folder : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (folder == "featured" || folder.startsWith("cls_") || folder == ".backup") continue;
        result.append(folder);
    }
    // Also include playlists inside clusters
    for (const ClusterInfo &ci : m_clusters) {
        result.append(getPlaylistsInCluster(ci.name));
    }
    return result;
}

void PlaylistsWidget::addPlaylistToCluster(const QString &playlistName, const QString &clusterName) {
    QString src = basePath() + "/" + playlistName;
    QString dst = clusterPath(clusterName) + "/" + playlistName;
    QDir().mkpath(clusterPath(clusterName));
    if (QDir(src).exists() && !QDir(dst).exists()) {
        QDir().rename(src, dst);
    }
}

void PlaylistsWidget::saveClusters() {
    QFile file(basePath() + "/config.json");
    QJsonObject root;
    if (file.open(QIODevice::ReadOnly)) {
        root = QJsonDocument::fromJson(file.readAll()).object();
        file.close();
    }

    QJsonObject clustersObj;
    for (const ClusterInfo &ci : m_clusters) {
        QJsonObject c;
        c["color"] = ci.color.isValid() ? ci.color.name() : "";
        clustersObj[ci.name] = c;
    }
    root["clusters"] = clustersObj;

    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

void PlaylistsWidget::loadClusters() {
    m_clusters.clear();
    QFile file(basePath() + "/config.json");
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonObject clustersObj = root["clusters"].toObject();
    for (auto it = clustersObj.begin(); it != clustersObj.end(); ++it) {
        ClusterInfo ci;
        ci.name = it.key();
        QJsonObject c = it.value().toObject();
        QString colorStr = c["color"].toString();
        if (!colorStr.isEmpty()) ci.color = QColor(colorStr);
        m_clusters.append(ci);
    }
}

bool PlaylistsWidget::isFirstRun() const {
    return m_clusters.isEmpty();
}

void PlaylistsWidget::setupDefaultClusters() {
    // Create "default" cluster
    ClusterInfo defaultCluster;
    defaultCluster.name = "default";
    defaultCluster.color = QColor(0, 255, 100);
    QDir().mkpath(clusterPath("default"));
    m_clusters.append(defaultCluster);

    // Create "по исполнителю" cluster
    ClusterInfo artistCluster;
    artistCluster.name = "по исполнителю";
    artistCluster.color = QColor(200, 0, 255);
    QDir().mkpath(clusterPath("по исполнителю"));
    m_clusters.append(artistCluster);

    saveClusters();
    m_clustersPanel->loadClusters();

    // Move any existing playlists into "default" cluster
    QStringList unclustered = getUnclusteredPlaylists();
    for (const QString &pl : unclustered) {
        addPlaylistToCluster(pl, "default");
    }
    loadPlaylists();
}

void PlaylistsWidget::onPlaylistClear() {
    onPlaylistStopped();
}

void PlaylistsWidget::savePlaylistColors() {
    QJsonObject root;
    QJsonObject colors;
    for (auto it = m_playlistColors.begin(); it != m_playlistColors.end(); ++it) {
        colors[it.key()] = it.value().name();
    }
    root["playlist_colors"] = colors;

    // Merge with existing config
    QFile file(basePath() + "/config.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject existing = doc.object();
        for (auto it = existing.begin(); it != existing.end(); ++it) {
            if (it.key() != "playlist_colors") {
                root[it.key()] = it.value();
            }
        }
        file.close();
    }

    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

void PlaylistsWidget::loadPlaylistColors() {
    m_playlistColors.clear();
    QFile file(basePath() + "/config.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonObject colors = root["playlist_colors"].toObject();
    for (auto it = colors.begin(); it != colors.end(); ++it) {
        m_playlistColors[it.key()] = QColor(it.value().toString());
    }
}

void PlaylistsWidget::loadPlaylists() {
    loadPlaylistColors();
    m_playlists.clear();
    m_listWidget->clear();

    // Determine which playlists to show based on cluster filter
    QStringList foldersToShow;
    if (m_currentClusterFilter == UnclusteredFilter) {
        foldersToShow = getUnclusteredPlaylists();
    } else if (m_currentClusterFilter.isEmpty()) {
        // Show unclustered playlists and all cluster playlists
        for (const ClusterInfo &ci : m_clusters) {
            foldersToShow.append(getPlaylistsInCluster(ci.name));
        }
        foldersToShow.append(getUnclusteredPlaylists());
    } else {
        // Show only playlists from the selected cluster
        foldersToShow = getPlaylistsInCluster(m_currentClusterFilter);
    }

    for (const QString &folder : foldersToShow) {
        QString folderPath;
        // Find where this playlist lives
        if (QDir(basePath() + "/" + folder).exists()) {
            folderPath = basePath() + "/" + folder;
        } else {
            // Check inside clusters
            for (const ClusterInfo &ci : m_clusters) {
                QString test = clusterPath(ci.name) + "/" + folder;
                if (QDir(test).exists()) {
                    folderPath = test;
                    break;
                }
            }
        }
        if (folderPath.isEmpty()) continue;

        PlaylistInfo info;
        info.name = folder;
        QDir pd(folderPath);
        for (const QString &f : pd.entryList(QDir::Files)) {
            if (supportedExts().contains(QFileInfo(f).suffix().toLower().prepend('.'))) {
                QString absPath = pd.absolutePath()+"/"+f;
                info.tracks.append(absPath);
                info.trackTitles.append(extractTitle(absPath));
            }
        }
        if (!info.tracks.isEmpty()) info.cover = extractCover(info.tracks.first());
        m_playlists.append(info);

        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(QSize(190, 240));

        PlaylistTileWidget *tile = new PlaylistTileWidget(info, m_listWidget, false);
        m_listWidget->setItemWidget(item, tile);

        if (m_playlistColors.contains(folder)) {
            tile->setBorderColor(m_playlistColors[folder]);
        } else if (!m_currentClusterFilter.isEmpty()) {
            // Use cluster color as border if no individual color set
            for (const ClusterInfo &ci : m_clusters) {
                if (ci.name == m_currentClusterFilter && ci.color.isValid()) {
                    tile->setBorderColor(ci.color);
                    break;
                }
            }
        }

        connect(tile, &PlaylistTileWidget::doubleClicked, this, &PlaylistsWidget::playlistSelected);
    }

    // Featured section
    PlaylistInfo featuredInfo;
    featuredInfo.name = "featured_music";
    QString featuredPath = QDir::homePath() + "/zmp_playlists/featured";
    QDir featuredDir(featuredPath);
    for (const QString &f : featuredDir.entryList(QDir::Files)) {
        if (supportedExts().contains(QFileInfo(f).suffix().toLower().prepend('.'))) {
            QString absPath = featuredPath + "/" + f;
            featuredInfo.tracks.append(absPath);
            featuredInfo.trackTitles.append(extractTitle(absPath));
        }
    }

    if (!featuredInfo.tracks.isEmpty()) featuredInfo.cover = extractCover(featuredInfo.tracks.first());
    m_playlists.append(featuredInfo);

    QListWidgetItem *item = new QListWidgetItem(m_listWidget);
    item->setSizeHint(QSize(190, 240));

    PlaylistTileWidget *tile = new PlaylistTileWidget(featuredInfo, m_listWidget, true);
    m_listWidget->setItemWidget(item, tile);

    connect(tile, &PlaylistTileWidget::doubleClicked, this, &PlaylistsWidget::playlistSelected);
}

void PlaylistsWidget::filterByCluster(const QString &clusterName) {
    // If selecting "по исполнителю" and it's empty, show artist scan dialog
    if (clusterName == "по исполнителю") {
        QStringList pls = getPlaylistsInCluster("по исполнителю");
        if (pls.isEmpty()) {
            ArtistScanDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                dlg.scanFolders();
            }
        }
    }
    m_currentClusterFilter = clusterName;

    // Update border color
    QColor borderColor;
    if (!clusterName.isEmpty() && clusterName != UnclusteredFilter) {
        for (const ClusterInfo &ci : m_clusters) {
            if (ci.name == clusterName && ci.color.isValid()) {
                borderColor = ci.color;
                break;
            }
        }
    }
    if (borderColor.isValid()) {
        m_tilesFrame->setStyleSheet(QString(
            "QFrame { border: 2px solid %1; border-radius: 6px; }"
        ).arg(borderColor.name()));
    } else {
        m_tilesFrame->setStyleSheet("QFrame { border: 2px solid transparent; border-radius: 6px; }");
    }

    loadPlaylists();
}

void PlaylistsWidget::onAddClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle(ztr("Создать плейлист"));
    dlg.setModal(true);
    dlg.resize(450, 450);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel(ztr("Название:")));
    QLineEdit *nameEdit = new QLineEdit;
    l->addWidget(nameEdit);
    l->addWidget(new QLabel(ztr("Файлы:")));
    QListWidget *fileList = new QListWidget;
    l->addWidget(fileList);
    QHBoxLayout *btnL = new QHBoxLayout;
    QPushButton *addFileBtn = new QPushButton(ztr("Добавить файлы"));
    QPushButton *removeFileBtn = new QPushButton(ztr("Удалить выбранный"));
    QPushButton *importBtn = new QPushButton(ztr("Импорт"));
    btnL->addWidget(addFileBtn);
    btnL->addWidget(removeFileBtn);
    btnL->addWidget(importBtn);
    l->addLayout(btnL);

    // Cluster selection
    l->addWidget(new QLabel(ztr("Добавить в кластер:")));
    QComboBox *clusterCombo = new QComboBox;
    clusterCombo->addItem(ztr("(без кластера)"), QString());
    for (const ClusterInfo &ci : m_clusters) {
        clusterCombo->addItem(ci.name, ci.name);
    }
    // Preselect current filter (but not unclustered sentinel)
    if (!m_currentClusterFilter.isEmpty() && m_currentClusterFilter != UnclusteredFilter) {
        int idx = clusterCombo->findText(m_currentClusterFilter);
        if (idx >= 0) clusterCombo->setCurrentIndex(idx);
    }
    l->addWidget(clusterCombo);

    QHBoxLayout *dialogBtns = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton(ztr("Создать"));
    QPushButton *cancelBtn = new QPushButton(ztr("Отмена"));
    dialogBtns->addWidget(okBtn);
    dialogBtns->addWidget(cancelBtn);
    l->addLayout(dialogBtns);

    connect(addFileBtn, &QPushButton::clicked, [&](){
        QStringList files = QFileDialog::getOpenFileNames(&dlg, ztr("Выберите аудиофайлы"), QDir::homePath(),
            ztr("Аудио (*.mp3 *.wav *.flac *.aac *.aiff)"));
        for (const QString &f : files) fileList->addItem(f);
    });
    connect(removeFileBtn, &QPushButton::clicked, [&](){
        delete fileList->currentItem();
    });
    connect(importBtn, &QPushButton::clicked, [&](){
        QString dir = QFileDialog::getExistingDirectory(&dlg, ztr("Выберите папку с аудиофайлами"));
        if (dir.isEmpty()) return;
        QDir d(dir);
        QStringList files = d.entryList(QStringList() << "*.mp3" << "*.wav" << "*.flac" << "*.aac" << "*.aiff", QDir::Files, QDir::Name);
        for (const QString &f : files) fileList->addItem(d.absoluteFilePath(f));
        QString folderName = QFileInfo(dir).fileName();
        if (nameEdit->text().trimmed().isEmpty()) nameEdit->setText(folderName);
    });
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) { QMessageBox::warning(this, ztr("Ошибка"), ztr("Введите название")); return; }
        name.replace('/', '_');
        QStringList files;
        for (int i=0; i<fileList->count(); ++i) files << fileList->item(i)->text();
        if (files.isEmpty()) { QMessageBox::warning(this, ztr("Ошибка"), ztr("Добавьте файлы")); return; }

        QString targetCluster = clusterCombo->currentData().toString();
        QString playlistDir;
        if (targetCluster.isEmpty()) {
            playlistDir = basePath() + "/" + name;
        } else {
            playlistDir = clusterPath(targetCluster) + "/" + name;
            QDir().mkpath(clusterPath(targetCluster));
        }

        QDir dir;
        if (dir.exists(playlistDir)) { QMessageBox::warning(this, ztr("Ошибка"), ztr("Плейлист уже существует")); return; }
        if (!dir.mkpath(playlistDir)) { QMessageBox::warning(this, ztr("Ошибка"), ztr("Не удалось создать папку")); return; }
        for (const QString &src : files) {
            QFileInfo fi(src);
            if (supportedExts().contains(fi.suffix().toLower().prepend('.'))) {
                QString dst = playlistDir + "/" + fi.fileName();
                QFile::copy(src, dst);
            }
        }
        loadPlaylists();
    }
}

void PlaylistsWidget::onDeleteClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle(ztr("Удалить плейлист"));
    dlg.setModal(true);
    dlg.resize(300,150);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel(ztr("Введите название плейлиста для удаления:")));
    QLineEdit *nameEdit = new QLineEdit;
    l->addWidget(nameEdit);
    QHBoxLayout *b = new QHBoxLayout;
    QPushButton *ok = new QPushButton(ztr("Удалить"));
    QPushButton *cancel = new QPushButton(ztr("Отмена"));
    b->addWidget(ok);
    b->addWidget(cancel);
    l->addLayout(b);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    if (dlg.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) { QMessageBox::warning(this, ztr("Ошибка"), ztr("Введите название")); return; }
        // Try unclustered path first, then cluster paths
        QString path = basePath() + "/" + name;
        QDir dir(path);
        if (!dir.exists()) {
            for (const ClusterInfo &ci : m_clusters) {
                path = clusterPath(ci.name) + "/" + name;
                dir.setPath(path);
                if (dir.exists()) break;
            }
        }
        if (!dir.exists()) { QMessageBox::warning(this, ztr("Ошибка"), ztr("Плейлист не найден")); return; }
        if (dir.removeRecursively()) loadPlaylists();
        else QMessageBox::warning(this, ztr("Ошибка"), ztr("Не удалось удалить"));
    }
}

void PlaylistsWidget::onPlaylistPlaying(const QStringList &tracks) {
    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem *item = m_listWidget->item(i);
        QWidget *widget = m_listWidget->itemWidget(item);
        if (PlaylistTileWidget *tile = qobject_cast<PlaylistTileWidget*>(widget)) {
            tile->setPlaying(false);
        }
    }
    for (int i = 0; i < m_playlists.size(); ++i) {
        if (m_playlists[i].tracks == tracks) {
            QListWidgetItem *item = m_listWidget->item(i);
            QWidget *widget = m_listWidget->itemWidget(item);
            if (PlaylistTileWidget *tile = qobject_cast<PlaylistTileWidget*>(widget)) {
                tile->setPlaying(true);
            }
            break;
        }
    }
}

void PlaylistsWidget::onPlaylistStopped() {
    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem *item = m_listWidget->item(i);
        QWidget *widget = m_listWidget->itemWidget(item);
        if (PlaylistTileWidget *tile = qobject_cast<PlaylistTileWidget*>(widget)) {
            tile->setPlaying(false);
        }
    }
}

void PlaylistsWidget::onEditClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle(ztr("Редактировать плейлист"));
    dlg.setModal(true);
    dlg.resize(400, 300);

    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel(ztr("Выберите плейлист:")));

    QListWidget *playlistList = new QListWidget;
    QStringList allFolders = allPlaylistFolders();
    for (const QString &folder : allFolders) {
        if (folder == "featured") continue;
        playlistList->addItem(folder);
    }
    l->addWidget(playlistList);

    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton(ztr("Открыть"));
    QPushButton *cancelBtn = new QPushButton(ztr("Отмена"));
    btns->addWidget(okBtn);
    btns->addWidget(cancelBtn);
    l->addLayout(btns);

    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString selectedName = playlistList->currentItem()->text();
        if (!selectedName.isEmpty()) {
            PlaylistEditDialog editDlg(selectedName, this);
            connect(&editDlg, &PlaylistEditDialog::playlistColorChanged, this, [this](const QString &name, const QColor &color) {
                m_playlistColors[name] = color;
                savePlaylistColors();
                loadPlaylists();
            });
            connect(&editDlg, &PlaylistEditDialog::destroyed, this, &PlaylistsWidget::loadPlaylists);
            editDlg.exec();
        }
    }
}

// ----------------------------------------------------------------
//  PlaylistEditDialog
// ----------------------------------------------------------------
PlaylistEditDialog::PlaylistEditDialog(const QString &playlistName, QWidget *parent)
    : QDialog(parent), m_playlistName(playlistName), m_borderColor(0, 255, 100)
{
    setWindowTitle(ztr("Редактирование плейлиста: ") + playlistName);
    resize(600, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *colorLayout = new QHBoxLayout;
    colorLayout->addWidget(new QLabel(ztr("Цвет полосы:")));

    m_trackList = new QListWidget;
    m_trackList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(m_trackList);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;

    QPushButton *addBtn = new QPushButton(ztr("Добавить файлы"));
    QPushButton *removeBtn = new QPushButton(ztr("Удалить выбранный"));
    m_applyBtn = new QPushButton(ztr("Применить"));
    m_cancelBtn = new QPushButton(ztr("Отмена"));

    buttonsLayout->addWidget(addBtn);
    buttonsLayout->addWidget(removeBtn);
    buttonsLayout->addWidget(m_applyBtn);
    buttonsLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(buttonsLayout);

    connect(addBtn, &QPushButton::clicked, this, &PlaylistEditDialog::onAddFiles);
    connect(removeBtn, &QPushButton::clicked, this, &PlaylistEditDialog::onRemoveTrack);
    connect(m_applyBtn, &QPushButton::clicked, this, &PlaylistEditDialog::onApply);
    connect(m_cancelBtn, &QPushButton::clicked, this, &PlaylistEditDialog::onCancel);

    loadTracks();
    setupColorButtons();
    setupClusterCombo();
}

void PlaylistEditDialog::loadTracks() {
    m_trackList->clear();
    QString playlistPath;
    if (QDir(PlaylistsWidget::basePath() + "/" + m_playlistName).exists()) {
        playlistPath = PlaylistsWidget::basePath() + "/" + m_playlistName;
    } else {
        // Search in clusters
        PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parent());
        if (pw) {
            for (const ClusterInfo &ci : pw->m_clusters) {
                QString test = PlaylistsWidget::clusterPath(ci.name) + "/" + m_playlistName;
                if (QDir(test).exists()) {
                    playlistPath = test;
                    break;
                }
            }
        }
    }
    if (playlistPath.isEmpty()) return;

    QDir dir(playlistPath);
    for (const QString &f : dir.entryList(QDir::Files)) {
        if (PlaylistsWidget::supportedExts().contains(QFileInfo(f).suffix().toLower().prepend('.'))) {
            m_trackList->addItem(QFileInfo(f).fileName());
        }
    }
    m_tracks.clear();
    for (int i = 0; i < m_trackList->count(); ++i) {
        m_tracks.append(m_trackList->item(i)->text());
    }
}

void PlaylistEditDialog::setupColorButtons() {
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    if (!mainLayout) return;

    QHBoxLayout *colorButtonsLayout = new QHBoxLayout;
    colorButtonsLayout->addWidget(new QLabel(ztr("Встроенные цвета:")));

    const QList<QPair<QString, QColor>> presetColors = {
        {ztr("Зеленый"), QColor(0, 255, 100)},
        {ztr("Фиолетовый"), QColor(128, 0, 128)},
        {ztr("Красный"), QColor(255, 0, 0)},
        {ztr("Синий"), QColor(0, 100, 255)},
        {ztr("Оранжевый"), QColor(255, 165, 0)},
        {ztr("Желтый"), QColor(255, 255, 0)}
    };

    for (const auto &preset : presetColors) {
        QPushButton *btn = new QPushButton(preset.first);
        btn->setStyleSheet(QString("background-color: %1;").arg(preset.second.name()));
        connect(btn, &QPushButton::clicked, [this, preset]() {
            m_borderColor = preset.second;
            onColorSelected(preset.second);
        });
        colorButtonsLayout->addWidget(btn);
    }

    colorButtonsLayout->addStretch();

    QHBoxLayout *rgbLayout = new QHBoxLayout;
    rgbLayout->addWidget(new QLabel("RGB:"));

    QLineEdit *rEdit = new QLineEdit;
    rEdit->setPlaceholderText("R");
    rEdit->setMaximumWidth(50);

    QLineEdit *gEdit = new QLineEdit;
    gEdit->setPlaceholderText("G");
    gEdit->setMaximumWidth(50);

    QLineEdit *bEdit = new QLineEdit;
    bEdit->setPlaceholderText("B");
    bEdit->setMaximumWidth(50);

    QPushButton *rgbBtn = new QPushButton(ztr("Выбрать"));
    rgbBtn->setMaximumWidth(80);

    rgbLayout->addWidget(rEdit);
    rgbLayout->addWidget(gEdit);
    rgbLayout->addWidget(bEdit);
    rgbLayout->addWidget(rgbBtn);

    connect(rgbBtn, &QPushButton::clicked, [this, rEdit, gEdit, bEdit]() {
        bool ok;
        int r = rEdit->text().toInt(&ok);
        if (!ok) return;
        int g = gEdit->text().toInt(&ok);
        if (!ok) return;
        int b = bEdit->text().toInt(&ok);
        if (!ok) return;
        m_borderColor = QColor(r, g, b);
        onColorSelected(m_borderColor);
    });

    mainLayout->addLayout(colorButtonsLayout);
    mainLayout->addLayout(rgbLayout);
}

void PlaylistEditDialog::setupClusterCombo() {
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    if (!mainLayout) return;

    QHBoxLayout *clusterLayout = new QHBoxLayout;
    clusterLayout->addWidget(new QLabel(ztr("Кластер:")));

    m_clusterCombo = new QComboBox;
    m_clusterCombo->addItem(ztr("(без кластера)"), QString());

    PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parent());
    if (pw) {
        // Find which cluster this playlist belongs to
        QString currentCluster;
        for (const ClusterInfo &ci : pw->m_clusters) {
            m_clusterCombo->addItem(ci.name, ci.name);
            QStringList pls = pw->getPlaylistsInCluster(ci.name);
            if (pls.contains(m_playlistName)) {
                currentCluster = ci.name;
            }
        }
        if (!currentCluster.isEmpty()) {
            int idx = m_clusterCombo->findText(currentCluster);
            if (idx >= 0) m_clusterCombo->setCurrentIndex(idx);
        }
    }
    clusterLayout->addWidget(m_clusterCombo, 1);
    mainLayout->addLayout(clusterLayout);
}

void PlaylistEditDialog::onColorSelected(const QColor &color) {
    Q_UNUSED(color);
}

void PlaylistEditDialog::onAddFiles() {
    QStringList files = QFileDialog::getOpenFileNames(this, ztr("Выберите аудиофайлы"),
        QDir::homePath(), ztr("Аудио (*.mp3 *.wav *.flac *.aac *.aiff)"));

    if (!files.isEmpty()) {
        // Find playlist path (could be in cluster or root)
        QString playlistPath;
        if (QDir(PlaylistsWidget::basePath() + "/" + m_playlistName).exists()) {
            playlistPath = PlaylistsWidget::basePath() + "/" + m_playlistName;
        } else {
            PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parent());
            if (pw) {
                for (const ClusterInfo &ci : pw->m_clusters) {
                    QString test = PlaylistsWidget::clusterPath(ci.name) + "/" + m_playlistName;
                    if (QDir(test).exists()) {
                        playlistPath = test;
                        break;
                    }
                }
            }
        }

        QDir dir(playlistPath);
        for (const QString &file : files) {
            QFileInfo fi(file);
            if (PlaylistsWidget::supportedExts().contains(fi.suffix().toLower().prepend('.'))) {
                QString destPath = playlistPath + "/" + fi.fileName();
                if (!QFile::exists(destPath)) {
                    QFile::copy(file, destPath);
                }
            }
        }
        loadTracks();
    }
}

void PlaylistEditDialog::onRemoveTrack() {
    int row = m_trackList->currentRow();
    if (row >= 0) {
        QString trackName = m_trackList->item(row)->text();
        // Find playlist path
        QString playlistPath;
        if (QDir(PlaylistsWidget::basePath() + "/" + m_playlistName).exists()) {
            playlistPath = PlaylistsWidget::basePath() + "/" + m_playlistName;
        } else {
            PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parent());
            if (pw) {
                for (const ClusterInfo &ci : pw->m_clusters) {
                    QString test = PlaylistsWidget::clusterPath(ci.name) + "/" + m_playlistName;
                    if (QDir(test).exists()) {
                        playlistPath = test;
                        break;
                    }
                }
            }
        }
        QString filePath = playlistPath + "/" + trackName;
        QFile::remove(filePath);
        loadTracks();
    }
}

void PlaylistEditDialog::onApply() {
    saveChanges();

    // Handle cluster change if combo exists
    if (m_clusterCombo) {
        PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parent());
        if (pw) {
            QString newCluster = m_clusterCombo->currentData().toString();
            // Find current cluster
            QString currentCluster;
            for (const ClusterInfo &ci : pw->m_clusters) {
                QStringList pls = pw->getPlaylistsInCluster(ci.name);
                if (pls.contains(m_playlistName)) {
                    currentCluster = ci.name;
                    break;
                }
            }
            if (currentCluster != newCluster) {
                // Move playlist between clusters
                QString src;
                if (currentCluster.isEmpty()) {
                    src = PlaylistsWidget::basePath() + "/" + m_playlistName;
                } else {
                    src = PlaylistsWidget::clusterPath(currentCluster) + "/" + m_playlistName;
                }
                if (newCluster.isEmpty()) {
                    QString dst = PlaylistsWidget::basePath() + "/" + m_playlistName;
                    if (QDir(src).exists() && src != dst) {
                        QDir().rename(src, dst);
                    }
                } else {
                    QString dst = PlaylistsWidget::clusterPath(newCluster) + "/" + m_playlistName;
                    QDir().mkpath(PlaylistsWidget::clusterPath(newCluster));
                    if (QDir(src).exists() && src != dst) {
                        QDir().rename(src, dst);
                    }
                }
            }
        }
    }

    emit playlistColorChanged(m_playlistName, m_borderColor);
    accept();
}

void PlaylistEditDialog::onCancel() {
    reject();
}

void PlaylistEditDialog::saveChanges() {
    // Find playlist path
    QString playlistPath;
    if (QDir(PlaylistsWidget::basePath() + "/" + m_playlistName).exists()) {
        playlistPath = PlaylistsWidget::basePath() + "/" + m_playlistName;
    } else {
        PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parent());
        if (pw) {
            for (const ClusterInfo &ci : pw->m_clusters) {
                QString test = PlaylistsWidget::clusterPath(ci.name) + "/" + m_playlistName;
                if (QDir(test).exists()) {
                    playlistPath = test;
                    break;
                }
            }
        }
    }
    if (playlistPath.isEmpty()) return;

    QDir dir(playlistPath);

    QDir backupDir(PlaylistsWidget::basePath() + "/.backup");
    if (!backupDir.exists()) {
        backupDir.mkpath(".");
    }

    QString backupPath = backupDir.path() + "/" + m_playlistName + "_" +
                         QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    if (dir.exists()) {
        QDir().rename(playlistPath, backupPath);
    }

    dir.mkpath(playlistPath);

    for (const QString &trackName : m_tracks) {
        QString backupTrack = backupPath + "/" + trackName;
        QString destTrack = playlistPath + "/" + trackName;
        if (QFile::exists(backupTrack)) {
            QFile::rename(backupTrack, destTrack);
        }
    }
}

// ----------------------------------------------------------------
//  CreateClusterDialog
// ----------------------------------------------------------------
CreateClusterDialog::CreateClusterDialog(QWidget *parent)
    : QDialog(parent), m_color(0, 255, 100)
{
    setWindowTitle(ztr("Создать кластер"));
    resize(450, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(new QLabel(ztr("Название кластера:")));
    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText(ztr("Введите название..."));
    mainLayout->addWidget(m_nameEdit);

    mainLayout->addWidget(new QLabel(ztr("Цвет:")));
    QHBoxLayout *colorButtonsLayout = new QHBoxLayout;
    const QList<QPair<QString, QColor>> presetColors = {
        {ztr("Зеленый"), QColor(0, 255, 100)},
        {ztr("Фиолетовый"), QColor(128, 0, 128)},
        {ztr("Красный"), QColor(255, 0, 0)},
        {ztr("Синий"), QColor(0, 100, 255)},
        {ztr("Оранжевый"), QColor(255, 165, 0)},
        {ztr("Желтый"), QColor(255, 255, 0)}
    };
    for (const auto &preset : presetColors) {
        QPushButton *btn = new QPushButton(preset.first);
        btn->setStyleSheet(QString("background-color: %1; border-radius: 4px; padding: 6px;").arg(preset.second.name()));
        connect(btn, &QPushButton::clicked, [this, preset]() {
            m_color = preset.second;
            onColorSelected(preset.second);
        });
        colorButtonsLayout->addWidget(btn);
    }
    mainLayout->addLayout(colorButtonsLayout);

    // RGB input
    QHBoxLayout *rgbLayout = new QHBoxLayout;
    rgbLayout->addWidget(new QLabel("RGB:"));
    QLineEdit *rEdit = new QLineEdit;
    rEdit->setPlaceholderText("R"); rEdit->setMaximumWidth(50);
    QLineEdit *gEdit = new QLineEdit;
    gEdit->setPlaceholderText("G"); gEdit->setMaximumWidth(50);
    QLineEdit *bEdit = new QLineEdit;
    bEdit->setPlaceholderText("B"); bEdit->setMaximumWidth(50);
    QPushButton *rgbBtn = new QPushButton(ztr("Выбрать"));
    rgbBtn->setMaximumWidth(80);
    rgbLayout->addWidget(rEdit);
    rgbLayout->addWidget(gEdit);
    rgbLayout->addWidget(bEdit);
    rgbLayout->addWidget(rgbBtn);
    mainLayout->addLayout(rgbLayout);
    connect(rgbBtn, &QPushButton::clicked, [this, rEdit, gEdit, bEdit]() {
        bool ok;
        int r = rEdit->text().toInt(&ok); if (!ok) return;
        int g = gEdit->text().toInt(&ok); if (!ok) return;
        int b = bEdit->text().toInt(&ok); if (!ok) return;
        m_color = QColor(r, g, b);
        onColorSelected(m_color);
    });

    mainLayout->addWidget(new QLabel(ztr("Добавить плейлисты:")));
    m_playlistCheckList = new QListWidget;
    PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parent);
    if (pw) {
        QStringList unclustered = pw->getUnclusteredPlaylists();
        for (const QString &pl : unclustered) {
            QListWidgetItem *item = new QListWidgetItem(pl);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            m_playlistCheckList->addItem(item);
        }
    }
    mainLayout->addWidget(m_playlistCheckList);

    QPushButton *addPlaylistBtn = new QPushButton(ztr("+ добавить плейлист"));
    addPlaylistBtn->setStyleSheet("QPushButton { background: transparent; color: #4CAF50; border: 1px solid #4CAF50; border-radius: 4px; padding: 6px; } QPushButton:hover { background: rgba(76,175,80,0.2); }");
    mainLayout->addWidget(addPlaylistBtn);
    connect(addPlaylistBtn, &QPushButton::clicked, this, &CreateClusterDialog::onAddPlaylist);

    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *createBtn = new QPushButton(ztr("Создать"));
    QPushButton *cancelBtn = new QPushButton(ztr("Отмена"));
    btns->addWidget(createBtn);
    btns->addWidget(cancelBtn);
    mainLayout->addLayout(btns);

    connect(createBtn, &QPushButton::clicked, this, &CreateClusterDialog::onCreate);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void CreateClusterDialog::onColorSelected(const QColor &color) {
    Q_UNUSED(color);
}

void CreateClusterDialog::onCreate() {
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, ztr("Ошибка"), ztr("Введите название кластера"));
        return;
    }
    accept();
}

QString CreateClusterDialog::clusterName() const {
    return m_nameEdit->text().trimmed();
}

QColor CreateClusterDialog::clusterColor() const {
    return m_color;
}

QStringList CreateClusterDialog::selectedPlaylists() const {
    QStringList result;
    for (int i = 0; i < m_playlistCheckList->count(); ++i) {
        if (m_playlistCheckList->item(i)->checkState() == Qt::Checked) {
            result.append(m_playlistCheckList->item(i)->text());
        }
    }
    return result;
}

void CreateClusterDialog::onAddPlaylist() {
    PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parent());
    if (!pw) return;

    // Get existing names in the checklist
    QStringList existing;
    for (int i = 0; i < m_playlistCheckList->count(); ++i)
        existing.append(m_playlistCheckList->item(i)->text());

    // Get all available playlists not in checklist
    QStringList all = pw->allPlaylistFolders();
    QStringList available;
    for (const QString &pl : all) {
        if (!existing.contains(pl))
            available.append(pl);
    }

    if (available.isEmpty()) {
        QMessageBox::information(this, ztr("Информация"), ztr("Нет доступных плейлистов для добавления"));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(ztr("Добавить плейлист"));
    dlg.resize(350, 300);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel(ztr("Выберите плейлист:")));
    QListWidget *list = new QListWidget;
    for (const QString &pl : available) list->addItem(pl);
    l->addWidget(list);
    QHBoxLayout *b = new QHBoxLayout;
    QPushButton *ok = new QPushButton(ztr("Добавить"));
    QPushButton *cancel = new QPushButton(ztr("Отмена"));
    b->addWidget(ok);
    b->addWidget(cancel);
    l->addLayout(b);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted && list->currentItem()) {
        QString name = list->currentItem()->text();
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        m_playlistCheckList->addItem(item);
    }
}

// ----------------------------------------------------------------
//  EditClusterDialog
// ----------------------------------------------------------------
EditClusterDialog::EditClusterDialog(const QString &clusterName, QWidget *parentWidget)
    : QDialog(parentWidget), m_clusterName(clusterName), m_color(0, 255, 100)
{
    setWindowTitle(ztr("Редактировать кластер: ") + clusterName);
    resize(450, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(new QLabel(ztr("Цвет кластера:")));
    QHBoxLayout *colorButtonsLayout = new QHBoxLayout;
    const QList<QPair<QString, QColor>> presetColors = {
        {ztr("Зеленый"), QColor(0, 255, 100)},
        {ztr("Фиолетовый"), QColor(128, 0, 128)},
        {ztr("Красный"), QColor(255, 0, 0)},
        {ztr("Синий"), QColor(0, 100, 255)},
        {ztr("Оранжевый"), QColor(255, 165, 0)},
        {ztr("Желтый"), QColor(255, 255, 0)}
    };
    for (const auto &preset : presetColors) {
        QPushButton *btn = new QPushButton(preset.first);
        btn->setStyleSheet(QString("background-color: %1; border-radius: 4px; padding: 6px;").arg(preset.second.name()));
        connect(btn, &QPushButton::clicked, [this, preset]() {
            m_color = preset.second;
            onColorSelected(preset.second);
        });
        colorButtonsLayout->addWidget(btn);
    }
    mainLayout->addLayout(colorButtonsLayout);

    // RGB input
    QHBoxLayout *rgbLayout = new QHBoxLayout;
    rgbLayout->addWidget(new QLabel("RGB:"));
    QLineEdit *rEdit = new QLineEdit;
    rEdit->setPlaceholderText("R"); rEdit->setMaximumWidth(50);
    QLineEdit *gEdit = new QLineEdit;
    gEdit->setPlaceholderText("G"); gEdit->setMaximumWidth(50);
    QLineEdit *bEdit = new QLineEdit;
    bEdit->setPlaceholderText("B"); bEdit->setMaximumWidth(50);
    QPushButton *rgbBtn = new QPushButton(ztr("Выбрать"));
    rgbBtn->setMaximumWidth(80);
    rgbLayout->addWidget(rEdit);
    rgbLayout->addWidget(gEdit);
    rgbLayout->addWidget(bEdit);
    rgbLayout->addWidget(rgbBtn);
    mainLayout->addLayout(rgbLayout);
    connect(rgbBtn, &QPushButton::clicked, [this, rEdit, gEdit, bEdit]() {
        bool ok;
        int r = rEdit->text().toInt(&ok); if (!ok) return;
        int g = gEdit->text().toInt(&ok); if (!ok) return;
        int b = bEdit->text().toInt(&ok); if (!ok) return;
        m_color = QColor(r, g, b);
        onColorSelected(m_color);
    });

    mainLayout->addWidget(new QLabel(ztr("Плейлисты в кластере:")));
    m_playlistCheckList = new QListWidget;
    PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parentWidget);
    if (pw) {
        // Load existing cluster color from config
        for (const ClusterInfo &ci : pw->m_clusters) {
            if (ci.name == clusterName) {
                m_color = ci.color;
                break;
            }
        }

        // Show all playlists, check those in this cluster
        QStringList inCluster = pw->getPlaylistsInCluster(clusterName);
        QStringList unclustered = pw->getUnclusteredPlaylists();

        for (const QString &pl : inCluster) {
            QListWidgetItem *item = new QListWidgetItem(pl);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
            m_playlistCheckList->addItem(item);
        }
        for (const QString &pl : unclustered) {
            QListWidgetItem *item = new QListWidgetItem(pl);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            m_playlistCheckList->addItem(item);
        }
    }
    mainLayout->addWidget(m_playlistCheckList);

    QPushButton *addPlaylistBtn = new QPushButton(ztr("+ добавить плейлист"));
    addPlaylistBtn->setStyleSheet("QPushButton { background: transparent; color: #4CAF50; border: 1px solid #4CAF50; border-radius: 4px; padding: 6px; } QPushButton:hover { background: rgba(76,175,80,0.2); }");
    mainLayout->addWidget(addPlaylistBtn);
    connect(addPlaylistBtn, &QPushButton::clicked, this, &EditClusterDialog::onAddPlaylist);

    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *saveBtn = new QPushButton(ztr("Сохранить"));
    QPushButton *cancelBtn = new QPushButton(ztr("Отмена"));
    btns->addWidget(saveBtn);
    btns->addWidget(cancelBtn);
    mainLayout->addLayout(btns);

    connect(saveBtn, &QPushButton::clicked, this, &EditClusterDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void EditClusterDialog::onColorSelected(const QColor &color) {
    Q_UNUSED(color);
}

void EditClusterDialog::onSave() {
    accept();
}

QColor EditClusterDialog::clusterColor() const {
    return m_color;
}

QStringList EditClusterDialog::selectedPlaylists() const {
    QStringList result;
    for (int i = 0; i < m_playlistCheckList->count(); ++i) {
        if (m_playlistCheckList->item(i)->checkState() == Qt::Checked) {
            result.append(m_playlistCheckList->item(i)->text());
        }
    }
    return result;
}

void EditClusterDialog::onAddPlaylist() {
    PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parentWidget());
    if (!pw) return;

    QStringList existing;
    for (int i = 0; i < m_playlistCheckList->count(); ++i)
        existing.append(m_playlistCheckList->item(i)->text());

    QStringList all = pw->allPlaylistFolders();
    QStringList available;
    for (const QString &pl : all) {
        if (!existing.contains(pl))
            available.append(pl);
    }

    if (available.isEmpty()) {
        QMessageBox::information(this, ztr("Информация"), ztr("Нет доступных плейлистов для добавления"));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(ztr("Добавить плейлист"));
    dlg.resize(350, 300);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel(ztr("Выберите плейлист:")));
    QListWidget *list = new QListWidget;
    for (const QString &pl : available) list->addItem(pl);
    l->addWidget(list);
    QHBoxLayout *b = new QHBoxLayout;
    QPushButton *ok = new QPushButton(ztr("Добавить"));
    QPushButton *cancel = new QPushButton(ztr("Отмена"));
    b->addWidget(ok);
    b->addWidget(cancel);
    l->addLayout(b);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted && list->currentItem()) {
        QString name = list->currentItem()->text();
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        m_playlistCheckList->addItem(item);
    }
}

// ----------------------------------------------------------------
//  ArtistScanDialog
// ----------------------------------------------------------------
ArtistScanDialog::ArtistScanDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(ztr("Настройка кластера \"по исполнителю\""));
    resize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *desc = new QLabel(ztr("Выберите папки для сканирования аудиофайлов.\nБудут найдены все mp3, flac, wav, aac, aiff файлы,\nпрочитаны их теги исполнителей и созданы плейлисты\nв кластере \"по исполнителю\"."));
    desc->setWordWrap(true);
    mainLayout->addWidget(desc);

    m_folderList = new QListWidget;
    mainLayout->addWidget(m_folderList, 1);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *addFolderBtn = new QPushButton(ztr("Добавить папку"));
    m_startBtn = new QPushButton(ztr("Начать сканирование"));
    QPushButton *skipBtn = new QPushButton(ztr("Пропустить"));
    btnLayout->addWidget(addFolderBtn);
    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(skipBtn);
    mainLayout->addLayout(btnLayout);

    m_statusLabel = new QLabel("");
    mainLayout->addWidget(m_statusLabel);

    connect(addFolderBtn, &QPushButton::clicked, this, &ArtistScanDialog::onAddFolder);
    connect(m_startBtn, &QPushButton::clicked, this, &ArtistScanDialog::onStartScan);
    connect(skipBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ArtistScanDialog::onAddFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, ztr("Выберите папку с аудиофайлами"));
    if (!dir.isEmpty()) {
        m_folderList->addItem(dir);
    }
}

void ArtistScanDialog::onStartScan() {
    if (m_folderList->count() == 0) {
        QMessageBox::warning(this, ztr("Ошибка"), ztr("Добавьте хотя бы одну папку для сканирования"));
        return;
    }
    accept();
}

void ArtistScanDialog::scanFolders() {
    PlaylistsWidget *pw = qobject_cast<PlaylistsWidget*>(parent());
    if (!pw) return;

    m_statusLabel->setText(ztr("Сканирование..."));
    QApplication::processEvents();

    QMap<QString, QStringList> artistTracks; // artist -> list of file paths
    int totalFiles = 0;

    for (int i = 0; i < m_folderList->count(); ++i) {
        QString folderPath = m_folderList->item(i)->text();
        QDirIterator it(folderPath, QStringList() << "*.mp3" << "*.flac" << "*.wav" << "*.aac" << "*.aiff",
                        QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QString artist = extractArtist(filePath);
            if (artist.isEmpty()) artist = "нет_данных";
            artistTracks[artist].append(filePath);
            totalFiles++;
        }
    }

    // Create playlists in the "по исполнителю" cluster
    QString clusterDir = PlaylistsWidget::clusterPath("по исполнителю");
    QDir().mkpath(clusterDir);

    int playlistCount = 0;
    for (auto it = artistTracks.begin(); it != artistTracks.end(); ++it) {
        QString artistName = it.key();
        QString playlistDir = clusterDir + "/" + artistName;
        QDir().mkpath(playlistDir);

        for (const QString &srcPath : it.value()) {
            QFileInfo fi(srcPath);
            // Avoid overwriting
            QString destPath = playlistDir + "/" + fi.fileName();
            if (!QFile::exists(destPath)) {
                QFile::copy(srcPath, destPath);
            }
        }
        playlistCount++;
    }

    m_statusLabel->setText(QString(ztr("Готово: найдено %1 файлов, создано %2 плейлистов"))
                          .arg(totalFiles).arg(playlistCount));
    QApplication::processEvents();
    QTimer::singleShot(1500, this, &QDialog::accept);
}
