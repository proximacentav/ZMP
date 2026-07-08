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
    qint64 getSavedPosition() const { return m_savedPosition; }
    void setSavedPosition(qint64 pos) { m_savedPosition = pos; }
    void setEqualizerGain(int bandIndex, float gainDb);
    void setEqualizerBandFreq(int bandIndex, double freqHz);
    void setPreampGain(float gainDb);
    void setPlaybackSpeed(double speed);
    void setPitchShift(double semitones);
    void setActiveOutputDevice(const QAudioDevice &device);
    QList<QAudioDevice> availableOutputDevices() const;
    QString currentDeviceName() const;
    void setVolume(double vol);
    double volume() const { return m_volume; }
    void setSpectrumGain(float gain);

signals:
    void positionChanged(qint64 pos);
    void durationChanged(qint64 dur);
    void stateChanged(bool playing);
    void errorOccurred(const QString &error);
    void spectrumDataChanged(const QVector<float> &amplitudes);
    void trackEnded();
    void volumeChanged(double vol);

private slots:
    void updatePosition();
    void updateSpectrum();

public slots:
    void setSpectrumFps(int fps);

private:
    HSTREAM m_currentStream;
    HSTREAM m_eqFX[17];
    QVector<double> m_bands;
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
    QString m_currentFilePath;
    qint64 m_savedPosition = 0;
    QWORD m_lastPosition = 0;
    float m_eqGains[17];
    float m_preampGain = 0.0f;
    double m_volume = 1.0;
    void applyVolume();
};

#endif
