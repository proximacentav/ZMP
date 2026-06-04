#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioDevice>

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

signals:
    void positionChanged(qint64 pos);
    void durationChanged(qint64 dur);
    void stateChanged(bool playing);
    void errorOccurred(const QString &error);

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onErrorOccurred(QMediaPlayer::Error error, const QString &errorString);

private:
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    bool m_playing;
};

#endif // AUDIOMANAGER_H