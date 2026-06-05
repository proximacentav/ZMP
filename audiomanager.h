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
    qint64 duration() const;
    qint64 position() const;
    void setPosition(qint64 ms);
    bool isPlaying() const;

    void setActiveOutputDevice(const QAudioDevice &device);
    QList<QAudioDevice> availableOutputDevices() const;
    QString currentDeviceName() const;

    void setEqualizerGain(int bandIndex, float gainDb);
    void setPreampGain(float gainDb);

signals:
    void positionChanged(qint64 pos);
    void durationChanged(qint64 dur);
    void stateChanged(bool playing);
    void errorOccurred(const QString &error);

private slots:
    void updatePosition();

private:
    HSTREAM m_currentStream;
    DWORD m_eqFX;
    bool m_playing;
    qint64 m_duration;
    QTimer *m_positionTimer;
    QAudioDevice m_currentDevice;
    bool m_seeking;
};

#endif // AUDIOMANAGER_H