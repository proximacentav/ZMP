#include "playerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QTime>
#include <QPainter>
#include <QPixmap>
#include <QFile>
#include <QDebug>
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
    RightMetaContainer(QWidget *parent = nullptr) : QWidget(parent), m_spectrum(nullptr) {}
    void setSpectrum(SpectrumWidget *s) { m_spectrum = s; }
protected:
    void resizeEvent(QResizeEvent *event) override {
        if (m_spectrum) {
            m_spectrum->setGeometry(0, 0, width(), height());
            m_spectrum->lower();
        }
        QWidget::resizeEvent(event);
    }
private:
    SpectrumWidget *m_spectrum;
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
    m_playBtn = new QPushButton("Play");
    m_pauseBtn = new QPushButton("Pause");
    m_stopBtn = new QPushButton("Stop");
    m_nextBtn = new QPushButton("Next");
    m_prevBtn = new QPushButton("Prev");
    btnLayout->addWidget(m_prevBtn);
    btnLayout->addWidget(m_playBtn);
    btnLayout->addWidget(m_pauseBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addWidget(m_nextBtn);
    mainLayout->addLayout(btnLayout);

    m_playlistWidget = new QListWidget;
    mainLayout->addWidget(new QLabel("Очередь воспроизведения:"));
    mainLayout->addWidget(m_playlistWidget);

    connect(m_playBtn, &QPushButton::clicked, this, &PlayerWidget::onPlay);
    connect(m_pauseBtn, &QPushButton::clicked, this, &PlayerWidget::onPause);
    connect(m_stopBtn, &QPushButton::clicked, this, &PlayerWidget::onStop);
    connect(m_nextBtn, &QPushButton::clicked, this, &PlayerWidget::onNext);
    connect(m_prevBtn, &QPushButton::clicked, this, &PlayerWidget::onPrevious);
    connect(m_positionSlider, &QSlider::sliderMoved, this, &PlayerWidget::onSliderMoved);
    connect(m_playlistWidget, &QListWidget::itemDoubleClicked, this, &PlayerWidget::onPlaylistItemDoubleClicked);
    connect(m_audioManager, &AudioManager::positionChanged, this, &PlayerWidget::onPositionChanged);
    connect(m_audioManager, &AudioManager::durationChanged, this, &PlayerWidget::onDurationChanged);
    connect(m_audioManager, &AudioManager::stateChanged, this, &PlayerWidget::onStateChanged);
    connect(m_audioManager, &AudioManager::spectrumDataChanged, this, &PlayerWidget::updateSpectrum);
}

void PlayerWidget::setPlaylist(const QStringList &files) {
    m_playlist = files;
    m_playlistWidget->clear();
    for (const QString &f : files) m_playlistWidget->addItem(QFileInfo(f).fileName());
    if (!files.isEmpty()) m_currentIndex = 0;
}

void PlayerWidget::onPlay() {
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) return;
    QString path = m_playlist[m_currentIndex];
    m_audioManager->setSourceFile(path);
    TrackMetadata meta = extractMetadata(path);
    updateTrackInfo(meta);
    m_audioManager->play();
    updateUI();
}

void PlayerWidget::onPause() { m_audioManager->pause(); }
void PlayerWidget::onStop() { m_audioManager->stop(); m_positionSlider->setValue(0); m_timeLabel->setText("00:00 / 00:00"); }
void PlayerWidget::onNext() {
    if (m_playlist.isEmpty()) return;
    m_currentIndex = (m_currentIndex + 1) % m_playlist.size();
    onPlay();
}
void PlayerWidget::onPrevious() {
    if (m_playlist.isEmpty()) return;
    m_currentIndex = (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
    onPlay();
}

void PlayerWidget::setMetadataHeight(int h) {
    m_metadataHeight = h;
    QWidget *meta = qobject_cast<QWidget*>(layout()->itemAt(0)->widget());
    if (meta) meta->setFixedHeight(h);
}

void PlayerWidget::updateSpectrum(const QVector<float> &levels) {
    if (m_spectrumWidget) m_spectrumWidget->setLevels(levels);
}

void PlayerWidget::setAccentColor(const QColor &color) {
    m_accentColor = color;
    if (m_spectrumWidget) m_spectrumWidget->setColor(color);
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
void PlayerWidget::onStateChanged(bool playing) { Q_UNUSED(playing); }
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