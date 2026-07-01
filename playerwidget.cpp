#include "playerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QTime>
#include <QPainter>
#include <QPixmap>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>

class SpectrumWidget : public QWidget
{
public:
    SpectrumWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
    }
    void setLevels(const QVector<float> &levels) { m_levels = levels; update(); }
    void setColor(const QColor &color) { m_color = color; update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setPen(Qt::NoPen);
        int w = width(), h = height();
        int n = m_levels.size();
        if (n == 0) return;
        float bw = (float)w / n;
        for (int i=0; i<n; ++i) {
            float level = qBound(0.0f, m_levels[i], 1.0f);
            int bh = level * h;
            p.fillRect(i*bw + 1, h-bh, bw - 2, bh, m_color);
        }
    }
private:
    QVector<float> m_levels;
    QColor m_color;
};

class RightMetaContainer : public QWidget {
public:
    RightMetaContainer(QWidget *parent = nullptr) : QWidget(parent), m_spectrum(nullptr), m_glowAngle(0) {
        m_glowTimer = new QTimer(this);
        m_glowTimer->setInterval(16);
        connect(m_glowTimer, &QTimer::timeout, this, [this](){
            m_glowAngle = (m_glowAngle + 6) % 3600;
            update();
        });
    }
    void setSpectrum(SpectrumWidget *s) { m_spectrum = s; }

    void startGlow() {
        m_glowAngle = 0;
        m_glowTimer->start();
        QTimer::singleShot(5000, m_glowTimer, &QTimer::stop);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        if (m_spectrum) {
            m_spectrum->setGeometry(0, 0, width(), height());
            m_spectrum->lower();
        }
        QWidget::resizeEvent(event);
    }

    void paintEvent(QPaintEvent*) override {
        if (!m_glowTimer->isActive()) return;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        qreal angle = m_glowAngle / 10.0;
        QColor accent = QColor(42,130,218);

        QPen glowPen;
        glowPen.setWidth(6);
        glowPen.setColor(QColor(accent.red(), accent.green(), accent.blue(), 80));

        QConicalGradient glowGrad(rect().center(), angle);
        glowGrad.setColorAt(0.0, QColor(accent.red(), accent.green(), accent.blue(), 100));
        glowGrad.setColorAt(0.1, QColor(accent.red(), accent.green(), accent.blue(), 0));
        glowGrad.setColorAt(1.0, QColor(accent.red(), accent.green(), accent.blue(), 0));
        glowPen.setBrush(QBrush(glowGrad));

        p.setPen(glowPen);
        p.drawRoundedRect(1, 1, width()-2, height()-2, 5, 5);

        QPen mainPen;
        mainPen.setWidth(2);
        QConicalGradient mainGrad(rect().center(), angle);
        mainGrad.setColorAt(0.0, accent);
        mainGrad.setColorAt(0.05, accent);
        mainGrad.setColorAt(0.15, QColor(accent.red(), accent.green(), accent.blue(), 0));
        mainGrad.setColorAt(1.0, QColor(accent.red(), accent.green(), accent.blue(), 0));
        mainPen.setBrush(QBrush(mainGrad));

        p.setPen(mainPen);
        p.drawRoundedRect(1, 1, width()-2, height()-2, 5, 5);
    }

private:
    SpectrumWidget *m_spectrum;
    QTimer *m_glowTimer;
    int m_glowAngle;
};


