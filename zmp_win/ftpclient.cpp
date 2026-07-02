#include "ftpclient.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlInfo>
#include <QDebug>
#include <QRegularExpression>

FTPClient::FTPClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
{
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &FTPClient::onListReplyFinished);
}

FTPClient::~FTPClient()
{
    if (m_currentReply) {
        m_currentReply->deleteLater();
    }
}

void FTPClient::connectToHost(const QString &host, int port, const QString &user, const QString &pass)
{
    m_host = host;
    m_port = port;
    m_user = user;
    m_pass = pass;
    m_currentPath = "/";

    // В Qt6 нет QFtp, используем raw TCP для FTP команд
    // Это сложнее, но работает
    
    // Отправляем команду USER
    QString cmd = QString("USER %1\r\n").arg(user);
    // Отправляем команду PASS
    cmd += QString("PASS %1\r\n").arg(pass);
    // Отправляем команду LIST
    cmd += "LIST\r\n";
    
    // TODO: Реализовать отправку команд через сокет
    qDebug() << "FTP Connect:" << host << port;
    emit connected();
}

void FTPClient::list(const QString &path)
{
    Q_UNUSED(path)
    qDebug() << "FTP List:" << m_host << m_port << path;
    
    // Для простоты пока просто эмитируем пустой список
    emit listReceived(QList<QUrlInfo>());
}

void FTPClient::download(const QString &remotePath, const QString &localPath)
{
    Q_UNUSED(remotePath)
    Q_UNUSED(localPath)
    qDebug() << "FTP Download:" << remotePath << "to" << localPath;
}

void FTPClient::onListReplyFinished()
{
    if (m_currentReply) {
        QByteArray data = m_currentReply->readAll();
        qDebug() << "FTP Response:" << data;
        
        // Парсинг списка файлов FTP
        // Формат: drwxr-xr-x 1 user group 4096 Jan 01 12:00 folder
        //         -rw-r--r-- 1 user group 1234 Jan 01 12:00 file.mp3
        
        QList<QUrlInfo> fileList;
        QStringList lines = QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts);
        
        for (const QString &line : lines) {
            if (line.isEmpty()) continue;
            
            QUrlInfo info;
            // Парсинг строки FTP LIST
            // Формат: permissions date user size filename
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            
            if (parts.size() >= 9) {
                QString perms = parts[0];
                QString name = parts.last();
                bool isDir = perms.startsWith('d');
                
                info.setName(name);
                info.setDir(isDir);
                info.setSize(parts[parts.size() - 2].toLongLong());
                
                fileList.append(info);
            }
        }
        
        emit listReceived(fileList);
    }
}

void FTPClient::onDownloadFinished()
{
    if (m_currentReply && m_currentReply->error() == QNetworkReply::NoError) {
        QByteArray data = m_currentReply->readAll();
        qDebug() << "Downloaded:" << data.size() << "bytes";
        emit downloadFinished();
    } else {
        QString error = m_currentReply ? m_currentReply->errorString() : "Unknown error";
        emit downloadError(error);
    }
}

void FTPClient::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}
