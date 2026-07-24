#include "audiomanager.h"
#include "translator.h"
#include <cmath>
#include <cfloat>
#include <QDebug>
#include <QFile>
#include <QMediaDevices>
#include <QVector>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QCryptographicHash>

AudioManager::AudioManager(QObject *parent)
    : QObject(parent), m_currentStream(0), m_playing(false), m_seeking(false), m_duration(0),
      m_originalFreq(44100.0), m_currentSpeed(1.0), m_currentPitch(0.0), m_spectrumGain(8.0f)
{
    for (int i = 0; i < 17; ++i) {
        m_eqFX[i] = 0;
        m_eqGains[i] = 0.0f;
    }

    // Инициализировать частоты полос
    m_bands = {5,20,40,75,150,300,800,1200,2500,4000,6000,10000,13000,16000,19000,20000,25000};
    
    cleanupTempDir();

    if (!BASS_Init(-1, 44100, BASS_DEVICE_LATENCY, 0, NULL)) {
        qCritical() << "BASS_Init failed!";
        return;
    }

    BASS_SetConfig(BASS_CONFIG_BUFFER, 500);
    BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 100);

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
    for (int i = 0; i < 17; ++i) {
        if (m_eqFX[i]) BASS_FXFree(m_eqFX[i]);
    }
    if (m_echoFX) BASS_FXFree(m_echoFX);
    if (m_currentStream) BASS_StreamFree(m_currentStream);
    BASS_Free();
    cleanupTempDir();
}

void AudioManager::setSourceFile(const QString &filePath) {
    stop();
    if (m_currentStream) BASS_StreamFree(m_currentStream);
    m_currentStream = 0;
    for (int i = 0; i < 15; ++i) {
        m_eqFX[i] = 0;
    }
    m_echoFX = 0;
    m_currentFilePath = filePath;

    QString playPath = filePath;
    if (m_maxBitrate > 0) {
        int trackBitrate = detectBitrate(filePath);
        if (trackBitrate > 0 && trackBitrate > m_maxBitrate) {
            QString transcoded = transcodeFile(filePath, m_maxBitrate);
            if (!transcoded.isEmpty()) {
                playPath = transcoded;
            }
        }
    }

    QByteArray pathBytes = QFile::encodeName(playPath);
    m_currentStream = BASS_StreamCreateFile(FALSE, pathBytes.constData(), 0, 0, BASS_STREAM_AUTOFREE);
    if (!m_currentStream) {
        qCritical() << "Failed to load file. BASS error:" << BASS_ErrorGetCode();
        emit errorOccurred(ztr("Не удалось загрузить файл"));
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

    for (int i = 0; i < 17; ++i) {
        m_eqFX[i] = BASS_ChannelSetFX(m_currentStream, BASS_FX_BFX_PEAKEQ, 1);
        if (!m_eqFX[i]) {
            qCritical() << "Failed to create EQ effect " << i << ", error:" << BASS_ErrorGetCode();
        }
    }
    qDebug() << "Created 17 EQ effects";

    setPlaybackSpeed(m_currentSpeed);
    setPitchShift(m_currentPitch);
    setPreampGain(m_preampGain);
    setEchoEnabled(m_echoEnabled);
    for (int i = 0; i < 17; ++i) {
        setEqualizerGain(i, m_eqGains[i]);
    }
}

void AudioManager::play() {
    if (!m_currentStream) { emit errorOccurred(ztr("Файл не выбран")); return; }
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
        m_savedPosition = position();
        BASS_ChannelStop(m_currentStream);
        m_playing = false;
        m_positionTimer->stop();
        m_spectrumTimer->stop();
        emit stateChanged(false);
    }
}

void AudioManager::next() {
    if (m_currentFilePath.isEmpty()) {
        emit errorOccurred(ztr("Файл не выбран"));
        return;
    }
    
    QStringList files;
    files << m_currentFilePath;
    QFileInfo fi(m_currentFilePath);
    QDir dir = fi.dir();
    QStringList filters;
    filters << "*.mp3" << "*.flac" << "*.wav" << "*.ogg" << "*.aac" << "*.m4a";
    
    QStringList allFiles = dir.entryList(filters, QDir::Files | QDir::Readable, QDir::Name);
    int currentIndex = allFiles.indexOf(fi.fileName());
    
    if (currentIndex >= 0 && currentIndex < allFiles.size() - 1) {
        QString nextFile = dir.filePath(allFiles[currentIndex + 1]);
        setSourceFile(nextFile);
        play();
    } else {
        emit errorOccurred(ztr("Больше файлов в очереди"));
    }
}