PlayerWidget::PlayerWidget(AudioManager *audioManager, QWidget *parent)
    : QWidget(parent), m_audioManager(audioManager), m_currentIndex(-1), m_isSeeking(false),
      m_metadataHeight(220), m_accentColor(42,130,218), m_spectrumWidget(nullptr)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QWidget *metaContainer = new QWidget(this);
    metaContainer->setFixedHeight(m_metadataHeight);
    QHBoxLayout *metaLayout = new QHBoxLayout(metaContainer);
    metaLayout->setContentsMargins(0,0,0,0);

    m_coverLabel = new QLabel;
    m_coverLabel->setFixedSize(180, 180);
    m_coverLabel->setScaledContents(true);
    metaLayout->addWidget(m_coverLabel);

    RightMetaContainer *rightContainer = new RightMetaContainer(this);
    m_metaContainer = rightContainer;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(10,10,10,10);

    m_titleLabel = new QLabel;
    m_titleLabel->setStyleSheet("font-size:14pt; font-weight:bold; background:transparent;");
    m_artistLabel = new QLabel;
    m_artistLabel->setStyleSheet("background:transparent;");
    m_albumYearLabel = new QLabel;
    m_albumYearLabel->setStyleSheet("background:transparent;");
    rightLayout->addWidget(m_titleLabel);
    rightLayout->addWidget(m_artistLabel);
    rightLayout->addWidget(m_albumYearLabel);
    rightLayout->addStretch();

    m_spectrumWidget = new SpectrumWidget(rightContainer);
    m_spectrumWidget->setColor(m_accentColor);
    m_spectrumWidget->lower();

    rightContainer->setSpectrum(m_spectrumWidget);

    rightContainer->setLayout(rightLayout);
    metaLayout->addWidget(rightContainer, 1);
    mainLayout->addWidget(metaContainer);

    m_positionSlider = new QSlider(Qt::Horizontal);
    m_positionSlider->setRange(0, 1000);
    mainLayout->addWidget(m_positionSlider);

    m_timeLabel = new QLabel("00:00 / 00:00");
    mainLayout->addWidget(m_timeLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_prevIcon = new IconButton;
    m_prevIcon->setAlignment(Qt::AlignCenter);
    m_prevIcon->setScaledContents(true);
    btnLayout->addWidget(m_prevIcon);

    m_playIcon = new IconButton;
    m_playIcon->setAlignment(Qt::AlignCenter);
    m_playIcon->setScaledContents(true);
    btnLayout->addWidget(m_playIcon);

    m_nextIcon = new IconButton;
    m_nextIcon->setAlignment(Qt::AlignCenter);
    m_nextIcon->setScaledContents(true);
    btnLayout->addWidget(m_nextIcon);

    m_featuredIcon = new IconButton;
    m_featuredIcon->setAlignment(Qt::AlignCenter);
    m_featuredIcon->setScaledContents(true);
    btnLayout->addWidget(m_featuredIcon);
    mainLayout->addLayout(btnLayout);

    m_playlistWidget = new QListWidget;
    mainLayout->addWidget(new QLabel("Очередь воспроизведения:"));
    mainLayout->addWidget(m_playlistWidget);

    m_pulseTimer = new QTimer(this);
    m_pulseElapsed = 0;
    m_isPulsing = false;
    connect(m_pulseTimer, &QTimer::timeout, this, QOverload<>::of(&PlayerWidget::update));
    connect(m_positionSlider, &QSlider::sliderMoved, this, &PlayerWidget::onSliderMoved);
    connect(m_playlistWidget, &QListWidget::itemDoubleClicked, this, &PlayerWidget::onPlaylistItemDoubleClicked);
    connect(m_audioManager, &AudioManager::positionChanged, this, &PlayerWidget::onPositionChanged);
    connect(m_audioManager, &AudioManager::durationChanged, this, &PlayerWidget::onDurationChanged);
    connect(m_audioManager, &AudioManager::stateChanged, this, &PlayerWidget::onStateChanged);
    connect(m_audioManager, &AudioManager::spectrumDataChanged, this, &PlayerWidget::updateSpectrum);
    connect(m_playIcon, &IconButton::clicked, this, &PlayerWidget::onPlayClicked);
    connect(m_nextIcon, &IconButton::clicked, this, &PlayerWidget::onNextClicked);
    connect(m_prevIcon, &IconButton::clicked, this, &PlayerWidget::onPrevClicked);
    connect(m_featuredIcon, &IconButton::clicked, this, &PlayerWidget::onFeaturedClicked);

    m_iconSize = 40;
    loadIcons();
    setIconSize(40);
}

void PlayerWidget::loadIcons() {
    const QString iconPath = ":/icons/";
    
    if (QPixmap(iconPath + "play.svg").isNull()) {
        m_playIcon->setText("▶");
        m_playIcon->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(text);");
    } else {
        m_playIcon->setPixmap(QPixmap(iconPath + "play.svg"));
    }
    
    if (QPixmap(iconPath + "next.svg").isNull()) {
        m_nextIcon->setText("▶");
        m_nextIcon->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(text);");
    } else {
        m_nextIcon->setPixmap(QPixmap(iconPath + "next.svg"));
    }
    
    if (QPixmap(iconPath + "prev.svg").isNull()) {
        m_prevIcon->setText("◀");
        m_prevIcon->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(text);");
    } else {
        m_prevIcon->setPixmap(QPixmap(iconPath + "prev.svg"));
    }
    
    updateFeaturedButtonIcon();
    setIconSize(m_iconSize);
}

