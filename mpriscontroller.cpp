#include "mpriscontroller.h"
#include "playerwidget.h"

#ifdef Q_OS_LINUX

#include <QGuiApplication>
#include <QWindow>
#include <QUrl>
#include <QFileInfo>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusArgument>
#include <QDBusMetaType>
#include <QImage>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QUuid>

// ─── MprisRootAdaptor ──────────────────────────────────────────────────────

MprisRootAdaptor::MprisRootAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

QStringList MprisRootAdaptor::SupportedMimeTypes() const
{
    return {
        QStringLiteral("audio/mpeg"),
        QStringLiteral("audio/flac"),
        QStringLiteral("audio/ogg"),
        QStringLiteral("audio/x-wav"),
        QStringLiteral("audio/aac"),
        QStringLiteral("audio/mp4"),
        QStringLiteral("audio/x-m4a"),
    };
}

void MprisRootAdaptor::Raise()
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (ctrl && ctrl->m_mainWindow) {
        ctrl->m_mainWindow->show();
        ctrl->m_mainWindow->raise();
        ctrl->m_mainWindow->activateWindow();
        if (auto *win = ctrl->m_mainWindow->windowHandle())
            win->requestActivate();
    }
}

void MprisRootAdaptor::Quit()
{
    QGuiApplication::quit();
}

// ─── MprisPlayerAdaptor ────────────────────────────────────────────────────

MprisPlayerAdaptor::MprisPlayerAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
}

QString MprisPlayerAdaptor::PlaybackStatus() const
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    return ctrl ? ctrl->playbackStatus() : QStringLiteral("Stopped");
}

QString MprisPlayerAdaptor::LoopStatus() const
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (!ctrl) return QStringLiteral("None");
    return ctrl->m_loopStatus.isEmpty() ? QStringLiteral("None") : ctrl->m_loopStatus;
}

void MprisPlayerAdaptor::setLoopStatus(const QString &status)
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (!ctrl) return;
    if (status == QStringLiteral("None") || status == QStringLiteral("Track") || status == QStringLiteral("Playlist")) {
        ctrl->m_loopStatus = status;
        emitLoopStatusChanged();
    }
}

double MprisPlayerAdaptor::Rate() const
{
    return 1.0;
}

void MprisPlayerAdaptor::setRate(double)
{
}

bool MprisPlayerAdaptor::Shuffle() const
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    return ctrl ? ctrl->m_shuffle : false;
}

void MprisPlayerAdaptor::setShuffle(bool shuffle)
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (!ctrl) return;
    ctrl->m_shuffle = shuffle;
    emitShuffleChanged();
}

QVariantMap MprisPlayerAdaptor::Metadata() const
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    return ctrl ? ctrl->metadata() : QVariantMap();
}

double MprisPlayerAdaptor::Volume() const
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    return ctrl ? ctrl->volume() : 1.0;
}

void MprisPlayerAdaptor::setVolume(double vol)
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (ctrl) {
        ctrl->m_audioManager->setVolume(vol);
    }
}

qint64 MprisPlayerAdaptor::Position() const
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    return ctrl ? ctrl->position() : 0;
}

bool MprisPlayerAdaptor::CanGoNext() const
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    return ctrl ? ctrl->canGoNext() : false;
}

bool MprisPlayerAdaptor::CanGoPrevious() const
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    return ctrl ? ctrl->canGoPrevious() : false;
}

bool MprisPlayerAdaptor::CanPlay() const
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    return ctrl ? ctrl->canPlay() : false;
}

bool MprisPlayerAdaptor::CanPause() const
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    return ctrl ? ctrl->canPause() : false;
}

void MprisPlayerAdaptor::emitPlaybackStatusChanged() { emit playbackStatusChanged(); }
void MprisPlayerAdaptor::emitLoopStatusChanged() { emit loopStatusChanged(); }
void MprisPlayerAdaptor::emitRateChanged() { emit rateChanged(); }
void MprisPlayerAdaptor::emitShuffleChanged() { emit shuffleChanged(); }
void MprisPlayerAdaptor::emitMetadataChanged() { emit metadataChanged(); }
void MprisPlayerAdaptor::emitVolumeChanged() { emit volumeChanged(); }
void MprisPlayerAdaptor::emitPositionChanged() { emit positionChanged(); }
void MprisPlayerAdaptor::emitCanGoNextChanged() { emit canGoNextChanged(); }
void MprisPlayerAdaptor::emitCanGoPreviousChanged() { emit canGoPreviousChanged(); }
void MprisPlayerAdaptor::emitCanPlayChanged() { emit canPlayChanged(); }
void MprisPlayerAdaptor::emitCanPauseChanged() { emit canPauseChanged(); }

