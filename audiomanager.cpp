#include "audiomanager.h"
#include "metadataextractor.h"
#include <QDebug>
#include <QFile>
#include <QMediaDevices>

AudioManager::AudioManager(QObject *parent)
    : QObject(parent), m_currentStream(0), m_eqFX(0), m_playing(false), m_seeking(false), m_duration(0)
{
    if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
        qCritical() << "BASS_Init failed!";
        return;
    }
    BASS_FX_GetVersion();

    m_positionTimer = new QTimer(this);
    connect(m_positionTimer, &QTimer::timeout, this, &AudioManager::updatePosition);
    m_positionTimer->setInterval(100);
}

AudioManager::~AudioManager()
{
    stop();
    if (m_currentStream) BASS_StreamFree(m_currentStream);
    BASS_Free();
}

void AudioManager::setSourceFile(const QString &filePath)
{
    m_currentFilePath = filePath;

    MetadataExtractor extractor;
    TrackMetadata metadata = extractor.extract(filePath);
    updateMetadata(metadata);

    stop();
    if (m_currentStream) BASS_StreamFree(m_currentStream);
    m_currentStream = 0;
    m_eqFX = 0;

    QByteArray pathBytes = QFile::encodeName(filePath);
    m_currentStream = BASS_StreamCreateFile(FALSE, pathBytes.constData(), 0, 0, BASS_STREAM_AUTOFREE | BASS_SAMPLE_FLOAT);
    if (!m_currentStream) {
        qCritical() << "Failed to load file. BASS error:" << BASS_ErrorGetCode();
        emit errorOccurred("Не удалось загрузить файл");
        return;
    }

    QWORD bytes = BASS_ChannelGetLength(m_currentStream, BASS_POS_BYTE);
    double seconds = BASS_ChannelBytes2Seconds(m_currentStream, bytes);
    m_duration = static_cast<qint64>(seconds * 1000);
    emit durationChanged(m_duration);

    m_eqFX = BASS_ChannelSetFX(m_currentStream, BASS_FX_BFX_PEAKEQ, 1);
    if (!m_eqFX) qWarning() << "Failed to create EQ effect!";
}

void AudioManager::play()
{
    if (!m_currentStream) {
        emit errorOccurred("Файл не выбран");
        return;
    }
    if (BASS_ChannelPlay(m_currentStream, FALSE)) {
        m_playing = true;
        m_positionTimer->start();
        emit stateChanged(true);
    } else {
        qCritical() << "BASS_ChannelPlay failed:" << BASS_ErrorGetCode();
    }
}

void AudioManager::pause()
{
    if (m_playing && m_currentStream) {
        BASS_ChannelPause(m_currentStream);
        m_playing = false;
        m_positionTimer->stop();
        emit stateChanged(false);
    }
}

void AudioManager::stop()
{
    if (m_currentStream) {
        BASS_ChannelStop(m_currentStream);
        m_playing = false;
        m_positionTimer->stop();
        emit stateChanged(false);
    }
}

void AudioManager::next()
{
    qDebug() << "Next track (requires playlist implementation)";
    emit nextRequested();
}

void AudioManager::previous()
{
    qDebug() << "Previous track (requires playlist implementation)";
    emit previousRequested();
}

void AudioManager::setPosition(qint64 ms)
{
    if (!m_currentStream) return;
    m_seeking = true;
    double seconds = ms / 1000.0;
    QWORD bytes = BASS_ChannelSeconds2Bytes(m_currentStream, seconds);
    BASS_ChannelSetPosition(m_currentStream, bytes, BASS_POS_BYTE);
    emit positionChanged(ms);
    m_seeking = false;
}

void AudioManager::updatePosition()
{
    if (m_playing && m_currentStream && !m_seeking) {
        emit positionChanged(position());
    }
}

qint64 AudioManager::position() const
{
    if (!m_currentStream) return 0;
    QWORD bytes = BASS_ChannelGetPosition(m_currentStream, BASS_POS_BYTE);
    double seconds = BASS_ChannelBytes2Seconds(m_currentStream, bytes);
    return static_cast<qint64>(seconds * 1000);
}

qint64 AudioManager::duration() const
{
    return m_duration;
}

bool AudioManager::isPlaying() const
{
    return m_playing;
}

void AudioManager::setEqualizerGain(int bandIndex, float gainDb)
{
    if (!m_currentStream || !m_eqFX) return;
    BASS_BFX_PEAKEQ eq;
    if (BASS_FXGetParameters(m_eqFX, &eq)) {
        eq.lBand = bandIndex;
        eq.fGain = gainDb;   // от -500 до +500 dB
        eq.fBandwidth = 1.0f;
        eq.lChannel = BASS_BFX_CHANALL;
        BASS_FXSetParameters(m_eqFX, &eq);
    }
}

void AudioManager::setPreampGain(float gainDb)
{
    if (m_currentStream) {
        float volume = qPow(10.0f, gainDb / 20.0f); // ±1000 dB
        BASS_ChannelSetAttribute(m_currentStream, BASS_ATTRIB_VOL, volume);
    }
}

void AudioManager::setActiveOutputDevice(const QAudioDevice &device)
{
    m_currentDevice = device;
    qDebug() << "Output device set to:" << device.description();
}

QList<QAudioDevice> AudioManager::availableOutputDevices() const
{
    return QMediaDevices::audioOutputs();
}

QString AudioManager::currentDeviceName() const
{
    return m_currentDevice.isNull() ? "По умолчанию" : m_currentDevice.description();
}

void AudioManager::updateMetadata(const TrackMetadata &metadata)
{
    m_currentMetadata = metadata;
    emit metadataChanged(metadata);
}