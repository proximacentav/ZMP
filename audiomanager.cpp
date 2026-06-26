#include "audiomanager.h"
#include <QDebug>
#include <QFile>
#include <QMediaDevices>
#include <QVector>

AudioManager::AudioManager(QObject *parent)
    : QObject(parent), m_currentStream(0), m_eqFX(0), m_playing(false), m_seeking(false), m_duration(0),
      m_originalFreq(44100.0), m_currentSpeed(1.0), m_currentPitch(0.0), m_spectrumGain(8.0f)
{
    if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
        qCritical() << "BASS_Init failed!";
        return;
    }
    BASS_FX_GetVersion();

    m_positionTimer = new QTimer(this);
    connect(m_positionTimer, &QTimer::timeout, this, &AudioManager::updatePosition);
    m_positionTimer->setInterval(100);

    m_spectrumTimer = new QTimer(this);
    connect(m_spectrumTimer, &QTimer::timeout, this, &AudioManager::updateSpectrum);
    m_spectrumTimer->setInterval(50);
}

AudioManager::~AudioManager() {
    stop();
    if (m_currentStream) BASS_StreamFree(m_currentStream);
    BASS_Free();
}

void AudioManager::setSourceFile(const QString &filePath) {
    stop();
    if (m_currentStream) BASS_StreamFree(m_currentStream);
    m_currentStream = 0;
    m_eqFX = 0;

    QByteArray pathBytes = QFile::encodeName(filePath);
    m_currentStream = BASS_StreamCreateFile(FALSE, pathBytes.constData(), 0, 0, BASS_STREAM_AUTOFREE);
    if (!m_currentStream) {
        qCritical() << "Failed to load file. BASS error:" << BASS_ErrorGetCode();
        emit errorOccurred("Не удалось загрузить файл");
        return;
    }

    BASS_CHANNELINFO info;
    if (BASS_ChannelGetInfo(m_currentStream, &info)) {
        m_originalFreq = info.freq;
    } else {
        m_originalFreq = 44100.0;
    }

    QWORD bytes = BASS_ChannelGetLength(m_currentStream, BASS_POS_BYTE);
    double seconds = BASS_ChannelBytes2Seconds(m_currentStream, bytes);
    m_duration = static_cast<qint64>(seconds * 1000);
    emit durationChanged(m_duration);

    m_eqFX = BASS_ChannelSetFX(m_currentStream, BASS_FX_BFX_PEAKEQ, 1);
    if (!m_eqFX) qCritical() << "Failed to create EQ effect, error:" << BASS_ErrorGetCode();

    setPlaybackSpeed(m_currentSpeed);
    setPitchShift(m_currentPitch);
}

void AudioManager::play() {
    if (!m_currentStream) { emit errorOccurred("Файл не выбран"); return; }
    if (BASS_ChannelPlay(m_currentStream, FALSE)) {
        m_playing = true;
        m_positionTimer->start();
        m_spectrumTimer->start();
        emit stateChanged(true);
    }
}

void AudioManager::pause() {
    if (m_playing && m_currentStream) {
        BASS_ChannelPause(m_currentStream);
        m_playing = false;
        m_positionTimer->stop();
        m_spectrumTimer->stop();
        emit stateChanged(false);
    }
}

void AudioManager::stop() {
    if (m_currentStream) {
        BASS_ChannelStop(m_currentStream);
        m_playing = false;
        m_positionTimer->stop();
        m_spectrumTimer->stop();
        emit stateChanged(false);
    }
}

void AudioManager::next() { qDebug() << "Next"; }
void AudioManager::previous() { qDebug() << "Previous"; }

qint64 AudioManager::duration() const { return m_duration; }
qint64 AudioManager::position() const {
    if (!m_currentStream) return 0;
    QWORD bytes = BASS_ChannelGetPosition(m_currentStream, BASS_POS_BYTE);
    double seconds = BASS_ChannelBytes2Seconds(m_currentStream, bytes);
    return static_cast<qint64>(seconds * 1000);
}

void AudioManager::setPosition(qint64 ms) {
    if (!m_currentStream) return;
    m_seeking = true;
    double seconds = ms / 1000.0;
    QWORD bytes = BASS_ChannelSeconds2Bytes(m_currentStream, seconds);
    BASS_ChannelSetPosition(m_currentStream, bytes, BASS_POS_BYTE);
    emit positionChanged(ms);
    m_seeking = false;
}

bool AudioManager::isPlaying() const { return m_playing; }

void AudioManager::setEqualizerGain(int bandIndex, float gainDb) {
    if (!m_currentStream || !m_eqFX) return;
    BASS_BFX_PEAKEQ eq;
    if (BASS_FXGetParameters(m_eqFX, &eq)) {
        eq.lBand = bandIndex;
        eq.fGain = gainDb;
        eq.fBandwidth = 1.0f;
        eq.lChannel = BASS_BFX_CHANALL;
        BASS_FXSetParameters(m_eqFX, &eq);
    }
}

void AudioManager::setPreampGain(float gainDb) {
    if (m_currentStream) {
        float volume = qPow(10.0f, gainDb / 20.0f);
        BASS_ChannelSetAttribute(m_currentStream, BASS_ATTRIB_VOL, volume);
    }
}

void AudioManager::setPlaybackSpeed(double speed) {
    if (!m_currentStream) return;
    m_currentSpeed = speed;
    double newFreq = m_originalFreq * speed;
    BASS_ChannelSetAttribute(m_currentStream, BASS_ATTRIB_FREQ, newFreq);
}

void AudioManager::setPitchShift(double semitones) {
    if (!m_currentStream) return;
    m_currentPitch = semitones;
    double pitchFactor = pow(2.0, semitones / 12.0);
    double newFreq = m_originalFreq * m_currentSpeed * pitchFactor;
    BASS_ChannelSetAttribute(m_currentStream, BASS_ATTRIB_FREQ, newFreq);
}

void AudioManager::setActiveOutputDevice(const QAudioDevice &device) {
    m_currentDevice = device;
    qDebug() << "Output device set to:" << device.description();
}

QList<QAudioDevice> AudioManager::availableOutputDevices() const {
    return QMediaDevices::audioOutputs();
}

QString AudioManager::currentDeviceName() const {
    return m_currentDevice.isNull() ? "По умолчанию" : m_currentDevice.description();
}

void AudioManager::updatePosition() {
    if (m_playing && m_currentStream && !m_seeking) {
        emit positionChanged(position());
    }
}

void AudioManager::updateSpectrum() {
    if (!m_currentStream || !m_playing) return;

    float fft[2048];
    int ret = BASS_ChannelGetData(m_currentStream, fft, BASS_DATA_FFT2048);
    if (ret <= 0) return;

    QVector<double> freqs = {5,20,40,75,150,300,800,1200,2500,4000,6000,10000,13000,16000,19000,22000,25000};
    QVector<float> levels;
    levels.reserve(freqs.size());

    for (double freq : freqs) {
        int index = static_cast<int>(freq / (m_originalFreq / 2.0) * 1024);
        index = qBound(0, index, 1023);
        float level = fft[index];
        level = (level / 10.0f) * m_spectrumGain;
        level = qBound(0.0f, level, 1.0f); 
        levels.append(level);
    }
    emit spectrumDataChanged(levels);
}
void AudioManager::setSpectrumGain(float gain) {
    m_spectrumGain = gain;
}
void AudioManager::setSpectrumFps(int fps) {
    if (fps <= 0) fps = 1;
    int interval = qMax(1, (int)(1000.0 / fps)); 
    m_spectrumTimer->setInterval(interval);
}