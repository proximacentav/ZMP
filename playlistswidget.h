#ifndef PLAYLISTSWIDGET_H
#define PLAYLISTSWIDGET_H

#include <QWidget>
#include <QListWidget>
#include "playlistmanager.h"

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
    void refreshPlaylists();

private:
    QListWidget *m_listWidget;
    PlaylistManager m_manager;
    void loadPlaylists();
};

#endif // PLAYLISTSWIDGET_H