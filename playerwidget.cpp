#include "playerwidget.h"
#include <QVBoxLayout>

PlayerWidget::PlayerWidget(AudioManager *audioManager, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_playbackControl = new PlaybackControlWidget(audioManager, this);
    layout->addWidget(m_playbackControl);

    connect(m_playbackControl, &PlaybackControlWidget::stateChanged, this, &PlayerWidget::stateChanged);
    connect(m_playbackControl, &PlaybackControlWidget::currentPlaylistChanged, this, &PlayerWidget::currentPlaylistChanged);
    connect(m_playbackControl, &PlaybackControlWidget::featuredUpdated, this, &PlayerWidget::featuredUpdated);
    connect(audioManager, &AudioManager::durationChanged, m_playbackControl, &PlaybackControlWidget::onDurationChanged);
    connect(audioManager, &AudioManager::positionChanged, m_playbackControl, &PlaybackControlWidget::updatePosition);
}

void PlayerWidget::setIconSize(int size) {
    m_playbackControl->setIconSize(size);
}

void PlayerWidget::loadIcons() {
    m_playbackControl->loadIcons();
}

void PlayerWidget::setMetadataHeight(int height) {
    m_playbackControl->setMetadataHeight(height);
}

void PlayerWidget::updateSpectrum(const QVector<float> &levels) {
    m_playbackControl->updateSpectrum(levels);
}

void PlayerWidget::setAccentColor(const QColor &color) {
    m_playbackControl->setAccentColor(color);
}

void PlayerWidget::setPlaylist(const QStringList &files) {
    m_playbackControl->setPlaylist(files);
}

void PlayerWidget::setCurrentPlaylist(const QStringList &tracks) {
    m_playbackControl->setCurrentPlaylist(tracks);
}

void PlayerWidget::onPlay() {
    m_playbackControl->onPlay();
}

void PlayerWidget::onStateChanged(bool playing) {
    m_playbackControl->onStateChanged(playing);
}

void PlayerWidget::onTrackStarted() {
    m_playbackControl->onPlay();
}

void PlayerWidget::setTrackInfo(const TrackMetadata &meta) {
    m_playbackControl->setTrackInfo(meta);
}
