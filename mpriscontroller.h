#ifndef MPRISCONTROLLER_H
#define MPRISCONTROLLER_H

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QStringList>
#include <QVariantMap>
#include "audiomanager.h"
#include "playerwidget.h"

class MprisRootAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ CanQuit CONSTANT)
    Q_PROPERTY(bool CanRaise READ CanRaise CONSTANT)
    Q_PROPERTY(bool CanSetFullscreen READ CanSetFullscreen CONSTANT)
    Q_PROPERTY(bool HasTrackList READ HasTrackList CONSTANT)
    Q_PROPERTY(QString Identity READ Identity CONSTANT)
    Q_PROPERTY(QString DesktopEntry READ DesktopEntry CONSTANT)
    Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes CONSTANT)
    Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes CONSTANT)
public:
    explicit MprisRootAdaptor(QObject *parent);

    bool CanQuit() const { return false; }
    bool CanRaise() const { return true; }
    bool CanSetFullscreen() const { return false; }
    bool HasTrackList() const { return false; }
    QString Identity() const { return QStringLiteral("ZMP"); }
    QString DesktopEntry() const { return QStringLiteral("zmp"); }
    QStringList SupportedUriSchemes() const { return {QStringLiteral("file")}; }
    QStringList SupportedMimeTypes() const;

public slots:
    void Raise();
    void Quit();
};

class MprisPlayerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus NOTIFY playbackStatusChanged)
    Q_PROPERTY(QString LoopStatus READ LoopStatus WRITE setLoopStatus NOTIFY loopStatusChanged)
    Q_PROPERTY(double Rate READ Rate WRITE setRate NOTIFY rateChanged)
    Q_PROPERTY(bool Shuffle READ Shuffle WRITE setShuffle NOTIFY shuffleChanged)
    Q_PROPERTY(QVariantMap Metadata READ Metadata NOTIFY metadataChanged)
    Q_PROPERTY(double Volume READ Volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qint64 Position READ Position NOTIFY positionChanged)
    Q_PROPERTY(double MinimumRate READ MinimumRate CONSTANT)
    Q_PROPERTY(double MaximumRate READ MaximumRate CONSTANT)
    Q_PROPERTY(bool CanGoNext READ CanGoNext NOTIFY canGoNextChanged)
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious NOTIFY canGoPreviousChanged)
    Q_PROPERTY(bool CanPlay READ CanPlay NOTIFY canPlayChanged)
    Q_PROPERTY(bool CanPause READ CanPause NOTIFY canPauseChanged)
    Q_PROPERTY(bool CanSeek READ CanSeek CONSTANT)
    Q_PROPERTY(bool CanControl READ CanControl CONSTANT)
public:
    explicit MprisPlayerAdaptor(QObject *parent);

    QString PlaybackStatus() const;
    QString LoopStatus() const;
    void setLoopStatus(const QString &status);
    double Rate() const;
    void setRate(double rate);
    bool Shuffle() const;
    void setShuffle(bool shuffle);
    QVariantMap Metadata() const;
    double Volume() const;
    void setVolume(double vol);
    qint64 Position() const;
    double MinimumRate() const { return 1.0; }
    double MaximumRate() const { return 1.0; }
    bool CanGoNext() const;
    bool CanGoPrevious() const;
    bool CanPlay() const;
    bool CanPause() const;
    bool CanSeek() const { return true; }
    bool CanControl() const { return true; }

    void emitPlaybackStatusChanged();
    void emitLoopStatusChanged();
    void emitRateChanged();
    void emitShuffleChanged();
    void emitMetadataChanged();
    void emitVolumeChanged();
    void emitPositionChanged();
    void emitCanGoNextChanged();
    void emitCanGoPreviousChanged();
    void emitCanPlayChanged();
    void emitCanPauseChanged();

signals:
    void playbackStatusChanged();
    void loopStatusChanged();
    void rateChanged();
    void shuffleChanged();
    void metadataChanged();
    void volumeChanged();
    void positionChanged();
    void canGoNextChanged();
    void canGoPreviousChanged();
    void canPlayChanged();
    void canPauseChanged();

public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qint64 offset);
    void SetPosition(const QDBusObjectPath &trackId, qint64 position);
    void OpenUri(const QString &uri);
};

class MprisController : public QObject
{
    Q_OBJECT
    friend class MprisRootAdaptor;
    friend class MprisPlayerAdaptor;
public:
    explicit MprisController(AudioManager *audioManager, PlayerWidget *playback, QWidget *mainWindow, QObject *parent = nullptr);
    ~MprisController();

    QString playbackStatus() const;
    QVariantMap metadata() const { return m_metadata; }
    double volume() const;
    qint64 position() const { return m_audioManager->position() * 1000; }
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canPlay() const;
    bool canPause() const;

private:
    AudioManager *m_audioManager;
    PlayerWidget *m_playback;
    QWidget *m_mainWindow;
    MprisRootAdaptor *m_rootAdaptor;
    MprisPlayerAdaptor *m_playerAdaptor;
    QVariantMap m_metadata;
    QString m_loopStatus;
    bool m_shuffle;
    QString m_coverFile;

    void updateMetadata();
    QDBusObjectPath generateTrackId();
};

#endif // MPRISCONTROLLER_H
