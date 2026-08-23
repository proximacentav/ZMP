#include "ftpclient.h"
#include <QUrl>
#include <QDebug>
#include <QRegularExpression>
#include <QDir>
#include <QFileInfo>

static FileEntry parseUnixLsLine(const QString &line)
{    FileEntry entry;
    static QRegularExpression re(
        R"(^([\-dplbcws])([\-r][\-w][\-xsS\-]{0,2})([\-r][\-w][\-xsS\-]{0,2})([\-rwxXsStT\-]{1,3})\s+)"  // 1-4: perms
        R"((\d+)\s+(\S+)\s+(\S+)\s+(\d+)\s+)"  // 5-8: links, owner, group, size
        R"((\w{3}\s+\d{1,2}\s+(?:\d{1,2}:\d{2}|\d{4}))\s+(.+)$)"  // 9-10: date, name
    );

    auto match = re.match(line);
    if (match.hasMatch()) {
        entry.name = match.captured(10).trimmed();
        int arrow = entry.name.indexOf(" -> ");
        if (arrow != -1) entry.name = entry.name.left(arrow);
        entry.isDir = match.captured(1) == QLatin1Char('d');
        entry.size = match.captured(8).toLongLong();
        return entry;
    }

    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() >= 9) {
        entry.name = parts.last();
        entry.isDir = parts[0].startsWith('d');
        entry.size = parts[parts.size() - 5].toLongLong();
    }
    return entry;
}

// Пути в командах FTP (CWD/LIST/RETR) нужно заключать в кавычки,
// иначе пути с пробелами ломают разбор команды сервером.
static QString quoteFtpPath(const QString &path)
{
    QString p = path;
    p.replace('"', "\"\"");
    return "\"" + p + "\"";
}

FTPClient::FTPClient(QObject *parent)
    : QObject(parent)
    , m_control(nullptr)
    , m_data(nullptr)
    , m_smbProcess(nullptr)
    , m_protocol(None)
    , m_port(0)
    , m_currentPath("/")
    , m_busy(false)
    , m_waitingForGreeting(false)
    , m_downloadFile(nullptr)
    , m_downloadTotal(0)
    , m_downloadReceived(0)
    , m_pendingList(false)
    , m_pendingDownload(false)
    , m_dataSocketConnected(false)
    , m_ignoreSslErrors(false)
    , m_connectTimer(new QTimer(this))
    , m_dataTimer(new QTimer(this))
{
    m_connectTimer->setSingleShot(true);
    connect(m_connectTimer, &QTimer::timeout, this, [this]() {
        if (m_control && m_control->state() == QAbstractSocket::ConnectingState) {
            m_control->abort();
            emit error("Connection timeout");
        }
    });

    m_dataTimer->setSingleShot(true);
    connect(m_dataTimer, &QTimer::timeout, this, [this]() {
        if (m_data && m_data->state() == QAbstractSocket::ConnectingState) {
            m_data->abort();
            m_pendingList = false;
            m_pendingDownload = false;
            emit error("Data connection timeout");
        }
    });
}

FTPClient::~FTPClient()
{
    disconnect();
}

void FTPClient::disconnect()
{
    m_cmdQueue.clear();
    m_busy = false;
    m_waitingForGreeting = false;
    m_pendingList = false;
    m_pendingDownload = false;
    m_responseBuf.clear();
    m_dataBuf.clear();
    m_listBuffer.clear();
    m_contCode.clear();
    m_connectTimer->stop();
    m_dataTimer->stop();
    m_pendingDataCmd.clear();
    m_dataSocketConnected = false;
    m_ignoreSslErrors = false;

    if (m_downloadFile) {
        m_downloadFile->close();
        delete m_downloadFile;
        m_downloadFile = nullptr;
    }

    if (m_data) {
        m_data->abort();
        m_data->deleteLater();
        m_data = nullptr;
    }

    if (m_smbProcess) {
        m_smbProcess->kill();
        m_smbProcess->waitForFinished(1000);
        m_smbProcess->deleteLater();
        m_smbProcess = nullptr;
    }

    if (m_control) {
        m_control->abort();
        m_control->deleteLater();
        m_control = nullptr;
    }

    m_protocol = None;
}