void MprisPlayerAdaptor::Next()
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (ctrl) ctrl->m_playback->onNext();
}

void MprisPlayerAdaptor::Previous()
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (ctrl) ctrl->m_playback->onPrev();
}

void MprisPlayerAdaptor::Pause()
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (ctrl) {
        if (ctrl->m_audioManager->isPlaying())
            ctrl->m_audioManager->pause();
    }
}

void MprisPlayerAdaptor::PlayPause()
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (ctrl) ctrl->m_playback->onPlayClicked();
}

void MprisPlayerAdaptor::Stop()
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (ctrl) ctrl->m_audioManager->stop();
}

void MprisPlayerAdaptor::Play()
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (ctrl) {
        QString path = ctrl->m_playback->currentFilePath();
        if (path.isEmpty()) return;
        ctrl->m_audioManager->setSourceFile(path);
        ctrl->m_audioManager->play();
    }
}

void MprisPlayerAdaptor::Seek(qint64 offset)
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (!ctrl) return;
    // offset is in microseconds, AudioManager works in milliseconds
    qint64 newPos = ctrl->m_audioManager->position() + offset / 1000;
    newPos = qMax(0LL, newPos);
    qint64 dur = ctrl->m_audioManager->duration();
    if (dur > 0) newPos = qMin(newPos, dur);
    ctrl->m_audioManager->setPosition(newPos);
}

void MprisPlayerAdaptor::SetPosition(const QDBusObjectPath &trackId, qint64 position)
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (!ctrl) return;
    // position is in microseconds, trackId must match current track
    if (trackId.path() != ctrl->generateTrackId().path()) return;
    if (position < 0) return;
    qint64 posMs = position / 1000;
    qint64 dur = ctrl->m_audioManager->duration();
    if (dur > 0 && posMs > dur) return;
    ctrl->m_audioManager->setPosition(posMs);
}

void MprisPlayerAdaptor::OpenUri(const QString &uri)
{
    auto *ctrl = qobject_cast<MprisController*>(parent());
    if (!ctrl) return;

    QUrl url(uri);
    if (!url.isValid()) return;

    QString localFile;
    if (url.scheme() == QStringLiteral("file")) {
        localFile = url.toLocalFile();
    } else {
        localFile = url.path();
    }

    QFileInfo fi(localFile);
    if (!fi.exists() || !fi.isReadable()) return;

    ctrl->m_playback->setPlaylist({localFile});
    ctrl->m_playback->setCurrentPlaylist({localFile});
    ctrl->m_playback->onPlay();
}

// ─── MprisController ───────────────────────────────────────────────────────

MprisController::MprisController(AudioManager *audioManager, PlayerWidget *playback, QWidget *mainWindow, QObject *parent)
    : QObject(parent)
    , m_audioManager(audioManager)
    , m_playback(playback)
    , m_mainWindow(mainWindow)
    , m_shuffle(false)
{
    m_loopStatus = QStringLiteral("None");

    m_rootAdaptor = new MprisRootAdaptor(this);
    m_playerAdaptor = new MprisPlayerAdaptor(this);

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(QStringLiteral("org.mpris.MediaPlayer2.zmp"))) {
        qWarning("MPRIS: failed to register DBus service org.mpris.MediaPlayer2.zmp");
        return;
    }
    if (!bus.registerObject(QStringLiteral("/org/mpris/MediaPlayer2"),
                            this, QDBusConnection::ExportAdaptors)) {
        qWarning("MPRIS: failed to register object /org/mpris/MediaPlayer2");
        bus.unregisterService(QStringLiteral("org.mpris.MediaPlayer2.zmp"));
        return;
    }

    qDebug("MPRIS: registered org.mpris.MediaPlayer2.zmp");

    // ── AudioManager connections ──
    connect(audioManager, &AudioManager::stateChanged, this, [this](bool playing) {
        Q_UNUSED(playing);
        m_playerAdaptor->emitPlaybackStatusChanged();
        m_playerAdaptor->emitCanPlayChanged();
        m_playerAdaptor->emitCanPauseChanged();
        m_playerAdaptor->emitCanGoNextChanged();
        m_playerAdaptor->emitCanGoPreviousChanged();
    });

    connect(audioManager, &AudioManager::positionChanged, this, [this]() {
        m_playerAdaptor->emitPositionChanged();
    });

    connect(audioManager, &AudioManager::durationChanged, this, [this]() {
        updateMetadata();
    });

    connect(audioManager, &AudioManager::trackEnded, this, [this]() {
        m_playerAdaptor->emitPlaybackStatusChanged();
        m_playerAdaptor->emitCanPlayChanged();
        m_playerAdaptor->emitCanPauseChanged();
        m_playerAdaptor->emitCanGoNextChanged();
        m_playerAdaptor->emitCanGoPreviousChanged();
    });

    connect(audioManager, &AudioManager::volumeChanged, this, [this]() {
        m_playerAdaptor->emitVolumeChanged();
    });

    // ── PlaybackControlWidget connections via PlayerWidget ──
    connect(playback, &PlayerWidget::trackInfoChanged, this, [this]() {
        updateMetadata();
        m_playerAdaptor->emitCanGoNextChanged();
        m_playerAdaptor->emitCanGoPreviousChanged();
    });

    connect(playback, &PlayerWidget::stateChanged, this, [this]() {
        m_playerAdaptor->emitPlaybackStatusChanged();
        m_playerAdaptor->emitCanPlayChanged();
        m_playerAdaptor->emitCanPauseChanged();
        m_playerAdaptor->emitCanGoNextChanged();
        m_playerAdaptor->emitCanGoPreviousChanged();
    });
}

