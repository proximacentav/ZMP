#ifndef PLAYLISTSWIDGET_H
#define PLAYLISTSWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QImage>
#include <QStringList>

struct PlaylistInfo {
    QString name;
    QStringList tracks;
    QImage cover;
};
Q_DECLARE_METATYPE(PlaylistInfo)

class PlaylistsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaylistsWidget(QWidget *parent = nullptr);

signals:
    void playlistSelected(const QStringList &tracks);

private slots:
    void onAddClicked();
    void onDeleteClicked();
    void onItemDoubleClicked(QListWidgetItem *item);

private:
    void loadPlaylists();
    QImage extractCover(const QString &filePath);
    QString basePath() const;
    QStringList supportedExts() const;

    QListWidget *m_listWidget;
    QList<PlaylistInfo> m_playlists;
};

#endif