bool FTPClient::isConnected() const
{
    if (m_protocol == SMB)
        return m_smbProcess && m_smbProcess->state() == QProcess::Running;
    return m_control && m_control->state() == QAbstractSocket::ConnectedState;
}

void FTPClient::connectToHost(const QString &host, quint16 port,
                               const QString &user, const QString &pass, Protocol proto)
{
    disconnect();

    m_host = host;
    m_port = port;
    m_user = user;
    m_pass = pass;
    m_protocol = proto;
    m_currentPath = "/";

    if (proto == SMB) {
        if (m_smbShare.isEmpty())
            m_smbShare = QString("//%1/share").arg(host);
        m_smbProcess = new QProcess(this);
        connect(m_smbProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &FTPClient::onSmbProcessFinished);
        connect(m_smbProcess, &QProcess::errorOccurred, this, &FTPClient::onSmbError);
        emit connected();
        return;
    }

    if (proto == FTPS) {
        m_control = new QSslSocket(this);
    } else {
        m_control = new QTcpSocket(this);
    }

    connect(m_control, &QAbstractSocket::connected, this, &FTPClient::onControlConnected);
    connect(m_control, &QAbstractSocket::disconnected, this, &FTPClient::onControlDisconnected);
    connect(m_control, &QAbstractSocket::readyRead, this, &FTPClient::onControlReadyRead);
    connect(m_control, &QAbstractSocket::errorOccurred,
            this, &FTPClient::onControlError);

    m_waitingForGreeting = true;
    m_connectTimer->start(10000);
    m_control->connectToHost(host, port);
}

void FTPClient::onControlConnected()
{
    m_connectTimer->stop();
}

void FTPClient::onControlError(QAbstractSocket::SocketError err)
{
    Q_UNUSED(err)
    m_connectTimer->stop();
    if (m_control)
        emit error(QString("Connection error: %1").arg(m_control->errorString()));
}

void FTPClient::onControlDisconnected()
{
    if (m_protocol != None) {
        emit disconnected();
    }
}

void FTPClient::onControlReadyRead()
{
    m_responseBuf.append(m_control->readAll());

    while (true) {
        int idx = m_responseBuf.indexOf("\r\n");
        if (idx == -1) break;

        QByteArray line = m_responseBuf.left(idx);
        m_responseBuf.remove(0, idx + 2);

        if (line.length() < 4) continue;

        QString text = QString::fromUtf8(line);

        // Многострочный ответ: пропускаем строки, пока не придёт
        // завершающая строка с тем же кодом и пробелом ("220 ...").
        if (!m_contCode.isEmpty()) {
            if (text.startsWith(m_contCode + QLatin1Char(' ')))
                m_contCode.clear();
            continue;
        }
        if (text.length() > 3 && text.at(3) == '-' && text.left(3).toInt() >= 100) {
            m_contCode = text.left(3);
            continue;
        }

        if (m_waitingForGreeting) {
            m_waitingForGreeting = false;
            if (m_protocol == FTPS) {
                QSslSocket *ssl = qobject_cast<QSslSocket*>(m_control);
                if (ssl) {
                    connect(ssl, &QSslSocket::sslErrors, this, [this, ssl](const QList<QSslError> &errors) {
                        emit sslErrorsOccurred(errors);
                        if (m_ignoreSslErrors) {
                            ssl->ignoreSslErrors();
                        }
                    });
                    connect(ssl, &QSslSocket::encrypted, this, [this]() {
                        sendQueued("USER " + m_user, "3", [this](const QString &resp) {
                            if (resp.startsWith("2")) {
                                emit connected();
                            } else if (resp.startsWith("3")) {
                                sendQueued("PASS " + m_pass, "2", [this](const QString &r) {
                                    if (r.startsWith("2")) emit connected();
                                    else emit error("Authentication failed");
                                });
                            } else {
                                emit error("Authentication failed");
                            }
                        });
                    });
                    sendQueued("AUTH TLS", "2", [this, ssl](const QString &) {
                        ssl->startClientEncryption();
                    });
                    continue;
                }
            }
            sendQueued("USER " + m_user, "3", [this](const QString &resp) {
                if (resp.startsWith("2")) {
                    emit connected();
                } else if (resp.startsWith("3")) {
                    sendQueued("PASS " + m_pass, "2", [this](const QString &r) {
                        if (r.startsWith("2")) emit connected();
                        else emit error("Authentication failed");
                    });
                } else {
                    emit error("Authentication failed");
                }
            });
            continue;
        }

        if (m_busy && !m_cmdQueue.isEmpty()) {
            QueuedCmd &qc = m_cmdQueue.head();
            if (qc.callback) {
                qc.callback(text);
            }
            m_cmdQueue.dequeue();
            if (m_cmdQueue.isEmpty()) {
                m_busy = false;
            } else {
                processQueue();
            }
        }
    }
}

