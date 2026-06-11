#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QStringList>
#include <QMap>
#include <QImage>

struct PlaylistInfo {
    QString name;
    QString folderPath;
    QStringList tracks;
    QImage cover;
};
Q_DECLARE_METATYPE(PlaylistInfo)

class PlaylistManager : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistManager(QObject *parent = nullptr);
    
    QList<PlaylistInfo> getAllPlaylists() const;
    bool createPlaylist(const QString &name, const QStringList &filePaths);
    bool deletePlaylist(const QString &name);
    static QString basePath();
    
private:
    QStringList supportedExtensions() const;
    QImage extractCoverFromFile(const QString &filePath) const;
};

#endif // PLAYLISTMANAGER_H