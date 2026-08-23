#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QObject>
#include <QString>
#include <QList>
#include <QTcpSocket>
#include <QSslSocket>
#include <QSslError>
#include <QProcess>
#include <QQueue>
#include <QFile>
#include <QTimer>
#include <functional>

struct FileEntry {
    QString name;
    bool isDir = false;
    qint64 size = 0;
};

class FTPClient : public QObject
{
    Q_OBJECT

public:
    enum Protocol { None, FTP, FTPS, SMB };

    explicit FTPClient(QObject *parent = nullptr);
    ~FTPClient();

    void connectToHost(const QString &host, quint16 port, const QString &user, const QString &pass, Protocol proto);
    void list(const QString &path = QString());
    void download(const QString &remotePath, const QString &localPath);
    void cdUp();
    void cd(const QString &path);
    void disconnect();
    void setSmbShare(const QString &share) { m_smbShare = share; }
    void ignoreSslErrors();

    Protocol protocol() const { return m_protocol; }
    QString currentPath() const { return m_currentPath; }
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void listReceived(const QList<FileEntry> &list);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    void error(const QString &error);
    void pathChanged(const QString &path);
    void sslErrorsOccurred(const QList<QSslError> &errors);

private slots:
    void onControlConnected();
    void onControlReadyRead();
    void onControlDisconnected();
    void onControlError(QAbstractSocket::SocketError error);
    void onDataConnected();
    void onDataReadyRead();
    void onDataFinished();
    void onDataError(QAbstractSocket::SocketError err);
    void onSmbProcessFinished();
    void onSmbError(QProcess::ProcessError err);

private:
    struct QueuedCmd {
        QString cmd;
        QString expectedPrefix;
        std::function<void(const QString&)> callback;
    };

    void sendQueued(const QString &cmd, const QString &expectedPrefix,
                    std::function<void(const QString&)> cb = nullptr);
    void processQueue();
    void startDataConnection();
    void completeList();
    void smbListDir(const QString &path);
    void smbDownload(const QString &remote, const QString &local);
    void ftpDownload(const QString &remote, const QString &local);
    bool parsePasvResponse(const QString &response, QString &host, quint16 &port);
    void parseListResponse(const QString &data);

    QAbstractSocket *m_control;
    QTcpSocket *m_data;
    QProcess *m_smbProcess;

    Protocol m_protocol;
    QString m_host;
    quint16 m_port;
    QString m_user;
    QString m_pass;
    QString m_currentPath;
    QString m_smbShare;

    QQueue<QueuedCmd> m_cmdQueue;
    bool m_busy;
    bool m_waitingForGreeting;
    QByteArray m_responseBuf;
    QByteArray m_dataBuf;
    QFile *m_downloadFile;
    qint64 m_downloadTotal;
    qint64 m_downloadReceived;
    bool m_pendingList;
    bool m_pendingDownload;
    bool m_dataSocketConnected;
    QString m_pendingDataCmd;
    QString m_pendingRemotePath;
    QString m_pendingLocalPath;
    QList<FileEntry> m_listBuffer;
    QTimer *m_connectTimer;
    QTimer *m_dataTimer;
    bool m_ignoreSslErrors;
    QString m_contCode;   // код текущего многострочного ответа ("220" для "220-...")
};

#endif
