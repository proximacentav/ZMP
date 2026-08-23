#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <functional>

// Одноэкземплярный запуск: второй процесс передаёт сообщение первому
// (открытие файлов из файлового менеджера, play-плейлиста и т.п.)
class SingleInstance : public QObject
{
    Q_OBJECT
public:
    static QString serverName() { return "zmp_instance_v1"; }

    // Попытка отправить сообщение уже запущенному ZMP; true = отправлено,
    // новый экземпляр должен завершиться.
    static bool sendToRunning(const QByteArray &payload)
    {
        QLocalSocket sock;
        sock.connectToServer(serverName());
        if (!sock.waitForConnected(500))
            return false;
        sock.write(payload);
        sock.write("\n");
        sock.flush();
        sock.waitForBytesWritten(1000);
        sock.disconnectFromServer();
        return true;
    }

    explicit SingleInstance(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    bool listen(std::function<void(const QByteArray &)> handler)
    {
        m_handler = handler;
        QLocalServer::removeServer(serverName()); // убрать остатки после краша
        if (!m_server.listen(serverName()))
            return false;
        connect(&m_server, &QLocalServer::newConnection, this, [this]() {
            while (QLocalSocket *sock = m_server.nextPendingConnection()) {
                connect(sock, &QLocalSocket::disconnected, sock, &QLocalSocket::deleteLater);
                connect(sock, &QLocalSocket::readyRead, this, [this, sock]() {
                    m_buf.append(sock->readAll());
                    int nl;
                    while ((nl = m_buf.indexOf('\n')) >= 0) {
                        const QByteArray msg = m_buf.left(nl);
                        m_buf.remove(0, nl + 1);
                        if (m_handler)
                            m_handler(msg);
                    }
                });
            }
        });
        return true;
    }

private:
    QLocalServer m_server;
    QByteArray m_buf;
    std::function<void(const QByteArray &)> m_handler;
};

#endif // SINGLEINSTANCE_H