void AudioManager::previous() {
    if (m_currentFilePath.isEmpty()) {
        emit errorOccurred(ztr("Файл не выбран"));
        return;
    }
    
    QStringList files;
    files << m_currentFilePath;
    QFileInfo fi(m_currentFilePath);
    QDir dir = fi.dir();
    QStringList filters;
    filters << "*.mp3" << "*.flac" << "*.wav" << "*.ogg" << "*.aac" << "*.m4a";
    
    QStringList allFiles = dir.entryList(filters, QDir::Files | QDir::Readable, QDir::Name);
    int currentIndex = allFiles.indexOf(fi.fileName());
    
    if (currentIndex > 0) {
        QString prevFile = dir.filePath(allFiles[currentIndex - 1]);
        setSourceFile(prevFile);
        play();
    } else {
        emit errorOccurred(ztr("Это первый файл в очереди"));
    }
}

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
    m_lastPosition = bytes;
    emit positionChanged(ms);
    m_seeking = false;
}

bool AudioManager::isPlaying() const { return m_playing; }

void AudioManager::setEqualizerGain(int bandIndex, float gainDb) {
    if (bandIndex < 0 || bandIndex >= 17) return;
    m_eqGains[bandIndex] = gainDb;
    if (!m_currentStream) return;

    BASS_BFX_PEAKEQ eq = {0};
    eq.lBand = 0;
    eq.fGain = gainDb;
    eq.fBandwidth = 1.0f;
    eq.fCenter = m_bands[bandIndex];
    eq.lChannel = BASS_BFX_CHANALL;

    if (!m_eqFX[bandIndex]) return;

    BASS_FXSetParameters(m_eqFX[bandIndex], &eq);
}

void AudioManager::setPreampGain(float gainDb) {
    m_preampGain = gainDb;
    applyVolume();
}

void AudioManager::setVolume(double vol) {
    vol = qBound(0.0, vol, 1.0);
    m_volume = vol;
    applyVolume();
    emit volumeChanged(m_volume);
}

