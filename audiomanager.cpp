// Реализация miniaudio компилируется в этой единице трансляции
#define MINIAUDIO_IMPLEMENTATION
#include "audiomanager.h"
#include "depsmanager.h"
#include "translator.h"
#include <cmath>
#include <cfloat>
#include <QDebug>
#include <QHash>
#include <QFile>
#include <QMediaDevices>
#include <QVector>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QCryptographicHash>

extern "C" {
#include "kiss_fft.h"
}

// ---------------------------------------------------------------------------
//  Biquad (peaking EQ, RBJ cookbook)
// ---------------------------------------------------------------------------

float Biquad::process(float x)
{
    const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = x;
    y2 = y1; y1 = y;
    return static_cast<float>(y);
}

void Biquad::reset() { x1 = x2 = y1 = y2 = 0; }

static Biquad makePeakFilter(double f0, double fs, double gainDb, double Q)
{
    Biquad bq;
    if (f0 <= 0 || f0 >= fs / 2.0)
        return bq; // фильтр-заглушка (b0=1)
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * M_PI * f0 / fs;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * Q);

    const double a0 = 1.0 + alpha / A;
    bq.b0 = (1.0 + alpha * A) / a0;
    bq.b1 = (-2.0 * cw) / a0;
    bq.b2 = (1.0 - alpha * A) / a0;
    bq.a1 = (-2.0 * cw) / a0;
    bq.a2 = (1.0 - alpha / A) / a0;
    return bq;
}

// ---------------------------------------------------------------------------
//  AudioManager
// ---------------------------------------------------------------------------

AudioManager::AudioManager(QObject *parent)
    : QObject(parent), m_playing(false), m_seeking(false), m_duration(0),
      m_srcRate(44100), m_currentSpeed(1.0), m_currentPitch(0.0),
      m_spectrumGain(8.0f)
{
    for (int i = 0; i < 17; ++i)
        m_eqGains[i] = 0.0f;

    // Частоты полос эквалайзера
    m_bands = {5,20,40,75,150,300,800,1200,2500,4000,6000,10000,13000,16000,19000,20000,25000};

    m_pcmRing.resize(m_ringSize * 2);   // стерео: ringSize кадров по 2 семпла
    m_pcmRing.fill(0.0f);

    cleanupTempDir();

    m_positionTimer = new QTimer(this);
    connect(m_positionTimer, &QTimer::timeout, this, &AudioManager::updatePosition);
    m_positionTimer->setInterval(100);

    m_spectrumTimer = new QTimer(this);
    connect(m_spectrumTimer, &QTimer::timeout, this, &AudioManager::updateSpectrum);
    m_spectrumTimer->setInterval(50);
}

AudioManager::~AudioManager()
{
    stop();
    closeDevice();
    if (m_decoderValid) {
        ma_decoder_uninit(&m_decoder);
        m_decoderValid = false;
    }
    cleanupTempDir();
}

void AudioManager::dataCallbackStatic(ma_device *pDevice, void *output,
                                      const void *input, ma_uint32 frameCount)
{
    Q_UNUSED(input)
    static_cast<AudioManager *>(pDevice->pUserData)->fillOutput(
        static_cast<float *>(output), frameCount);
}

bool AudioManager::openDevice(double sampleRate)
{
    closeDevice();

    ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate = static_cast<ma_uint32>(sampleRate);
    cfg.dataCallback = dataCallbackStatic;
    cfg.pUserData = this;

    if (ma_device_init(nullptr, &cfg, &m_device) != MA_SUCCESS) {
        qCritical() << "miniaudio: device init failed";
        return false;
    }
    if (ma_device_start(&m_device) != MA_SUCCESS) {
        qCritical() << "miniaudio: device start failed";
        ma_device_uninit(&m_device);
        return false;
    }
    m_deviceOpen = true;
    m_deviceRate = cfg.sampleRate;
    return true;
}

void AudioManager::closeDevice()
{
    if (m_deviceOpen) {
        ma_device_stop(&m_device);
        ma_device_uninit(&m_device);
        m_deviceOpen = false;
    }
}