void FTPClient::sendQueued(const QString &cmd, const QString &expectedPrefix,
                            std::function<void(const QString&)> cb)
{
    QueuedCmd qc;
    qc.cmd = cmd;
    qc.expectedPrefix = expectedPrefix;
    qc.callback = cb;
    m_cmdQueue.enqueue(qc);

    if (!m_busy) {
        processQueue();
    }
}

void FTPClient::processQueue()
{
    if (m_cmdQueue.isEmpty()) {
        m_busy = false;
        return;
    }

    m_busy = true;
    QueuedCmd qc = m_cmdQueue.head();

    if (!m_control || m_control->state() != QAbstractSocket::ConnectedState) {
        emit error("Not connected");
        m_cmdQueue.clear();
        m_busy = false;
        return;
    }

    m_control->write((qc.cmd + "\r\n").toUtf8());
}

void FTPClient::list(const QString &path)
{
    if (m_protocol == SMB) {
        smbListDir(path.isEmpty() ? m_currentPath : path);
        return;
    }

    m_pendingList = true;
    m_pendingDownload = false;
    m_listBuffer.clear();
    m_dataBuf.clear();

    QString target = path.isEmpty() ? m_currentPath : path;

    sendQueued("TYPE I", "2", [this, target](const QString &) {
        sendQueued("PASV", "2", [this, target](const QString &resp) {
            QString dataHost;
            quint16 dataPort;
            if (!parsePasvResponse(resp, dataHost, dataPort)) {
                emit error("Failed to parse PASV response");
                m_pendingList = false;
                return;
            }

            m_dataSocketConnected = false;
            m_pendingDataCmd = "LIST";
            if (!target.isEmpty() && target != "/") {
                m_pendingDataCmd += " " + quoteFtpPath(target);
            }

            m_data = new QTcpSocket(this);
            connect(m_data, &QTcpSocket::connected, this, &FTPClient::onDataConnected);
            connect(m_data, &QTcpSocket::readyRead, this, &FTPClient::onDataReadyRead);
            connect(m_data, &QTcpSocket::disconnected, this, &FTPClient::onDataFinished);
            connect(m_data, &QAbstractSocket::errorOccurred, this, &FTPClient::onDataError);

            m_listBuffer.clear();
            m_dataBuf.clear();
            m_dataTimer->start(10000);
            m_data->connectToHost(dataHost, dataPort);
        });
    });
}

void FTPClient::onDataConnected()
{
    m_dataTimer->stop();
    m_dataSocketConnected = true;

    if (!m_pendingDataCmd.isEmpty()) {
        QString cmd = m_pendingDataCmd;
        m_pendingDataCmd.clear();
        sendQueued(cmd, "1", [](const QString &) {});
    }
}

void FTPClient::onDataReadyRead()
{
    m_dataBuf.append(m_data->readAll());
}

void FTPClient::onDataError(QAbstractSocket::SocketError err)
{
    m_dataTimer->stop();
    if (err == QAbstractSocket::RemoteHostClosedError) {
        return;
    }
    m_pendingList = false;
    m_pendingDownload = false;
    if (m_data) {
        emit error(QString("Data connection: %1").arg(m_data->errorString()));
    }
}

void FTPClient::ignoreSslErrors()
{
    m_ignoreSslErrors = true;
}

void FTPClient::onDataFinished()
{
    m_dataTimer->stop();
    if (m_pendingList) {
        completeList();
    } else if (m_pendingDownload) {
        if (m_downloadFile) {
            m_downloadFile->write(m_dataBuf);
            m_downloadFile->close();
            delete m_downloadFile;
            m_downloadFile = nullptr;
        }
        m_pendingDownload = false;
        emit downloadFinished();
    }

    if (m_data) {
        m_data->deleteLater();
        m_data = nullptr;
    }
}