MprisController::~MprisController()
{
    if (!m_coverFile.isEmpty())
        QFile::remove(m_coverFile);
    QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/org/mpris/MediaPlayer2"));
    QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.mpris.MediaPlayer2.zmp"));
}

QString MprisController::playbackStatus() const
{
    if (!m_audioManager->isPlaying()) {
        if (m_audioManager->position() > 0)
            return QStringLiteral("Paused");
        return QStringLiteral("Stopped");
    }
    return QStringLiteral("Playing");
}

double MprisController::volume() const
{
    return m_audioManager->volume();
}

bool MprisController::canGoNext() const
{
    QStringList pl = m_playback->getCurrentPlaylist();
    if (pl.isEmpty()) return false;
    int idx = -1;
    QString cur = m_playback->currentFilePath();
    for (int i = 0; i < pl.size(); ++i) {
        if (pl[i] == cur) { idx = i; break; }
    }
    return idx >= 0 && idx < pl.size() - 1;
}

bool MprisController::canGoPrevious() const
{
    QStringList pl = m_playback->getCurrentPlaylist();
    if (pl.isEmpty()) return false;
    int idx = -1;
    QString cur = m_playback->currentFilePath();
    for (int i = 0; i < pl.size(); ++i) {
        if (pl[i] == cur) { idx = i; break; }
    }
    return idx > 0;
}

bool MprisController::canPlay() const
{
    return !m_playback->getCurrentPlaylist().isEmpty();
}

bool MprisController::canPause() const
{
    return m_audioManager->isPlaying() || m_audioManager->position() > 0;
}

void MprisController::updateMetadata()
{
    TrackMetadata meta = m_playback->currentTrackMetadata();
    QString filePath = m_playback->currentFilePath();
    QDBusObjectPath trackId = generateTrackId();

    m_metadata.clear();
    m_metadata[QStringLiteral("mpris:trackid")] = QVariant::fromValue(trackId);
    m_metadata[QStringLiteral("mpris:length")] = m_audioManager->duration() * 1000;

    if (!meta.title.isEmpty())
        m_metadata[QStringLiteral("xesam:title")] = meta.title;
    if (!meta.artist.isEmpty())
        m_metadata[QStringLiteral("xesam:artist")] = QStringList{meta.artist};
    if (!meta.album.isEmpty())
        m_metadata[QStringLiteral("xesam:album")] = meta.album;
    if (meta.year > 0)
        m_metadata[QStringLiteral("xesam:contentCreated")] = QString::number(meta.year);
    if (!filePath.isEmpty())
        m_metadata[QStringLiteral("xesam:url")] = QUrl::fromLocalFile(filePath).toString();

    // cover art
    if (!meta.cover.isNull()) {
        if (!m_coverFile.isEmpty()) {
            QFile::remove(m_coverFile);
            m_coverFile.clear();
        }
        QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString fileName = QStringLiteral("zmp-cover-%1.jpg").arg(QUuid::createUuid().toString(QUuid::Id128));
        m_coverFile = tmpDir + QStringLiteral("/") + fileName;
        if (meta.cover.save(m_coverFile, "JPEG")) {
            m_metadata[QStringLiteral("mpris:artUrl")] = QUrl::fromLocalFile(m_coverFile).toString();
        } else {
            m_coverFile.clear();
        }
    }

    m_playerAdaptor->emitMetadataChanged();
}

QDBusObjectPath MprisController::generateTrackId()
{
    QStringList pl = m_playback->getCurrentPlaylist();
    QString cur = m_playback->currentFilePath();
    int idx = -1;
    for (int i = 0; i < pl.size(); ++i) {
        if (pl[i] == cur) { idx = i; break; }
    }
    if (idx < 0) idx = 0;
    return QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/Track/%1").arg(idx));
}

#endif // Q_OS_LINUX