void AudioManager::setSourceFile(const QString &filePath)
{
    stop();

    QFileInfo fi(filePath);
    if (!fi.exists() || fi.size() == 0) {
        qCritical() << "Failed to load file. File is missing or empty:" << filePath;
        emit errorOccurred(ztr("Не удалось загрузить файл"));
        return;
    }

    {
        std::lock_guard<std::mutex> lk(m_decoderMutex);
        if (m_decoderValid) {
            ma_decoder_uninit(&m_decoder);
            m_decoderValid = false;
        }
    }

    QString playPath = filePath;
    if (m_maxBitrate > 0) {
        int trackBitrate = detectBitrate(filePath);
        if (trackBitrate > 0 && trackBitrate > m_maxBitrate) {
            QString transcoded = transcodeFile(filePath, m_maxBitrate);
            if (!transcoded.isEmpty())
                playPath = transcoded;
        }
    }

    // Сначала открываем в нативном формате, чтобы узнать частоту сэмплирования
    ma_decoder native;
    if (ma_decoder_init_file(playPath.toUtf8().constData(), nullptr, &native) != MA_SUCCESS) {
        qCritical() << "Failed to load file (decode):" << playPath;
        emit errorOccurred(ztr("Не удалось загрузить файл"));
        DependencyManager::instance()->reportMissingDependencies();
        return;
    }
    const double srcRate = native.outputSampleRate > 0 ? native.outputSampleRate : 44100;
    ma_decoder_uninit(&native);

    // Повторно открываем: f32, стерео, нативная частота
    ma_decoder_config dcfg = ma_decoder_config_init(ma_format_f32, 2,
                                                    static_cast<ma_uint32>(srcRate));
    if (ma_decoder_init_file(playPath.toUtf8().constData(), &dcfg, &m_decoder) != MA_SUCCESS) {
        qCritical() << "Failed to load file (convert):" << playPath;
        emit errorOccurred(ztr("Не удалось загрузить файл"));
        DependencyManager::instance()->reportMissingDependencies();
        return;
    }

    {
        std::lock_guard<std::mutex> lk(m_paramMutex);
        m_decoderValid = true;
        m_srcRate = srcRate;
        m_eofReached = false;
        m_trackFinished = false;

        ma_uint64 frames = 0;
        if (ma_decoder_get_length_in_pcm_frames(&m_decoder, &frames) == MA_SUCCESS && frames > 0)
            m_duration = static_cast<qint64>(frames / srcRate * 1000.0);
        else
            m_duration = 0;
    }

    emit durationChanged(m_duration);

    // SoundTouch: rate-фактор повторяет прежнюю семантику BASS_ATTRIB_FREQ:
    // и скорость, и питч меняют высоту тона вместе с длительностью.
    {
        std::lock_guard<std::mutex> lk(m_paramMutex);
        std::lock_guard<std::mutex> stLk(m_stMutex);
        m_st.setSampleRate(static_cast<ma_uint32>(srcRate));
        m_st.setChannels(2);
        m_st.clear();
        const double factor = m_currentSpeed * std::pow(2.0, m_currentPitch / 12.0);
        m_st.setRate(factor);
    }

    rebuildEqCoeffs();

    // Эхо-буфер под новую частоту
    {
        std::lock_guard<std::mutex> lk(m_paramMutex);
        const int delayLen = static_cast<int>(srcRate * 0.45);
        for (int ch = 0; ch < 2; ++ch) {
            m_echoBuf[ch].assign(delayLen, 0.0f);
        }
        m_echoPos = 0;
    }

    if (!m_deviceOpen || m_deviceRate != srcRate)
        openDevice(srcRate);

    qDebug() << "Created audio pipeline for" << filePath
             << "rate" << srcRate << "duration" << m_duration << "ms";
}

void AudioManager::rebuildEqCoeffs()
{
    std::lock_guard<std::mutex> lk(m_paramMutex);
    if (m_eqCoeffRate == m_srcRate && !m_bands.isEmpty()) {
        for (int i = 0; i < 17; ++i) {
            m_eqL[i] = makePeakFilter(m_bands[i], m_srcRate, m_eqGains[i], 1.4);
            m_eqR[i] = m_eqL[i];
        }
    } else {
        for (int i = 0; i < 17; ++i) {
            m_eqL[i] = makePeakFilter(m_bands.value(i, 1000), m_srcRate, m_eqGains[i], 1.4);
            m_eqR[i] = m_eqL[i];
        }
    }
}

float AudioManager::currentGainLinear() const
{
    float linear = std::pow(10.0f, m_preampGain / 20.0f);
    if (!std::isfinite(linear))
        linear = 1e30f;
    linear *= static_cast<float>(m_volume);
    return linear;
}

