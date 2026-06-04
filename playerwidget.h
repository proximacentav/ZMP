#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QSlider>
#include "audiomanager.h"

class PlayerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerWidget(AudioManager *audioManager, QWidget *parent = nullptr);
    void setPlaylist(const QStringList &files);
    const QStringList& getPlaylist() const { return m_playlist; }

public slots:
    void onPlay();
    void onPause();
    void onStop();
    void onNext();
    void onPrevious();

private slots:
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onStateChanged(bool playing);
    void onSliderMoved(int value);
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);

private:
    void updateUI();

    AudioManager *m_audioManager;
    QLabel *m_currentFileLabel;
    QSlider *m_positionSlider;
    QLabel *m_timeLabel;
    QPushButton *m_playBtn, *m_pauseBtn, *m_stopBtn, *m_nextBtn, *m_prevBtn;
    QListWidget *m_playlistWidget;
    QStringList m_playlist;
    int m_currentIndex;
    bool m_isSeeking;
};

#endif // PLAYERWIDGET_H