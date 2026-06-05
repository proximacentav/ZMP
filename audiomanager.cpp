#include "audiomanager.h"
#include <QMediaDevices>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QUrl>

AudioManager::AudioManager(QObject *parent)
    : QObject(parent), m_currentStream(0), m_eqFX(0), m_playing(false), m_duration(0), m_seeking(false)
{
    if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
        qCritical() << "BASS_Init failed! Error code:" << BASS_ErrorGetCode();
        return;
    }
    BASS_FX_GetVersion();
    qDebug() << "BASS initialized successfully.";

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
    stop();
    if (m_currentStream) {
        BASS_StreamFree(m_currentStream);
        m_currentStream = 0;
        m_eqFX = 0;
    }

    QByteArray pathBytes = QFile::encodeName(filePath);
    m_currentStream = BASS_StreamCreateFile(FALSE, pathBytes.constData(), 0, 0, BASS_STREAM_AUTOFREE | BASS_SAMPLE_FLOAT);
    if (!m_currentStream) {
        qCritical() << "Failed to load file. BASS error code:" << BASS_ErrorGetCode();
        emit errorOccurred("Не удалось загрузить файл");
        return;
    }

    QWORD bytes = BASS_ChannelGetLength(m_currentStream, BASS_POS_BYTE);
    double seconds = BASS_ChannelBytes2Seconds(m_currentStream, bytes);
    m_duration = static_cast<qint64>(seconds * 1000);
    emit durationChanged(m_duration);

    m_eqFX = BASS_ChannelSetFX(m_currentStream, BASS_FX_BFX_PEAKEQ, 1);
    if (!m_eqFX) qWarning() << "Failed to create EQ effect, error:" << BASS_ErrorGetCode();

    qDebug() << "Source loaded, duration:" << m_duration << "ms";
}

void AudioManager::play()
{
    if (!m_currentStream) {
        emit errorOccurred("Не выбран файл для воспроизведения");
        return;
    }
    if (BASS_ChannelPlay(m_currentStream, FALSE)) {
        m_playing = true;
        m_positionTimer->start();
        emit stateChanged(true);
        qDebug() << "Play started";
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
        qDebug() << "Paused";
    }
}

void AudioManager::stop()
{
    if (m_currentStream) {
        BASS_ChannelStop(m_currentStream);
        m_playing = false;
        m_positionTimer->stop();
        emit stateChanged(false);
        qDebug() << "Stopped";
    }
}

qint64 AudioManager::duration() const
{
    return m_duration;
}

qint64 AudioManager::position() const
{
    if (!m_currentStream) return 0;
    QWORD bytes = BASS_ChannelGetPosition(m_currentStream, BASS_POS_BYTE);
    double seconds = BASS_ChannelBytes2Seconds(m_currentStream, bytes);
    return static_cast<qint64>(seconds * 1000);
}

void AudioManager::setPosition(qint64 ms)
{
    if (!m_currentStream) return;
    m_seeking = true;
    double seconds = ms / 1000.0;
    QWORD bytes = BASS_ChannelSeconds2Bytes(m_currentStream, seconds);
    BASS_ChannelSetPosition(m_currentStream, bytes, BASS_POS_BYTE);
    emit positionChanged(ms);  // принудительно обновляем UI
    m_seeking = false;
}

bool AudioManager::isPlaying() const
{
    return m_playing;
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

void AudioManager::setEqualizerGain(int bandIndex, float gainDb)
{
    if (!m_currentStream || !m_eqFX) return;
    BASS_BFX_PEAKEQ eqparam;
    if (BASS_FXGetParameters(m_eqFX, &eqparam)) {
        eqparam.lBand = bandIndex;
        eqparam.fGain = gainDb;
        eqparam.fBandwidth = 1.0f;
        eqparam.lChannel = BASS_BFX_CHANALL;
        BASS_FXSetParameters(m_eqFX, &eqparam);
        qDebug() << "EQ band" << bandIndex << "set to" << gainDb << "dB";
    }
}

void AudioManager::setPreampGain(float gainDb)
{
    if (m_currentStream) {
        float volume = qBound(0.0f, qPow(10.0f, gainDb / 20.0f), 2.0f);
        BASS_ChannelSetAttribute(m_currentStream, BASS_ATTRIB_VOL, volume);
    }
    qDebug() << "Preamp gain set to" << gainDb << "dB";
}

void AudioManager::updatePosition()
{
    if (m_playing && m_currentStream && !m_seeking) {
        emit positionChanged(position());
    }
}