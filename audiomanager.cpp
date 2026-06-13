#include "audiomanager.h"
#include "metadataextractor.h"
#include <QDebug>
#include <QFile>
#include <QMediaDevices>

AudioManager::AudioManager(QObject *parent)
    : QObject(parent), m_currentStream(0), m_eqFX(0), m_pitchFX(0), m_playing(false), m_seeking(false), m_duration(0),
      m_originalFreq(44100.0), m_currentSpeed(1.0), m_currentPitch(0.0)
{
    if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
        qCritical() << "BASS_Init failed!";

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
    m_currentStream = BASS_StreamCreateFile(FALSE, pathBytes.constData(), 0, 0, BASS_STREAM_AUTOFREE);
    if (!m_currentStream) {
        qCritical() << "ERROR when loading file BASS error:" << BASS_ErrorGetCode();
        emit errorOccurred("Не удалось загрузить файл");
        return;
    }

    QWORD bytes = BASS_ChannelGetLength(m_currentStream, BASS_POS_BYTE);
    double seconds = BASS_ChannelBytes2Seconds(m_currentStream, bytes);
    m_duration = static_cast<qint64>(seconds * 1000);
    emit durationChanged(m_duration);

    m_eqFX = BASS_ChannelSetFX(m_currentStream, BASS_FX_BFX_PEAKEQ, 1);
    if (!m_eqFX) qWarning() << "Failed to create EQ effect";
    BASS_CHANNELINFO info;
    if (BASS_ChannelGetInfo(m_currentStream, &info)) {
        m_originalFreq = info.freq;
        qDebug() << "Original frequency:" << m_originalFreq;
    } else {
        m_originalFreq = 44100.0;
    }
    setPlaybackSpeed(m_currentSpeed);
    setPitchShift(m_currentPitch);
}

void AudioManager::play()
{
    if (!m_currentStream) {
        emit errorOccurred("Файл не выбран");
        return;
    }
    if (BASS_ChannelPlay(m_currentStream, FALSE)) {
        m_playing = true;
        if (m_positionTimer) {
            m_positionTimer->start();
        }
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
        if (m_positionTimer) {
            m_positionTimer->stop();
        }
        emit stateChanged(false);
    }
}

void AudioManager::stop()
{
    if (m_currentStream) {
        BASS_ChannelStop(m_currentStream);
        m_playing = false;
        if (m_positionTimer) {
            m_positionTimer->stop();
        }
        emit stateChanged(false);
    }
}

void AudioManager::next()
{
    qDebug() << "Next track";
    emit nextRequested();
}

void AudioManager::previous()
{
    qDebug() << "Previous track";
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
    if (!m_currentStream) {
        qWarning() << "setEqualizerGain: no current stream";
        return;
    }
    if (!m_eqFX) {
        qWarning() << "setEqualizerGain: no EQ effect (m_eqFX=0)";
        return;
    }
    BASS_BFX_PEAKEQ eq;
    if (BASS_FXGetParameters(m_eqFX, &eq)) {
        eq.lBand = bandIndex;
        eq.fGain = gainDb;
        eq.fBandwidth = 1.0f;
        eq.lChannel = BASS_BFX_CHANALL;
        if (BASS_FXSetParameters(m_eqFX, &eq)) {
            qDebug() << "EQ band" << bandIndex << "set to" << gainDb << "dB - OK";
        } else {
            qCritical() << "Failed to set EQ parameters, BASS error:" << BASS_ErrorGetCode();
        }
    } else {
        qCritical() << "Failed to get EQ parameters, BASS error:" << BASS_ErrorGetCode();
    }
}

void AudioManager::setPlaybackSpeed(double speed)
{
    if (!m_currentStream) return;
    m_currentSpeed = speed;
    double newFreq = m_originalFreq * speed;
    BASS_ChannelSetAttribute(m_currentStream, BASS_ATTRIB_FREQ, newFreq);
    qDebug() << "Playback speed:" << speed << "new freq:" << newFreq;
}

void AudioManager::setPitchShift(double semitones) {
    if (!m_currentStream) return;
    m_currentPitch = semitones;
    double pitchFactor = pow(2.0, semitones / 12.0);
    double newFreq = m_originalFreq * m_currentSpeed * pitchFactor;
    BASS_ChannelSetAttribute(m_currentStream, BASS_ATTRIB_FREQ, newFreq);
    qDebug() << "Pitch shift set to" << semitones << "semitones, new freq:" << newFreq;
}

void AudioManager::setPreampGain(float gainDb)
{
    if (m_currentStream) {
        float volume = qPow(10.0f, gainDb / 20.0f);
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