void AudioManager::fillOutput(float *out, ma_uint32 frames)
{
    const bool playing = m_playing.load(std::memory_order_relaxed);
    const bool valid = m_decoderValid.load(std::memory_order_relaxed);

    if (!playing || !valid) {
        std::memset(out, 0, sizeof(float) * frames * 2);
        return;
    }

    // 1+2. Декодер -> SoundTouch -> приём семплов.
    // Важно: m_decoderMutex и m_stMutex никогда не удерживаются одновременно,
    // иначе возможен дедлок с setPosition/setSourceFile на GUI-потоке.
    float decodeBuf[2048 * 2];
    float tmp[2048 * 2];
    int available = 0;
    int got = 0;
    bool eof = false;
    for (;;) {
        int queued = 0;
        {
            std::lock_guard<std::mutex> stLk(m_stMutex);
            if (m_eofReached.load(std::memory_order_relaxed))
                eof = true;
            else
                queued = static_cast<int>(m_st.numSamples());
        }
        if (eof || queued >= static_cast<int>(frames))
            break;

        ma_uint64 read = 0;
        {
            // Декодер общий с GUI-потоком (seek/position) — только под мьютексом
            std::lock_guard<std::mutex> lk(m_decoderMutex);
            if (!m_decoderValid.load(std::memory_order_relaxed))
                break;
            if (ma_decoder_read_pcm_frames(&m_decoder, decodeBuf, 2048, &read) != MA_SUCCESS)
                read = 0;
        }
        if (read == 0) {
            eof = true;
            break;
        }
        {
            std::lock_guard<std::mutex> stLk(m_stMutex);
            m_st.putSamples(decodeBuf, static_cast<int>(read));
        }
    }

    if (eof) {
        std::lock_guard<std::mutex> stLk(m_stMutex);
        m_st.flush();
        m_eofReached.store(true, std::memory_order_relaxed);
    }

    {
        std::lock_guard<std::mutex> stLk(m_stMutex);
        available = static_cast<int>(m_st.numSamples());
        const int take = qMin<int>(available, static_cast<int>(frames));
        got = take > 0 ? m_st.receiveSamples(tmp, take) : 0;
    }

    // 3. EQ + предусиление + эхо + клиппинг.
    // Весь блок под мьютексом: setSourceFile/setEqualizer*/setEchoEnabled могут
    // пересоздавать коэффициенты и echo-буфер параллельно (use-after-free).
    {
        std::lock_guard<std::mutex> lk(m_paramMutex);
        const float gain = currentGainLinear();
        const bool echoOn = m_echoEnabled;
        const int echoLen = static_cast<int>(m_echoBuf[0].size());

        for (int i = 0; i < got; ++i) {
            float l = tmp[i * 2 + 0];
            float r = tmp[i * 2 + 1];

            for (int b = 0; b < 17; ++b) {
                l = m_eqL[b].process(l);
                r = m_eqR[b].process(r);
            }

            // Эхо до усиления: хвосты тоже усиливаются предусилением
            if (echoOn && echoLen > 0) {
                const float dl = m_echoBuf[0][m_echoPos];
                const float dr = m_echoBuf[1][m_echoPos];
                m_echoBuf[0][m_echoPos] = l * 0.40f + dl * 0.42f;
                m_echoBuf[1][m_echoPos] = r * 0.40f + dr * 0.42f;
                l = l + dl * 0.65f;
                r = r + dr * 0.65f;
                m_echoPos = (m_echoPos + 1) % echoLen;
            }

            l *= gain;
            r *= gain;

            // Мягкий клиппинг: до 1.0 линейно, выше — tanh (POWERMODE оправдан)
            out[i * 2 + 0] = (l > 1.0f || l < -1.0f) ? std::tanh(l) : l;
            out[i * 2 + 1] = (r > 1.0f || r < -1.0f) ? std::tanh(r) : r;
        }
    }
    if (got < static_cast<int>(frames))
        std::memset(out + got * 2, 0, sizeof(float) * (frames - got) * 2);

    // 4. Пишем выход в кольцевой буфер (спектр + projectM)
    {
        std::lock_guard<std::mutex> lk(m_ringMutex);
        const qint64 start = m_ringTotalWritten % m_ringSize;
        for (ma_uint32 i = 0; i < frames; ++i) {
            const qint64 idx = (start + i) % m_ringSize;
            m_pcmRing[static_cast<int>(idx * 2 + 0)] = out[i * 2 + 0];
            m_pcmRing[static_cast<int>(idx * 2 + 1)] = out[i * 2 + 1];
        }
        m_ringTotalWritten += frames;
    }

    // Конец трека: декодер опустел и SoundTouch отдал всё
    if (m_eofReached && got < static_cast<int>(frames) && available == 0)
        m_trackFinished = true;   // updatePosition() заметит и пошлёт trackEnded
}