void FTPClient::completeList()
{
    m_pendingList = false;
    parseListResponse(QString::fromUtf8(m_dataBuf));
    emit listReceived(m_listBuffer);

    if (m_data) {
        m_data->deleteLater();
        m_data = nullptr;
    }
}

bool FTPClient::parsePasvResponse(const QString &response, QString &host, quint16 &port)
{
    QRegularExpression re(R"((\d+),(\d+),(\d+),(\d+),(\d+),(\d+))");
    auto match = re.match(response);
    if (!match.hasMatch()) return false;

    host = QString("%1.%2.%3.%4")
               .arg(match.captured(1))
               .arg(match.captured(2))
               .arg(match.captured(3))
               .arg(match.captured(4));
    port = match.captured(5).toUShort() * 256 + match.captured(6).toUShort();
    return true;
}

void FTPClient::parseListResponse(const QString &data)
{
    QStringList lines = data.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        FileEntry entry = parseUnixLsLine(trimmed);
        if (!entry.name.isEmpty() && entry.name != "." && entry.name != "..") {
            m_listBuffer.append(entry);
        }
    }
}

void FTPClient::download(const QString &remotePath, const QString &localPath)
{
    if (m_protocol == SMB) {
        smbDownload(remotePath, localPath);
        return;
    }
    ftpDownload(remotePath, localPath);
}

void FTPClient::ftpDownload(const QString &remote, const QString &local)
{
    m_pendingDownload = true;
    m_pendingRemotePath = remote;
    m_pendingLocalPath = local;
    m_downloadTotal = 0;
    m_downloadReceived = 0;
    m_dataBuf.clear();

    m_downloadFile = new QFile(local);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        emit error(QString("Cannot open local file: %1").arg(local));
        m_pendingDownload = false;
        delete m_downloadFile;
        m_downloadFile = nullptr;
        return;
    }

    sendQueued("TYPE I", "2", [this](const QString &) {
        sendQueued("PASV", "2", [this](const QString &resp) {
            QString dataHost;
            quint16 dataPort;
            if (!parsePasvResponse(resp, dataHost, dataPort)) {
                emit error("Failed to parse PASV response");
                m_pendingDownload = false;
                return;
            }

            m_dataSocketConnected = false;
            m_pendingDataCmd = "RETR " + quoteFtpPath(m_pendingRemotePath);

            m_data = new QTcpSocket(this);
            connect(m_data, &QTcpSocket::connected, this, &FTPClient::onDataConnected);
            connect(m_data, &QTcpSocket::readyRead, this, &FTPClient::onDataReadyRead);
            connect(m_data, &QTcpSocket::disconnected, this, &FTPClient::onDataFinished);
            connect(m_data, &QAbstractSocket::errorOccurred, this, &FTPClient::onDataError);

            m_dataBuf.clear();
            m_dataTimer->start(10000);
            m_data->connectToHost(dataHost, dataPort);
        });
    });
}

void FTPClient::cdUp()
{
    if (m_currentPath == "/") return;
    QFileInfo fi(m_currentPath);
    QString parent = fi.path();
    if (parent.isEmpty()) parent = "/";
    cd(parent);
}

void FTPClient::cd(const QString &path)
{
    if (m_protocol == SMB) {
        m_currentPath = path;
        emit pathChanged(m_currentPath);
        return;
    }

    QString target = path;
    if (target.isEmpty()) target = "/";

    QString fullPath = target;

    if (target.startsWith('/'))
        target = target.mid(1);
    if (target.isEmpty())
        target = "/";

    sendQueued("CWD " + quoteFtpPath(target), "2", [this, fullPath](const QString &resp) {
        // При ошибке CWD (например, 550) НЕ выполняем PWD — иначе получим
        // старую директорию и UI "перекинет" пользователя назад.
        if (!resp.startsWith("2")) {
            emit error(QString("Failed to change directory: %1").arg(resp));
            return;
        }
        sendQueued("PWD", "2", [this, fullPath](const QString &resp) {
            int q1 = resp.indexOf('"');
            int q2 = q1 >= 0 ? resp.indexOf('"', q1 + 1) : -1;
            if (q1 >= 0 && q2 > q1) {
                m_currentPath = resp.mid(q1 + 1, q2 - q1 - 1);
            } else {
                m_currentPath = fullPath;
            }
            emit pathChanged(m_currentPath);
        });
    });
}

