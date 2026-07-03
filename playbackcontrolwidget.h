#ifndef PLAYBACKCONTROLWIDGET_H
#define PLAYBACKCONTROLWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QImage>
#include <QListWidget>
#include <QAbstractItemView>
#include <QDialog>
#include "audiomanager.h"
#include "iconbutton.h"

class SpectrumWidget;  // forward

struct TrackMetadata {
    QString title;
    QString artist;
    QString album;
    int year = 0;
    QImage cover;
};

class PlaybackControlWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaybackControlWidget(AudioManager *audioManager, QWidget *parent = nullptr);

    void setTrackInfo(const TrackMetadata &meta);
    void setAccentColor(const QColor &color);
    void setIconSize(int size);
    void loadIcons();
    void setMetadataHeight(int height);
    void updateSpectrum(const QVector<float> &levels);
    void setPlaylist(const QStringList &files);
    void setCurrentPlaylist(const QStringList &tracks);
    QStringList getCurrentPlaylist() const { return m_playlist; }

signals:
    void stateChanged(bool playing);
    void currentPlaylistChanged(const QStringList &tracks);
    void featuredUpdated();

public slots:
    void onPlay();
    void onAddToPlaylistClicked();
    void onFeaturedClicked();
    void onDurationChanged(qint64 dur);
    void onStateChanged(bool playing);
    void onSliderMoved(int value);
    void onNext();
    void onPrev();
    void updatePosition(qint64 pos);

private slots:
    void onPlayClicked();
    void onNextClicked();
    void onPrevClicked();
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);
    void onPositionChanged(qint64 pos);

private:
    void updatePlayButtonIcon(bool playing);
    void updateNextPrevButtonIcons();
    void updateFeaturedButtonIcon();
    bool isTrackInFeatured();
    void showAddToPlaylistDialog();
    TrackMetadata extractMetadata(const QString &filePath);
    void updateUI();
    void updateTrackInfo(const TrackMetadata &meta);

    AudioManager *m_audioManager;
    QLabel *m_coverLabel;
    QLabel *m_titleLabel;
    QLabel *m_artistLabel;
    QLabel *m_albumYearLabel;
    QWidget *m_spectrumContainer;
    QWidget *m_glowContainer;
    QSlider *m_positionSlider;
    QLabel *m_timeLabel;
    QListWidget *m_playlistWidget;
    IconButton *m_prevIcon;
    IconButton *m_playIcon;
    IconButton *m_nextIcon;
    IconButton *m_featuredIcon;
    IconButton *m_addToPlaylistIcon;
    QStringList m_playlist;
    QStringList m_currentPlaylistTracks;
    int m_currentIndex;
    bool m_isSeeking;
    QColor m_accentColor;
    int m_iconSize;
    int m_metadataHeight;
    QWidget *m_metaContainer;
    SpectrumWidget *m_spectrumWidget;
    bool m_isPlaying = false;
    bool m_isSwitchingTracks = false;
    bool m_isUserStop = false;
    bool m_isUserPause = false;
    bool m_isUserManuallyStopped = false;
    qint64 m_savedPosition = 0;
};

#endif // PLAYBACKCONTROLWIDGET_H
