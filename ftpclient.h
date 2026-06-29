#ifndef FTPCLIENT_H // внимание в данный момент этот файл бесполезен из за нерабочего FTP функционала
#define FTPCLIENT_H

#include <QObject>
#include <QString>
#include <QList>
#include <QUrlInfo>

class QNetworkAccessManager;
class QNetworkReply;

class FTPClient : public QObject
{
    Q_OBJECT

public:
    explicit FTPClient(QObject *parent = nullptr);
    ~FTPClient();

    void connectToHost(const QString &host, int port, const QString &user, const QString &pass);
    void list(const QString &path = "/");
    void download(const QString &remotePath, const QString &localPath);

signals:
    void connected();
    void listReceived(const QList<QUrlInfo> &list);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    void downloadError(const QString &error);
    void connectionError(const QString &error);

private slots:
    void onListReplyFinished();
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;
    QString m_host;
    int m_port;
    QString m_user;
    QString m_pass;
    QString m_currentPath;
    QString m_cachedResponse;
};

#endif // FTPCLIENT_H