void PlayerWidget::updatePlayButtonIcon(bool playing) {
    const QString iconPath = ":/icons/";
    
    if (QPixmap(iconPath + (playing ? "stop.svg" : "play.svg")).isNull()) {
        m_playIcon->setText(playing ? "⏹" : "▶");
        m_playIcon->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(text);");
    } else {
        m_playIcon->setPixmap(QPixmap(iconPath + (playing ? "stop.svg" : "play.svg")));
    }
    update();
}

void PlayerWidget::updateNextPrevButtonIcons() {
    const QString iconPath = ":/icons/";
    
    if (QPixmap(iconPath + "next.svg").isNull()) {
        m_nextIcon->setText("▶");
        m_nextIcon->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(text);");
    } else {
        m_nextIcon->setPixmap(QPixmap(iconPath + "next.svg"));
    }
    
    if (QPixmap(iconPath + "prev.svg").isNull()) {
        m_prevIcon->setText("◀");
        m_prevIcon->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(text);");
    } else {
        m_prevIcon->setPixmap(QPixmap(iconPath + "prev.svg"));
    }
    
    updateFeaturedButtonIcon();
}

void PlayerWidget::updateFeaturedButtonIcon() {
    const QString iconPath = ":/icons/";
    
    if (QPixmap(iconPath + (isTrackInFeatured() ? "featured-1.svg" : "featured-0.svg")).isNull()) {
        m_featuredIcon->setText(isTrackInFeatured() ? "★" : "☆");
        m_featuredIcon->setStyleSheet("font-size: 24px; font-weight: bold; color: palette(text);");
    } else {
        m_featuredIcon->setPixmap(QPixmap(iconPath + (isTrackInFeatured() ? "featured-1.svg" : "featured-0.svg")));
    }
    update();
}

bool PlayerWidget::isTrackInFeatured() {
    if (m_playlist.isEmpty() || m_currentIndex < 0) return false;

    QString currentTrack = m_playlist[m_currentIndex];
    QFileInfo currentFi(currentTrack);

    QString featuredPath = QDir::homePath() + "/zmp_playlists/featured";
    if (!QDir(featuredPath).exists()) return false;

    QDir featuredDir(featuredPath);
    QStringList files = featuredDir.entryList(QDir::Files);

    return files.contains(currentFi.fileName());
}

void PlayerWidget::setIconSize(int size) {
    m_iconSize = size;
    int playIconSize = size + 10;
    int iconSize = size;

    m_playIcon->setFixedSize(playIconSize, playIconSize);
    m_playIcon->setAlignment(Qt::AlignCenter);
    m_playIcon->setScaledContents(true);

    m_prevIcon->setFixedSize(iconSize, iconSize);
    m_prevIcon->setAlignment(Qt::AlignCenter);
    m_prevIcon->setScaledContents(true);

    m_nextIcon->setFixedSize(iconSize, iconSize);
    m_nextIcon->setAlignment(Qt::AlignCenter);
    m_nextIcon->setScaledContents(true);

    m_featuredIcon->setFixedSize(iconSize, iconSize);
    m_featuredIcon->setAlignment(Qt::AlignCenter);
    m_featuredIcon->setScaledContents(true);

    updateNextPrevButtonIcons();
    updateFeaturedButtonIcon();

    update();
}

void PlayerWidget::setPlaylist(const QStringList &files) {
    m_playlistWidget->clear();
    for (const QString &f : files) m_playlistWidget->addItem(QFileInfo(f).fileName());
    if (!files.isEmpty()) m_currentIndex = 0;
    setCurrentPlaylist(files);
}

void PlayerWidget::onPlay() {
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) return;
    QString path = m_playlist[m_currentIndex];

    m_isUserStop = false;
    m_isUserPause = false;
    m_isSwitchingTracks = true;

    m_audioManager->setSourceFile(path);
    TrackMetadata meta = extractMetadata(path);
    updateTrackInfo(meta);
    m_audioManager->play();

    m_isSwitchingTracks = false;

    updateUI();
    updatePlayButtonIcon(true);

    if (m_metaContainer) m_metaContainer->startGlow();
    m_pulseElapsed = 0;
    m_isPulsing = true;
    m_pulseTimer->start(16);
    QTimer::singleShot(5000, this, [this](){
        m_isPulsing = false;
        m_pulseTimer->stop();
        update();
    });
}

void PlayerWidget::setMetadataHeight(int h) {
    m_metadataHeight = h;
    QWidget *meta = qobject_cast<QWidget*>(layout()->itemAt(0)->widget());
    if (meta) meta->setFixedHeight(h);
}

void PlayerWidget::updateSpectrum(const QVector<float> &levels) {
    if (m_spectrumWidget) m_spectrumWidget->setLevels(levels);
}

