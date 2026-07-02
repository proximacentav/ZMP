#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QSlider>
#include <QImage>
#include <QVector>
#include "audiomanager.h"
#include "iconbutton.h"

struct TrackMetadata {
    QString title;
    QString artist;
    QString album;
    int year = 0;
    QImage cover;
};

class SpectrumWidget;  // forward
class RightMetaContainer;
class PlayerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlayerWidget(AudioManager *audioManager, QWidget *parent = nullptr);
    void setPlaylist(const QStringList &files);
    QStringList getCurrentPlaylist() const { return m_playlist; }
    void onPlay();
    void onStateChanged(bool playing);
    void onTrackStarted();
    void setIconSize(int size);
    void loadIcons();

signals:
    void stateChanged(bool playing);
    void currentPlaylistChanged(const QStringList &tracks);

signals:
    void featuredUpdated();

public slots:
    void setMetadataHeight(int height);
    void updateSpectrum(const QVector<float> &levels);
    void setAccentColor(const QColor &color);
    void setCurrentPlaylist(const QStringList &tracks);

private slots:
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onSliderMoved(int value);
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);
    void onPlayClicked();
    void onNextClicked();
    void onPrevClicked();
    void onFeaturedClicked();
    void onAddToPlaylistClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateUI();
    TrackMetadata extractMetadata(const QString &filePath);
    void updateTrackInfo(const TrackMetadata &metadata);
    void updatePlayButtonIcon(bool playing);
    void updateNextPrevButtonIcons();
    void updateFeaturedButtonIcon();
    bool isTrackInFeatured();
    void showAddToPlaylistDialog();

    AudioManager *m_audioManager;
    QLabel *m_coverLabel;
    QLabel *m_titleLabel;
    QLabel *m_artistLabel;
    QLabel *m_albumYearLabel;
    QWidget *m_spectrumContainer;
    SpectrumWidget *m_spectrumWidget;
    QColor m_accentColor;
    QSlider *m_positionSlider;
    QLabel *m_timeLabel;
    QListWidget *m_playlistWidget;
    IconButton *m_playIcon;
    IconButton *m_prevIcon;
    IconButton *m_nextIcon;
    IconButton *m_featuredIcon;
    IconButton *m_addToPlaylistIcon;
    QStringList m_playlist;
    QStringList m_currentPlaylistTracks;
    int m_currentIndex;
    bool m_isSeeking;
    int m_metadataHeight;
    int m_iconSize;
    RightMetaContainer* m_metaContainer;
    QTimer *m_pulseTimer;
    int m_pulseElapsed;
    bool m_isPulsing;
    bool m_isSwitchingTracks = false;
    bool m_isUserStop = false;
    bool m_isUserPause = false;
    bool m_isUserManuallyStopped = false;
    qint64 m_savedPosition = 0;
};

#endif
