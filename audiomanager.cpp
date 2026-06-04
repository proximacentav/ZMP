#include "audiomanager.h"
#include <QMediaDevices>
#include <QUrl>
#include <QDebug>

AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_playing(false)
{
    m_player->setAudioOutput(m_audioOutput);

    // Настройка громкости (опционально)
    m_audioOutput->setVolume(1.0);

    // Связи
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AudioManager::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &AudioManager::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &AudioManager::onDurationChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &AudioManager::onErrorOccurred);
}

AudioManager::~AudioManager()
{
    stop();
}

void AudioManager::setSourceFile(const QString &filePath)
{
    stop();
    m_player->setSource(QUrl::fromLocalFile(filePath));
    qDebug() << "Source set:" << filePath;
}

void AudioManager::play()
{
    if (m_player->source().isEmpty()) {
        emit errorOccurred("Не выбран файл для воспроизведения");
        return;
    }
    m_player->play();
    m_playing = true;
    emit stateChanged(true);
    qDebug() << "Play started";
}

void AudioManager::pause()
{
    if (m_playing) {
        m_player->pause();
        m_playing = false;
        emit stateChanged(false);
        qDebug() << "Paused";
    }
}

void AudioManager::stop()
{
    m_player->stop();
    m_playing = false;
    emit stateChanged(false);
    qDebug() << "Stopped";
}

qint64 AudioManager::duration() const
{
    return m_player->duration();
}

qint64 AudioManager::position() const
{
    return m_player->position();
}

void AudioManager::setPosition(qint64 ms)
{
    m_player->setPosition(ms);
}

bool AudioManager::isPlaying() const
{
    return m_playing;
}

void AudioManager::setActiveOutputDevice(const QAudioDevice &device)
{
    m_audioOutput->setDevice(device);
    qDebug() << "Output device set to:" << device.description();
}

QList<QAudioDevice> AudioManager::availableOutputDevices() const
{
    return QMediaDevices::audioOutputs();
}

QString AudioManager::currentDeviceName() const
{
    return m_audioOutput->device().description();
}

void AudioManager::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        m_playing = false;
        emit stateChanged(false);
        qDebug() << "End of media";
    } else if (status == QMediaPlayer::LoadedMedia) {
        emit durationChanged(duration());
        qDebug() << "Media loaded, duration:" << duration();
    } else if (status == QMediaPlayer::BufferingMedia) {
        qDebug() << "Buffering...";
    }
}

void AudioManager::onPositionChanged(qint64 pos)
{
    emit positionChanged(pos);
}

void AudioManager::onDurationChanged(qint64 dur)
{
    emit durationChanged(dur);
}

void AudioManager::onErrorOccurred(QMediaPlayer::Error error, const QString &errorString)
{
    qWarning() << "MediaPlayer error:" << error << errorString;
    emit errorOccurred(errorString);
    stop();
}