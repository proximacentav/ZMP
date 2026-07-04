#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QWidget>
#include "playbackcontrolwidget.h"

class PlayerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlayerWidget(AudioManager *audioManager, QWidget *parent = nullptr);
    void setIconSize(int size);
    void loadIcons();
    void setMetadataHeight(int height);
    void updateSpectrum(const QVector<float> &levels);
    void setAccentColor(const QColor &color);
    void setPlaylist(const QStringList &files);
    void setCurrentPlaylist(const QStringList &tracks);
    void onPlay();
    void onStateChanged(bool playing);
    void onTrackStarted();
    void setTrackInfo(const TrackMetadata &meta);
    void onPlayClicked();
    void onNext();
    void onPrev();

public:
    QStringList getCurrentPlaylist() const { return m_playbackControl->getCurrentPlaylist(); }

signals:
    void stateChanged(bool playing);
    void currentPlaylistChanged(const QStringList &tracks);
    void featuredUpdated();
    void trackInfoChanged(const TrackMetadata &meta);

private:
    PlaybackControlWidget *m_playbackControl;
};

#endif
