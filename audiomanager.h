#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QAudioDevice>
#include <QVector>
#include <QTimer>
#include <bass.h>
#include <bass_fx.h>

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
    void setEqualizerGain(int bandIndex, float gainDb);
    void setEqualizerBandFreq(int bandIndex, double freqHz);
    void setPreampGain(float gainDb);
    void setPlaybackSpeed(double speed);
    void setPitchShift(double semitones);
    void setActiveOutputDevice(const QAudioDevice &device);
    QList<QAudioDevice> availableOutputDevices() const;
    QString currentDeviceName() const;
    void setSpectrumGain(float gain);

signals:
    void positionChanged(qint64 pos);
    void durationChanged(qint64 dur);
    void stateChanged(bool playing);
    void errorOccurred(const QString &error);
    void spectrumDataChanged(const QVector<float> &amplitudes);

private slots:
    void updatePosition();
    void updateSpectrum();

public slots:
    void setSpectrumFps(int fps);

private:
    HSTREAM m_currentStream;
    DWORD m_eqFX;
    bool m_playing;
    bool m_seeking;
    qint64 m_duration;
    QTimer *m_positionTimer;
    QTimer *m_spectrumTimer;
    double m_originalFreq;
    double m_currentSpeed;
    double m_currentPitch;
    QAudioDevice m_currentDevice;
    float m_spectrumGain;
 // Вотч Дэма
};

#endif
