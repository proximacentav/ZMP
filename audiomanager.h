#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QAudioDevice>
#include <QVector>
#include <QTimer>
#include <bass.h>
#include <bass_fx.h>
#include "metadataextractor.h"

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
    void setPreampGain(float gainDb);
    void setActiveOutputDevice(const QAudioDevice &device);
    QList<QAudioDevice> availableOutputDevices() const;
    QString currentDeviceName() const;
    QString currentFilePath() const { return m_currentFilePath; }
    TrackMetadata currentMetadata() const { return m_currentMetadata; }

signals:
    void positionChanged(qint64 pos);
    void durationChanged(qint64 dur);
    void stateChanged(bool playing);
    void metadataChanged(const TrackMetadata &metadata);
    void errorOccurred(const QString &error);
    void nextRequested();
    void previousRequested();

private slots:
    void updatePosition();

private:
    void updateMetadata(const TrackMetadata &metadata);

    HSTREAM m_currentStream;
    DWORD m_eqFX;
    bool m_playing;
    bool m_seeking;
    qint64 m_duration;
    QTimer *m_positionTimer;
    QString m_currentFilePath;
    TrackMetadata m_currentMetadata;
    QAudioDevice m_currentDevice;
};

#endif // AUDIOMANAGER_H