void PlayerWidget::onPositionChanged(qint64 pos) {
    if (!m_isSeeking) {
        qint64 dur = m_audioManager->duration();
        if (dur > 0) m_positionSlider->setValue(static_cast<int>(pos * 1000 / dur));
        QTime t(0,0), cur = t.addMSecs(pos), tot = t.addMSecs(dur);
        m_timeLabel->setText(cur.toString("mm:ss") + " / " + tot.toString("mm:ss"));
    }
}
void PlayerWidget::onDurationChanged(qint64 dur) { onPositionChanged(m_audioManager->position()); }

void PlayerWidget::onStateChanged(bool playing) {
    if (playing) {
        emit stateChanged(true);
        updatePlayButtonIcon(true);
        return;
    }

    if (m_isSwitchingTracks || m_isUserStop || m_isUserPause || m_isUserManuallyStopped) {
        return;
    }

    if (!m_playlist.isEmpty() && m_currentIndex < m_playlist.size() - 1) {
        m_currentIndex++;
        onPlay();
    } else {
        emit stateChanged(false);
        updatePlayButtonIcon(false);
        if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
            m_audioManager->setSavedPosition(m_audioManager->position());
        }
        emit featuredUpdated();
    }
}

void PlayerWidget::setCurrentPlaylist(const QStringList &tracks) {
    m_currentPlaylistTracks = tracks;
    m_playlist = tracks;
    emit currentPlaylistChanged(tracks);
    emit featuredUpdated();
}
void PlayerWidget::onSliderMoved(int value) {
    qint64 dur = m_audioManager->duration();
    if (dur > 0) {
        m_isSeeking = true;
        m_audioManager->setPosition(value * dur / 1000);
        m_isSeeking = false;
    }
}
void PlayerWidget::onPlaylistItemDoubleClicked(QListWidgetItem *item) {
    int row = m_playlistWidget->row(item);
    if (row >= 0 && row < m_playlist.size()) {
        m_currentIndex = row;
        m_audioManager->setSavedPosition(0);
        setCurrentPlaylist(m_playlist);
        onPlay();
    }
}

void PlayerWidget::updateUI() {
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        TrackMetadata meta = extractMetadata(m_playlist[m_currentIndex]);
        updateTrackInfo(meta);
    }
}

TrackMetadata PlayerWidget::extractMetadata(const QString &filePath) {
    TrackMetadata data;
    QFileInfo fi(filePath);
    data.title = fi.baseName();
    TagLib::FileRef f(filePath.toUtf8().data());
    if (!f.isNull() && f.tag()) {
        TagLib::Tag *tag = f.tag();
        if (!tag->title().isEmpty()) data.title = tag->title().toCString(true);
        if (!tag->artist().isEmpty()) data.artist = tag->artist().toCString(true);
        if (!tag->album().isEmpty()) data.album = tag->album().toCString(true);
        data.year = tag->year();
    }
    if (filePath.endsWith(".mp3", Qt::CaseInsensitive)) {
        TagLib::MPEG::File file(filePath.toUtf8().data());
        if (file.ID3v2Tag()) {
            auto list = file.ID3v2Tag()->frameList("APIC");
            if (!list.isEmpty()) {
                auto *pic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(list.front());
                if (pic && pic->picture().size() > 0) {
                    data.cover.loadFromData((const uchar*)pic->picture().data(), pic->picture().size());
                }
            }
        }
    }
    return data;
}

void PlayerWidget::updateTrackInfo(const TrackMetadata &meta) {
    if (!meta.cover.isNull()) {
        QPixmap pix = QPixmap::fromImage(meta.cover);
        m_coverLabel->setPixmap(pix.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_coverLabel->clear();
    }
    m_titleLabel->setText(meta.title.isEmpty() ? "Неизвестно" : meta.title);
    m_artistLabel->setText(meta.artist);
    QString album = meta.album;
    QString year = meta.year > 0 ? QString::number(meta.year) : "";
    m_albumYearLabel->setText(album + (album.isEmpty() ? "" : " • ") + year);
}

void PlayerWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    if (m_isPulsing && m_playIcon) {
        m_pulseElapsed += 16;
        float t = (float)m_pulseElapsed / 5000.0f;
        float alpha = 1.0f - t;

        int cx = m_playIcon->geometry().center().x();
        int cy = m_playIcon->geometry().bottom() + 5;

        int leftX = cx - (cx * t);
        int rightX = cx + ((width() - cx) * t);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QColor glowColor = m_accentColor;
        glowColor.setAlphaF(alpha * 0.3);
        QPen glowPen(glowColor, 6);
        p.setPen(glowPen);
        p.drawLine(cx, cy, leftX, cy);
        p.drawLine(cx, cy, rightX, cy);

        QColor mainColor = m_accentColor;
        mainColor.setAlphaF(alpha * 0.9);
        QPen mainPen(mainColor, 2);
        p.setPen(mainPen);
        p.drawLine(cx, cy, leftX, cy);
        p.drawLine(cx, cy, rightX, cy);
    }
}

