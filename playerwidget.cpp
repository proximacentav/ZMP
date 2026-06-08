#include "playerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QTime>
#include <QIcon>
#include <QDebug>

PlayerWidget::PlayerWidget(AudioManager *audioManager, QWidget *parent)
    : QWidget(parent)
    , m_audioManager(audioManager)
    , m_currentIndex(-1)
    , m_isSeeking(false)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_trackInfo = new TrackInfoWidget(this);
    mainLayout->addWidget(m_trackInfo);

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

    m_playBtn->setIcon(QIcon(":/icons/play.svg"));
    m_playBtn->setIconSize(QSize(24,24));
    m_pauseBtn->setIcon(QIcon(":/icons/pause.svg"));
    m_pauseBtn->setIconSize(QSize(24,24));
    m_stopBtn->setIcon(QIcon(":/icons/stop.svg"));
    m_stopBtn->setIconSize(QSize(24,24));
    m_nextBtn->setIcon(QIcon(":/icons/next.svg"));
    m_nextBtn->setIconSize(QSize(24,24));
    m_prevBtn->setIcon(QIcon(":/icons/prev.svg"));
    m_prevBtn->setIconSize(QSize(24,24));
\
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
    connect(m_audioManager, &AudioManager::metadataChanged, this, &PlayerWidget::onMetadataChanged);

    qDebug() << "PlayerWidget created";
}

void PlayerWidget::setPlaylist(const QStringList &files)
{
    m_playlist = files;
    m_playlistWidget->clear();
    for (const QString &file : files) {
        QFileInfo fi(file);
        m_playlistWidget->addItem(fi.fileName());
    }
    if (!files.isEmpty())
        m_currentIndex = 0;
    qDebug() << "Playlist set with" << files.size() << "files";
}

void PlayerWidget::onPlay()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        qDebug() << "Playing:" << m_playlist[m_currentIndex];
        m_audioManager->setSourceFile(m_playlist[m_currentIndex]);
        m_audioManager->play();
        updateUI();
    } else {
        qDebug() << "No file selected or invalid index";
    }
}

void PlayerWidget::onPause() { m_audioManager->pause(); }
void PlayerWidget::onStop()
{
    m_audioManager->stop();
    m_positionSlider->setValue(0);
    m_timeLabel->setText("00:00 / 00:00");
}
void PlayerWidget::onNext()
{
    if (m_playlist.isEmpty()) return;
    m_currentIndex = (m_currentIndex + 1) % m_playlist.size();
    onPlay();
}
void PlayerWidget::onPrevious()
{
    if (m_playlist.isEmpty()) return;
    m_currentIndex = (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
    onPlay();
}
void PlayerWidget::onPositionChanged(qint64 pos)
{
    if (!m_isSeeking) {
        qint64 dur = m_audioManager->duration();
        if (dur > 0)
            m_positionSlider->setValue(static_cast<int>(pos * 1000 / dur));
        QTime t(0,0);
        QTime current = t.addMSecs(pos);
        QTime total = t.addMSecs(dur);
        m_timeLabel->setText(current.toString("mm:ss") + " / " + total.toString("mm:ss"));
    }
}
void PlayerWidget::onDurationChanged(qint64 dur) { Q_UNUSED(dur); onPositionChanged(m_audioManager->position()); }
void PlayerWidget::onStateChanged(bool playing) { Q_UNUSED(playing); }
void PlayerWidget::onSliderMoved(int value)
{
    qint64 dur = m_audioManager->duration();
    if (dur > 0) {
        m_isSeeking = true;
        m_audioManager->setPosition(value * dur / 1000);
        m_isSeeking = false;
    }
}
void PlayerWidget::onPlaylistItemDoubleClicked(QListWidgetItem *item)
{
    int row = m_playlistWidget->row(item);
    if (row >= 0 && row < m_playlist.size()) {
        m_currentIndex = row;
        onPlay();
    }
}
void PlayerWidget::onMetadataChanged(const TrackMetadata &metadata)
{
    qDebug() << "Metadata received in PlayerWidget:" << metadata.title << metadata.artist;
    m_trackInfo->updateInfo(metadata);
}
void PlayerWidget::updateUI()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        QFileInfo fi(m_playlist[m_currentIndex]);
    }
}