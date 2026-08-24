#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QAudioDevice>
#include <QVector>
#include <QTimer>
#include <QStringList>
#include <mutex>
#include <atomic>

#include "miniaudio.h"

#ifndef ZMP_NO_SOUNDTOUCH
#include "soundtouch/SoundTouch.h"
#else
// Заглушка: без SoundTouch скорость/питч не применяются, звук идёт напрямую
namespace soundtouch {
class SoundTouch {
public:
    void setSampleRate(int) {}
    void setChannels(int) {}
    void setRate(double) {}
    void clear() {}
    void flush() {}
    void putSamples(const float *, int) { m_eos = true; }
    int  receiveSamples(float *out, int max) { return 0; Q_UNUSED(out) Q_UNUSED(max) }
    int  numSamples() const { return 0; }
private:
    bool m_eos = false;
};
}
#endif

struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    float process(float x);
    void reset();
};

// Аудио-движок ZMP: miniaudio (декодирование+вывод) + SoundTouch (скорость/
// питч) + собственный 17-полосный biquad-эквалайзер + KissFFT (спектр).
// Публичный интерфейс полностью повторяет прежний (на базе BASS).
class AudioManager : public QObject
{
    Q_OBJECT
public:
    explicit AudioManager(QObject *parent = nullptr);
    ~AudioManager();

    void setSourceFile(const QString &filePath);
    void play();
    void pause();
    void stop();
    void next();
    void previous();
    qint64 duration() const;
    qint64 position() const;
    void setPosition(qint64 ms);
    bool isPlaying() const;
    qint64 getSavedPosition() const { return m_savedPosition; }
    void setSavedPosition(qint64 pos) { m_savedPosition = pos; }
    void setEqualizerGain(int bandIndex, float gainDb);
    void setEqualizerBandFreq(int bandIndex, double freqHz);
    void setPreampGain(float gainDb);
    void setPlaybackSpeed(double speed);
    void setPitchShift(double semitones);
    void setEchoEnabled(bool enabled);
    void setActiveOutputDevice(const QAudioDevice &device);
    QList<QAudioDevice> availableOutputDevices() const;
    QString currentDeviceName() const;
    void setVolume(double vol);
    double volume() const { return m_volume; }
    void setSpectrumGain(float gain);
    void setSpectrumBands(int bands);
    int getPCMData(float *buffer, int maxSamples) const;
    void setMaxBitrate(int bitrate);

signals:
    void positionChanged(qint64 pos);
    void durationChanged(qint64 dur);
    void stateChanged(bool playing);
    void errorOccurred(const QString &error);
    void spectrumDataChanged(const QVector<float> &amplitudes, const QVector<double> &frequencies);
    void trackEnded();
    void volumeChanged(double vol);

private slots:
    void updatePosition();
    void updateSpectrum();

public slots:
    void setSpectrumFps(int fps);

private:
    // --- аудио-конвейер ---
    static void dataCallbackStatic(ma_device *pDevice, void *output,
                                   const void *input, ma_uint32 frameCount);
    void fillOutput(float *out, ma_uint32 frames);
    bool openDevice(double sampleRate);
    void closeDevice();
    void rebuildEqCoeffs();
    float currentGainLinear() const;

    ma_device m_device = {};
    bool m_deviceOpen = false;
    double m_deviceRate = 0;

    ma_decoder m_decoder;
    bool m_decoderValid = false;

    soundtouch::SoundTouch m_st;
    // 17 полос эквалайзера, коэффициенты пересчитываются при смене частоты
    Biquad m_eqL[17];
    Biquad m_eqR[17];
    double m_eqCoeffRate = 0;

    // Эхо (стерео feedback-delay)
    QVector<float> m_echoBuf[2];
    int m_echoPos = 0;

    // Кольцевой буфер выходного PCM (для спектра и projectM)
    mutable std::mutex m_ringMutex;
    QVector<float> m_pcmRing;
    int m_ringSize = 1 << 16;
    qint64 m_ringTotalWritten = 0;

    std::mutex m_paramMutex;
    mutable std::mutex m_decoderMutex;   // сериализует доступ к ma_decoder

    QVector<double> m_bands;
    bool m_playing = false;
    bool m_seeking = false;
    qint64 m_duration = 0;
    QTimer *m_positionTimer;
    QTimer *m_spectrumTimer;
    double m_srcRate = 44100;
    double m_currentSpeed;
    double m_currentPitch;
    QAudioDevice m_currentDevice;
    float m_spectrumGain;
    QString m_currentFilePath;
    qint64 m_savedPosition = 0;
    float m_eqGains[17];
    float m_preampGain = 0.0f;
    double m_volume = 1.0;
    int m_spectrumBands = 64;
    int m_maxBitrate = 0;
    bool m_echoEnabled = false;
    bool m_eofReached = false;   // декодер дошёл до конца файла
    std::atomic<bool> m_trackFinished{false};   // трек доиграл до конца (для trackEnded)
    QStringList m_tempFilePaths;
    int detectBitrate(const QString &filePath);
    QString transcodeFile(const QString &filePath, int targetBitrate);
    void cleanupTempDir();
    void applyVolume();
};

#endif