void PlayerWidget::setAccentColor(const QColor &color) {
    m_accentColor = color;
    if (m_spectrumWidget) m_spectrumWidget->setColor(color);
    QString sliderStyle = QString(
        "QSlider::groove:horizontal { height:4px; background:palette(mid); border-radius:2px; }"
        "QSlider::sub-page:horizontal { background:%1; border-radius:2px; }"
        "QSlider::handle:horizontal { background:%1; width:16px; height:16px; margin:-6px 0; border-radius:8px; }"
    ).arg(color.name());
    m_positionSlider->setStyleSheet(sliderStyle);
}

void PlayerWidget::onPlayClicked() {
    if (m_audioManager->isPlaying()) {
        m_savedPosition = m_audioManager->position();
        m_isUserManuallyStopped = true;
        m_audioManager->stop();
        updatePlayButtonIcon(false);
    } else {
        if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) return;
        m_isUserStop = false;
        m_isUserPause = false;
        m_isUserManuallyStopped = false;
        m_isSwitchingTracks = true;

        QString path = m_playlist[m_currentIndex];
        m_audioManager->setSourceFile(path);
        TrackMetadata meta = extractMetadata(path);
        updateTrackInfo(meta);

        qint64 savedPos = m_audioManager->getSavedPosition();
        m_audioManager->setSavedPosition(0);
        if (savedPos > 0) {
            m_audioManager->setPosition(savedPos);
        }

        m_audioManager->play();

        m_isSwitchingTracks = false;

        updateUI();
        updatePlayButtonIcon(true);

        if (m_metaContainer) m_metaContainer->startGlow();
        m_pulseElapsed = 0;
        m_isPulsing = true;
        m_pulseTimer->start(16);
        QTimer::singleShot(5000, this, [this](){
            m_isPulsing = false;
            m_pulseTimer->stop();
            update();
        });
    }
}

void PlayerWidget::onNextClicked() {
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size() - 1) {
        m_currentIndex++;
        onPlay();
    }
}

void PlayerWidget::onPrevClicked() {
    if (m_currentIndex > 0) {
        m_currentIndex--;
        m_audioManager->setSavedPosition(0);
        onPlay();
    } else if (m_currentIndex == 0) {
        m_currentIndex = -1;
        m_audioManager->setSavedPosition(0);
        m_audioManager->stop();
        emit stateChanged(false);
        updatePlayButtonIcon(false);
        updateTrackInfo(TrackMetadata());
    }
}

void PlayerWidget::onFeaturedClicked() {
    if (m_playlist.isEmpty() || m_currentIndex < 0) return;

    QString currentTrack = m_playlist[m_currentIndex];
    QFileInfo currentFi(currentTrack);

    QString featuredPath = QDir::homePath() + "/zmp_playlists/featured";
    QDir featuredDir(featuredPath);

    if (!featuredDir.exists()) {
        if (!featuredDir.mkpath(featuredPath)) {
            qDebug() << "Не удалось создать папку featured";
            return;
        }
    }

    if (isTrackInFeatured()) {
        QString fileName = currentFi.fileName();
        QString filePath = featuredPath + "/" + fileName;

        QMessageBox msgBox;
        msgBox.setWindowTitle("Подтверждение удаления");
        msgBox.setText("Вы уверены, что хотите удалить:\n" + filePath);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Question);
        
        int ret = msgBox.exec();
        if (ret == QMessageBox::Ok) {
            if (QFile::exists(filePath)) {
                QFile::remove(filePath);
                qDebug() << "Удален трек из featured:" << fileName;
            }
        }
    } else {
        QString fileName = currentFi.fileName();
        QString destPath = featuredPath + "/" + fileName;

        if (!QFile::exists(destPath)) {
            if (QFile::copy(currentTrack, destPath)) {
                qDebug() << "Добавлен трек в featured:" << fileName;
            } else {
                qDebug() << "Не удалось скопировать файл в featured:" << fileName;
            }
        } else {
            qDebug() << "Файл уже существует в featured:" << fileName;
        }
    }

    updateFeaturedButtonIcon();
}