void FTPClient::smbListDir(const QString &path)
{
    if (!m_smbProcess) {
        m_smbProcess = new QProcess(this);
        connect(m_smbProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &FTPClient::onSmbProcessFinished);
        connect(m_smbProcess, &QProcess::errorOccurred, this, &FTPClient::onSmbError);
    }

    if (m_smbProcess->state() == QProcess::Running) {
        m_smbProcess->kill();
        m_smbProcess->waitForFinished(500);
    }

    m_listBuffer.clear();
    m_currentPath = path;

    QStringList args;
    args << m_smbShare;
    if (!m_user.isEmpty()) {
        args << "-U" << (m_user + "%" + m_pass);
    }
    args << "-c" << ("cd \"" + path + "\"\nls");

    m_pendingList = true;
    m_smbProcess->start("smbclient", args);
}

void FTPClient::smbDownload(const QString &remote, const QString &local)
{
    if (!m_smbProcess) {
        m_smbProcess = new QProcess(this);
        connect(m_smbProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &FTPClient::onSmbProcessFinished);
        connect(m_smbProcess, &QProcess::errorOccurred, this, &FTPClient::onSmbError);
    }

    if (m_smbProcess->state() == QProcess::Running) {
        m_smbProcess->kill();
        m_smbProcess->waitForFinished(500);
    }

    m_pendingDownload = true;
    m_pendingRemotePath = remote;
    m_pendingLocalPath = local;

    QString remoteFull = m_currentPath + "/" + remote;
    while (remoteFull.contains("//")) remoteFull.replace("//", "/");

    QStringList args;
    args << m_smbShare;
    if (!m_user.isEmpty()) {
        args << "-U" << (m_user + "%" + m_pass);
    }
    args << "-c" << ("get \"" + remoteFull + "\" \"" + local + "\"");

    m_smbProcess->start("smbclient", args);
}

void FTPClient::onSmbProcessFinished()
{
    if (!m_smbProcess) return;

    QByteArray output = m_smbProcess->readAllStandardOutput();
    QByteArray errOutput = m_smbProcess->readAllStandardError();

    if (m_pendingDownload) {
        m_pendingDownload = false;
        QFileInfo fi(m_pendingLocalPath);
        if (fi.exists() && fi.size() > 0) {
            emit downloadFinished();
        } else if (!errOutput.isEmpty()) {
            emit error(QString("SMB: %1").arg(QString::fromUtf8(errOutput)));
        } else {
            emit error("SMB download failed");
        }
        return;
    }

    if (m_pendingList) {
        m_pendingList = false;

        QString text = QString::fromUtf8(output);
        QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        m_listBuffer.clear();

        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;
            if (trimmed.startsWith("Domain=") || trimmed.startsWith("session")) continue;
            if (trimmed.contains("blocks of size")) continue;
            if (trimmed.startsWith("\\")) continue;
            if (trimmed.startsWith("cd ") || trimmed.startsWith("ls")) continue;
            if ((trimmed == "." || trimmed == "..")) continue;

            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() < 2) continue;

            FileEntry entry;
            entry.name = parts.last();
            entry.isDir = parts[0].startsWith("D");
            if (parts.size() >= 4) {
                entry.size = parts[parts.size() - 3].toLongLong();
            }
            m_listBuffer.append(entry);
        }

        emit listReceived(m_listBuffer);
    }

    if (!errOutput.isEmpty() && !m_pendingDownload && !m_pendingList) {
        emit error(QString("SMB: %1").arg(QString::fromUtf8(errOutput)));
    }
}

void FTPClient::onSmbError(QProcess::ProcessError err)
{
    Q_UNUSED(err)
    if (m_smbProcess) {
        emit error(QString("SMB: %1").arg(m_smbProcess->errorString()));
    }
}
