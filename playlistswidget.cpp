#include "playlistswidget.h"
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
#include <QSettings>
#include <QDateTime>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <QRandomGenerator>

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

PlaylistEditDialog::PlaylistEditDialog(const QString &playlistName, QWidget *parent)
    : QDialog(parent), m_playlistName(playlistName), m_borderColor(0, 255, 100)
{
    setWindowTitle("Редактирование плейлиста: " + playlistName);
    resize(600, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *colorLayout = new QHBoxLayout;
    colorLayout->addWidget(new QLabel("Цвет полосы:"));

    m_trackList = new QListWidget;
    m_trackList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(m_trackList);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;

    QPushButton *addBtn = new QPushButton("Добавить файлы");
    QPushButton *removeBtn = new QPushButton("Удалить выбранный");
    m_applyBtn = new QPushButton("Применить");
    m_cancelBtn = new QPushButton("Отмена");

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
}

void PlaylistEditDialog::loadTracks() {
    m_trackList->clear();
    QString playlistPath = PlaylistsWidget::basePath() + "/" + m_playlistName;
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
    colorButtonsLayout->addWidget(new QLabel("Встроенные цвета:"));

    const QList<QPair<QString, QColor>> presetColors = {
        {"Зеленый", QColor(0, 255, 100)},
        {"Фиолетовый", QColor(128, 0, 128)},
        {"Красный", QColor(255, 0, 0)},
        {"Синий", QColor(0, 100, 255)},
        {"Оранжевый", QColor(255, 165, 0)},
        {"Желтый", QColor(255, 255, 0)}
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

    QPushButton *rgbBtn = new QPushButton("Выбрать");
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

void PlaylistEditDialog::onColorSelected(const QColor &color) {
    Q_UNUSED(color);
}

void PlaylistEditDialog::onAddFiles() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Выберите аудиофайлы",
        QDir::homePath(), "Аудио (*.mp3 *.wav *.flac *.aac *.aiff)");

    if (!files.isEmpty()) {
        QString playlistPath = PlaylistsWidget::basePath() + "/" + m_playlistName;
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
        QString filePath = PlaylistsWidget::basePath() + "/" + m_playlistName + "/" + trackName;
        QFile::remove(filePath);
        loadTracks();
    }
}

void PlaylistEditDialog::onApply() {
    saveChanges();
    emit savePlaylistColors();
    accept();
}

void PlaylistEditDialog::onCancel() {
    reject();
}

void PlaylistEditDialog::saveChanges() {
    QString playlistPath = PlaylistsWidget::basePath() + "/" + m_playlistName;
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

PlaylistsWidget::PlaylistsWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *top = new QHBoxLayout;
    QPushButton *addBtn = new QPushButton("+");
    addBtn->setFixedSize(40,40);
    QPushButton *editBtn = new QPushButton("Редактировать");
    editBtn->setFixedSize(100,40);
    QPushButton *delBtn = new QPushButton("Delete");
    delBtn->setFixedSize(70,40);
    top->addWidget(addBtn);
    top->addWidget(editBtn);
    top->addWidget(delBtn);
    top->addStretch();
    layout->addLayout(top);

    m_listWidget = new QListWidget;
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setGridSize(QSize(190, 240));
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setStyleSheet("QListWidget::item { background: transparent; border: none; } QListWidget::item:selected { background: transparent; }");
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(m_listWidget);

    connect(addBtn, &QPushButton::clicked, this, &PlaylistsWidget::onAddClicked);
    connect(editBtn, &QPushButton::clicked, this, &PlaylistsWidget::onEditClicked);
    connect(delBtn, &QPushButton::clicked, this, &PlaylistsWidget::onDeleteClicked);
    connect(this, &PlaylistsWidget::playlistSelected, this, &PlaylistsWidget::onPlaylistPlaying);

    loadPlaylists();
}

QString PlaylistsWidget::basePath() {
    QString path = QDir::homePath() + "/zmp_playlists";
    if (!QDir(path).exists()) QDir().mkpath(path);
    return path;
}

QStringList PlaylistsWidget::supportedExts() {
    return {".mp3", ".wav", ".flac", ".aac", ".aiff"};
}

void PlaylistsWidget::onPlaylistClear() {
    onPlaylistStopped();
}

void PlaylistsWidget::savePlaylistColors() {
    QSettings settings("MyPlayer", "PlaylistColors");
    for (auto it = m_playlistColors.begin(); it != m_playlistColors.end(); ++it) {
        settings.setValue(it.key(), it.value());
    }
}

void PlaylistsWidget::loadPlaylistColors() {
    QSettings settings("MyPlayer", "PlaylistColors");
    m_playlistColors.clear();
    for (const QString &key : settings.childKeys()) {
        m_playlistColors[key] = settings.value(key).value<QColor>();
    }
}

void PlaylistsWidget::loadPlaylists() {
    loadPlaylistColors();
    m_playlists.clear();
    m_listWidget->clear();
    QDir dir(basePath());
    for (const QString &folder : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (folder == "featured") continue;

        PlaylistInfo info;
        info.name = folder;
        QDir pd(basePath()+"/"+folder);
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
        }

        connect(tile, &PlaylistTileWidget::doubleClicked, this, &PlaylistsWidget::playlistSelected);
    }

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

void PlaylistsWidget::onAddClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle("Создать плейлист");
    dlg.setModal(true);
    dlg.resize(450, 400);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel("Название (без пробелов, спецсимволов, кроме _ - ( )):"));
    QLineEdit *nameEdit = new QLineEdit;
    l->addWidget(nameEdit);
    l->addWidget(new QLabel("Файлы:"));
    QListWidget *fileList = new QListWidget;
    l->addWidget(fileList);
    QHBoxLayout *btnL = new QHBoxLayout;
    QPushButton *addFileBtn = new QPushButton("Добавить файлы");
    QPushButton *removeFileBtn = new QPushButton("Удалить выбранный");
    btnL->addWidget(addFileBtn);
    btnL->addWidget(removeFileBtn);
    l->addLayout(btnL);
    QHBoxLayout *dialogBtns = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton("Создать");
    QPushButton *cancelBtn = new QPushButton("Отмена");
    dialogBtns->addWidget(okBtn);
    dialogBtns->addWidget(cancelBtn);
    l->addLayout(dialogBtns);

    connect(addFileBtn, &QPushButton::clicked, [&](){
        QStringList files = QFileDialog::getOpenFileNames(&dlg, "Выберите аудиофайлы", QDir::homePath(),
            "Аудио (*.mp3 *.wav *.flac *.aac *.aiff)");
        for (const QString &f : files) fileList->addItem(f);
    });
    connect(removeFileBtn, &QPushButton::clicked, [&](){
        delete fileList->currentItem();
    });
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Введите название"); return; }
        QStringList files;
        for (int i=0; i<fileList->count(); ++i) files << fileList->item(i)->text();
        if (files.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Добавьте файлы"); return; }
        QString playlistDir = basePath() + "/" + name;
        QDir dir;
        if (dir.exists(playlistDir)) { QMessageBox::warning(this, "Ошибка", "Плейлист уже существует"); return; }
        if (!dir.mkpath(playlistDir)) { QMessageBox::warning(this, "Ошибка", "Не удалось создать папку"); return; }
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
    dlg.setWindowTitle("Удалить плейлист");
    dlg.setModal(true);
    dlg.resize(300,150);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel("Введите название плейлиста для удаления:"));
    QLineEdit *nameEdit = new QLineEdit;
    l->addWidget(nameEdit);
    QHBoxLayout *b = new QHBoxLayout;
    QPushButton *ok = new QPushButton("Удалить");
    QPushButton *cancel = new QPushButton("Отмена");
    b->addWidget(ok);
    b->addWidget(cancel);
    l->addLayout(b);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    if (dlg.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Введите название"); return; }
        QString path = basePath() + "/" + name;
        QDir dir(path);
        if (!dir.exists()) { QMessageBox::warning(this, "Ошибка", "Плейлист не найден"); return; }
        if (dir.removeRecursively()) loadPlaylists();
        else QMessageBox::warning(this, "Ошибка", "Не удалось удалить");
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
    dlg.setWindowTitle("Редактировать плейлист");
    dlg.setModal(true);
    dlg.resize(400, 300);

    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel("Выберите плейлист:"));

    QListWidget *playlistList = new QListWidget;
    QDir dir(basePath());
    for (const QString &folder : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (folder == "featured") continue;
        playlistList->addItem(folder);
    }
    l->addWidget(playlistList);

    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton("Открыть");
    QPushButton *cancelBtn = new QPushButton("Отмена");
    btns->addWidget(okBtn);
    btns->addWidget(cancelBtn);
    l->addLayout(btns);

    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString selectedName = playlistList->currentItem()->text();
        if (!selectedName.isEmpty()) {
            PlaylistEditDialog editDlg(selectedName, this);
            connect(&editDlg, &PlaylistEditDialog::destroyed, this, &PlaylistsWidget::loadPlaylists);
            editDlg.exec();
        }
    }
}