void AudioManager::applyVolume() {
    if (!m_currentStream) return;
    float linear = qPow(10.0f, m_preampGain / 20.0f);
    if (!std::isfinite(linear)) linear = FLT_MAX;
    linear *= static_cast<float>(m_volume);
    BASS_ChannelSetAttribute(m_currentStream, BASS_ATTRIB_VOL, linear);
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

void AudioManager::setEchoEnabled(bool enabled) {
    m_echoEnabled = enabled;
    if (!m_currentStream) return;

    if (enabled) {
        if (m_echoFX) return;
        m_echoFX = BASS_ChannelSetFX(m_currentStream, BASS_FX_BFX_ECHO4, 0);
        if (!m_echoFX) {
            qWarning() << "Failed to create echo effect, error:" << BASS_ErrorGetCode();
            return;
        }
        BASS_BFX_ECHO4 echo = {0};
        echo.lChannel = BASS_BFX_CHANALL;
        echo.bStereo = TRUE;
        echo.fDryMix = 1.0f;
        echo.fWetMix = 0.5f;
        echo.fFeedback = 0.3f;
        echo.fDelay = 0.4f;
        BASS_FXSetParameters(m_echoFX, &echo);
    } else {
        if (m_echoFX) {
            BASS_ChannelRemoveFX(m_currentStream, m_echoFX);
            m_echoFX = 0;
        }
    }
}

void AudioManager::setActiveOutputDevice(const QAudioDevice &device) {
    m_currentDevice = device;
    qDebug() << "Output device set to:" << device.description();
}

QList<QAudioDevice> AudioManager::availableOutputDevices() const {
    return QMediaDevices::audioOutputs();
}

QString AudioManager::currentDeviceName() const {
    return m_currentDevice.isNull() ? ztr("По умолчанию") : m_currentDevice.description();
}

void AudioManager::updatePosition() {
    if (m_playing && m_currentStream && !m_seeking) {
        if (BASS_ChannelIsActive(m_currentStream) == BASS_ACTIVE_STOPPED) {
            m_playing = false;
            m_positionTimer->stop();
            m_spectrumTimer->stop();
            emit trackEnded();
            return;
        }
        QWORD currentPos = BASS_ChannelGetPosition(m_currentStream, BASS_POS_BYTE);
        if (currentPos != m_lastPosition) {
            m_lastPosition = currentPos;
            double seconds = BASS_ChannelBytes2Seconds(m_currentStream, currentPos);
            emit positionChanged(static_cast<qint64>(seconds * 1000));
        }
    }
}

void AudioManager::updateSpectrum() {
    if (!m_currentStream || !m_playing) return;

    float fft[2048];
    int ret = BASS_ChannelGetData(m_currentStream, fft, BASS_DATA_FFT2048 | BASS_DATA_FFT_NOWINDOW);
    if (ret <= 0) return;

    int numBands = qBound(2, m_spectrumBands, 16000);
    QVector<double> freqs;
    freqs.reserve(numBands);
    double minFreq = 1.0;
    double maxFreq = 30000.0;
    double step = (maxFreq - minFreq) / numBands;
    for (int i = 0; i < numBands; ++i) {
        freqs.append(minFreq + (i + 0.5) * step);
    }
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
    emit spectrumDataChanged(levels, freqs);
}
void AudioManager::setSpectrumGain(float gain) {
    m_spectrumGain = gain;
}
void AudioManager::setSpectrumBands(int bands) {
    m_spectrumBands = qBound(2, bands, 16000);
}
int AudioManager::getPCMData(float *buffer, int maxSamples) const {
    if (!m_currentStream || !m_playing) return 0;
    return BASS_ChannelGetData(m_currentStream, buffer, static_cast<DWORD>(maxSamples * sizeof(float)) | BASS_DATA_FLOAT);
}
void AudioManager::setSpectrumFps(int fps) {
    if (fps <= 0) fps = 1;
    int interval = qMax(1, (int)(1000.0 / fps)); 
    m_spectrumTimer->setInterval(interval);
}

void AudioManager::setMaxBitrate(int bitrate) {
    m_maxBitrate = bitrate;
}

int AudioManager::detectBitrate(const QString &filePath) {
    QProcess ffprobe;
    ffprobe.start("ffprobe", {
        "-v", "error",
        "-select_streams", "a:0",
        "-show_entries", "stream=bit_rate",
        "-of", "default=noprint_wrappers=1:nokey=1",
        filePath
    });
    ffprobe.waitForFinished(5000);
    QString output = QString::fromUtf8(ffprobe.readAllStandardOutput()).trimmed();
    bool ok = false;
    int bitrate = output.toInt(&ok);
    if (ok && bitrate > 0) {
        return bitrate / 1000;
    }
    return 0;
}

QString AudioManager::transcodeFile(const QString &filePath, int targetBitrate) {
    QString tmpDir = QDir::tempPath() + "/zmp";
    QDir().mkpath(tmpDir);

    QByteArray pathHash = QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex();
    QString tmpFileName = pathHash + "_" + QString::number(targetBitrate) + "kbps.mp3";
    QString tmpPath = tmpDir + "/" + tmpFileName;

    if (QFile::exists(tmpPath)) {
        m_tempFilePaths.append(tmpPath);
        return tmpPath;
    }

    QProcess ffmpeg;
    ffmpeg.start("ffmpeg", {
        "-i", filePath,
        "-b:a", QString::number(targetBitrate) + "k",
        "-y", tmpPath
    });
    ffmpeg.waitForFinished(60000);

    if (ffmpeg.exitCode() == 0 && QFile::exists(tmpPath)) {
        m_tempFilePaths.append(tmpPath);
        return tmpPath;
    }
    return {};
}

void AudioManager::cleanupTempDir() {
    QString tmpDir = QDir::tempPath() + "/zmp";
    QDir dir(tmpDir);
    if (dir.exists()) {
        dir.removeRecursively();
    }
    m_tempFilePaths.clear();
}