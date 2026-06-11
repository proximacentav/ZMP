#include "playlistmanager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>

PlaylistManager::PlaylistManager(QObject *parent) : QObject(parent) {}

QString PlaylistManager::basePath()
{
    QString home = QDir::homePath();
    QString path = home + "/zmp_playlists";
    QDir dir;
    if (!dir.exists(path))
        dir.mkpath(path);
    return path;
}

QStringList PlaylistManager::supportedExtensions() const
{
    return {".mp3", ".wav", ".flac", ".aac", ".aiff"};
}

QImage PlaylistManager::extractCoverFromFile(const QString &filePath) const
{
    QImage cover;
    if (!filePath.endsWith(".mp3", Qt::CaseInsensitive))
        return cover; // только MP3

    TagLib::FileName fn(filePath.toUtf8().data());
    TagLib::MPEG::File file(fn);
    if (file.isOpen() && file.ID3v2Tag()) {
        auto frameList = file.ID3v2Tag()->frameList("APIC");
        if (!frameList.isEmpty()) {
            auto *picFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
            if (picFrame && picFrame->picture().size() > 0) {
                cover.loadFromData((const uchar*)picFrame->picture().data(),
                                   (int)picFrame->picture().size());
            }
        }
    }
    return cover;
}

QList<PlaylistInfo> PlaylistManager::getAllPlaylists() const
{
    QList<PlaylistInfo> playlists;
    QDir baseDir(basePath());
    if (!baseDir.exists())
        return playlists;

    QStringList subDirs = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &dirName : subDirs) {
        PlaylistInfo info;
        info.name = dirName;
        info.folderPath = baseDir.absolutePath() + "/" + dirName;
        QDir playlistDir(info.folderPath);
        QStringList entries = playlistDir.entryList(QDir::Files);
        for (const QString &file : entries) {
            QFileInfo fi(file);
            QString ext = fi.suffix().toLower().prepend('.');
            if (supportedExtensions().contains(ext)) {
                info.tracks.append(playlistDir.absolutePath() + "/" + file);
            }
        }
        if (!info.tracks.isEmpty()) {
            info.cover = extractCoverFromFile(info.tracks.first());
        }
        playlists.append(info);
    }
    return playlists;
}

bool PlaylistManager::createPlaylist(const QString &name, const QStringList &filePaths)
{
    if (name.isEmpty()) return false;
    QRegularExpression validRegex("^[a-zA-Z0-9_\\-()]+$");
    if (!validRegex.match(name).hasMatch()) {
        qWarning() << "Invalid playlist name:" << name;
        return false;
    }
    QString playlistDir = basePath() + "/" + name;
    QDir dir;
    if (dir.exists(playlistDir)) {
        qWarning() << "Playlist already exists:" << name;
        return false;
    }
    if (!dir.mkpath(playlistDir)) {
        qWarning() << "Failed to create directory:" << playlistDir;
        return false;
    }
    for (const QString &srcPath : filePaths) {
        QFileInfo srcInfo(srcPath);
        QString ext = srcInfo.suffix().toLower().prepend('.');
        if (supportedExtensions().contains(ext)) {
            QString destPath = playlistDir + "/" + srcInfo.fileName();
            if (!QFile::copy(srcPath, destPath)) {
                qWarning() << "Failed to copy:" << srcPath;
            }
        }
    }
    return true;
}

bool PlaylistManager::deletePlaylist(const QString &name)
{
    QString playlistDir = basePath() + "/" + name;
    QDir dir(playlistDir);
    if (!dir.exists()) return false;
    dir.removeRecursively();
    return true;
}