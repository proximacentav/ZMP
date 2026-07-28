#ifndef MINIPLAYERBAR_H
#define MINIPLAYERBAR_H

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "playbackcontrolwidget.h"
#include "iconbutton.h"

class MiniPlayerBar : public QWidget
{
    Q_OBJECT
public:
    explicit MiniPlayerBar(AudioManager *audioManager, QWidget *parent = nullptr);

    void setAccentColor(const QColor &color);
    void setDownloadProgress(double percent);
    void setDownloadInfo(const QString &trackName, qint64 bytesReceived, qint64 bytesTotal,
                         const QString &proxyHost, bool hasProxy);
    void setDownloadActive(bool active);

signals:
    void playClicked();
    void prevClicked();
    void nextClicked();
    void downloadCancelled();

public slots:
    void setTrackInfo(const TrackMetadata &meta);
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onStateChanged(bool playing);

private slots:
    void onSliderMoved(int value);

private:
    AudioManager *m_audioManager;
    QLabel *m_coverLabel;
    QLabel *m_titleLabel;
    QSlider *m_positionSlider;
    QLabel *m_timeLabel;
    IconButton *m_prevIcon;
    IconButton *m_playIcon;
    IconButton *m_nextIcon;
    bool m_isSeeking;
    QColor m_accentColor;
    qint64 m_duration;

    QWidget *m_downloadInfoWidget;
    QLabel *m_downloadStatusLabel;
    QLabel *m_downloadTrackLabel;
    QLabel *m_downloadSpeedLabel;
    QProgressBar *m_downloadProgress;
    QPushButton *m_cancelDownloadBtn;

    void updatePlayButtonIcon(bool playing);
};

#endif