void AudioManager::play()
{
    if (!m_decoderValid) { emit errorOccurred(ztr("Файл не выбран")); return; }

    if (!m_deviceOpen)
        openDevice(m_srcRate);
    if (m_deviceOpen && ma_device_start(&m_device) != MA_SUCCESS) {
        emit errorOccurred(ztr("Не удалось загрузить файл"));
        return;
    }

    m_playing = true;
    m_trackFinished = false;
    m_positionTimer->start();
    m_spectrumTimer->start();
    emit stateChanged(true);
}

void AudioManager::pause()
{
    if (m_playing) {
        m_playing = false;
        m_positionTimer->stop();
        m_spectrumTimer->stop();
        emit stateChanged(false);
    }
}

void AudioManager::stop()
{
    if (m_playing || m_decoderValid) {
        m_savedPosition = position();
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

qint64 AudioManager::position() const
{
    if (!m_decoderValid)
        return 0;
    ma_uint64 frame = 0;
    {
        std::lock_guard<std::mutex> lk(m_decoderMutex);
        if (ma_decoder_get_cursor_in_pcm_frames(const_cast<ma_decoder *>(&m_decoder), &frame) != MA_SUCCESS)
            return 0;
    }
    return static_cast<qint64>(frame / m_srcRate * 1000.0);
}

void AudioManager::setPosition(qint64 ms)
{
    if (!m_decoderValid)
        return;
    m_seeking = true;
    {
        std::lock_guard<std::mutex> lk(m_decoderMutex);
        std::lock_guard<std::mutex> stLk(m_stMutex);
        const ma_int64 frame = static_cast<ma_int64>(ms / 1000.0 * m_srcRate);
        ma_decoder_seek_to_pcm_frame(&m_decoder, frame);
        m_st.clear();
        m_eofReached.store(false, std::memory_order_relaxed);
    }
    emit positionChanged(ms);
    m_seeking = false;
}

bool AudioManager::isPlaying() const { return m_playing; }

void AudioManager::setEqualizerGain(int bandIndex, float gainDb)
{
    if (bandIndex < 0 || bandIndex >= 17)
        return;
    {
        std::lock_guard<std::mutex> lk(m_paramMutex);
        m_eqGains[bandIndex] = gainDb;
        const double f0 = m_bands.value(bandIndex, 1000);
        m_eqL[bandIndex] = makePeakFilter(f0, m_srcRate, gainDb, 1.4);
        m_eqR[bandIndex] = m_eqL[bandIndex];
    }
}

void AudioManager::setEqualizerBandFreq(int bandIndex, double freqHz)
{
    if (bandIndex < 0 || bandIndex >= 17)
        return;
    {
        std::lock_guard<std::mutex> lk(m_paramMutex);
        m_bands[bandIndex] = freqHz;
        m_eqL[bandIndex] = makePeakFilter(freqHz, m_srcRate, m_eqGains[bandIndex], 1.4);
        m_eqR[bandIndex] = m_eqL[bandIndex];
    }
}

void AudioManager::setPreampGain(float gainDb)
{
    m_preampGain = gainDb;
    applyVolume();
}

void AudioManager::setVolume(double vol)
{
    vol = qBound(0.0, vol, 15.0);
    m_volume = vol;
    applyVolume();
    emit volumeChanged(m_volume);
}

void AudioManager::applyVolume()
{
    // Усиление применяется в fillOutput(); здесь ничего пересчитывать не нужно
}

void AudioManager::setPlaybackSpeed(double speed)
{
    std::lock_guard<std::mutex> lk(m_paramMutex);
    std::lock_guard<std::mutex> stLk(m_stMutex);
    m_currentSpeed = speed;
    const double factor = m_currentSpeed * std::pow(2.0, m_currentPitch / 12.0);
    m_st.setRate(factor);
}

void AudioManager::setPitchShift(double semitones)
{
    std::lock_guard<std::mutex> lk(m_paramMutex);
    std::lock_guard<std::mutex> stLk(m_stMutex);
    m_currentPitch = semitones;
    const double factor = m_currentSpeed * std::pow(2.0, m_currentPitch / 12.0);
    m_st.setRate(factor);
}

void AudioManager::setEchoEnabled(bool enabled)
{
    qDebug() << "audio: echo" << (enabled ? "enabled" : "disabled");
    std::lock_guard<std::mutex> lk(m_paramMutex);
    m_echoEnabled = enabled;
    if (enabled) {
        const int delayLen = static_cast<int>(m_srcRate * 0.45);
        for (int ch = 0; ch < 2; ++ch)
            m_echoBuf[ch].assign(delayLen, 0.0f);
        m_echoPos = 0;
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

void AudioManager::updatePosition()
{
    // Трек доиграл до конца (флаг ставится аудиопотоком в fillOutput)
    if (m_trackFinished.exchange(false)) {
        m_playing = false;
        m_positionTimer->stop();
        m_spectrumTimer->stop();
        emit stateChanged(false);
        emit trackEnded();
        return;
    }

    if (m_playing && !m_seeking) {
        const qint64 pos = position();
        emit positionChanged(pos);
    }
}

void AudioManager::updateSpectrum()
{
    if (!m_playing)
        return;

    constexpr int N = 2048;
    float mono[N];
    {
        std::lock_guard<std::mutex> lk(m_ringMutex);
        if (m_ringTotalWritten < N)
            return;
        const qint64 end = m_ringTotalWritten;
        for (int i = 0; i < N; ++i) {
            const qint64 idx = (end - N + i) % m_ringSize;
            mono[i] = (m_pcmRing[static_cast<int>(idx * 2)] +
                       m_pcmRing[static_cast<int>(idx * 2 + 1)]) * 0.5f;
        }
    }

    kiss_fft_cfg cfg = kiss_fft_alloc(N, 0, nullptr, nullptr);
    if (!cfg)
        return;
    kiss_fft_cpx in[N], out[N];
    for (int i = 0; i < N; ++i) {
        in[i].r = mono[i];
        in[i].i = 0.0f;
    }
    kiss_fft(cfg, in, out);
    free(cfg);

    int numBands = qBound(2, m_spectrumBands, 16000);
    QVector<double> freqs;
    freqs.reserve(numBands);
    const double minFreq = 1.0, maxFreq = 30000.0;
    const double step = (maxFreq - minFreq) / numBands;
    for (int i = 0; i < numBands; ++i)
        freqs.append(minFreq + (i + 0.5) * step);

    QVector<float> levels;
    levels.reserve(freqs.size());
    for (double freq : freqs) {
        int index = static_cast<int>(freq / (m_srcRate / 2.0) * (N / 2));
        index = qBound(0, index, N / 2 - 1);
        float mag = 2.0f * std::sqrt(out[index].r * out[index].r +
                                     out[index].i * out[index].i) / N;
        float level = (mag / 10.0f) * m_spectrumGain;
        level = qBound(0.0f, level, 1.0f);
        levels.append(level);
    }
    emit spectrumDataChanged(levels, freqs);
}

int AudioManager::getPCMData(float *buffer, int maxSamples) const
{
    std::lock_guard<std::mutex> lk(m_ringMutex);
    const qint64 availFrames = qMin<qint64>(m_ringTotalWritten, maxSamples);
    if (availFrames <= 0)
        return 0;
    const qint64 end = m_ringTotalWritten;
    for (qint64 i = 0; i < availFrames; ++i) {
        const qint64 idx = (end - availFrames + i) % m_ringSize;
        buffer[i * 2 + 0] = m_pcmRing[static_cast<int>(idx * 2)];
        buffer[i * 2 + 1] = m_pcmRing[static_cast<int>(idx * 2 + 1)];
    }
    return static_cast<int>(availFrames * 2 * sizeof(float)); // байты, как у BASS
}

void AudioManager::setSpectrumGain(float gain) { m_spectrumGain = gain; }
void AudioManager::setSpectrumFps(int fps) {
    if (fps <= 0) fps = 1;
    int interval = qMax(1, (int)(1000.0 / fps));
    m_spectrumTimer->setInterval(interval);
}
void AudioManager::setSpectrumBands(int bands) { m_spectrumBands = qBound(2, bands, 16000); }
void AudioManager::setMaxBitrate(int bitrate) { m_maxBitrate = bitrate; }

int AudioManager::detectBitrate(const QString &filePath) {
    // Кэш: ffprobe не должен запускаться повторно для того же файла
    // (waitForFinished блокирует главный поток до 5 секунд)
    static QHash<QString, int> cache;
    const auto it = cache.constFind(filePath);
    if (it != cache.constEnd())
        return it.value();

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
        cache.insert(filePath, bitrate / 1000);
        return bitrate / 1000;
    }
    cache.insert(filePath, 0);
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
    if (dir.exists())
        dir.removeRecursively();
}
