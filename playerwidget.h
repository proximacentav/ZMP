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
    void onPlay();
    void onPause();
    void onStop();
    void onNext();
    void onPrevious();

public slots:
    void setMetadataHeight(int height);
    void updateSpectrum(const QVector<float> &levels);
    void setAccentColor(const QColor &color);

private slots:
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onStateChanged(bool playing);
    void onSliderMoved(int value);
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateUI();
    TrackMetadata extractMetadata(const QString &filePath);
    void updateTrackInfo(const TrackMetadata &metadata);

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
    QPushButton *m_playBtn, *m_pauseBtn, *m_stopBtn, *m_nextBtn, *m_prevBtn;
    QListWidget *m_playlistWidget;
    QStringList m_playlist;
    int m_currentIndex;
    bool m_isSeeking;
    int m_metadataHeight;
    RightMetaContainer* m_metaContainer;
    QTimer *m_pulseTimer;
    int m_pulseElapsed;
    bool m_isPulsing;
};

#endif