#ifndef FILESWIDGET_H
#define FILESWIDGET_H


#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#error "install qt6"
#endif

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QPushButton>
#include <QLineEdit>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslError>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QTimer>
#include <QTcpSocket>
#include "ftpclient.h"

class FilesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FilesWidget(QWidget *parent = nullptr);
    QString currentSelectedFile() const;

signals:
    void fileSelected(const QString &path);

private slots:
    void onDoubleClicked(const QModelIndex &index);
    void onMenuButtonClicked();
    void onPathChanged(const QString &path);
    void onBrowseHome();
    void onBrowseRoot();
    void onConnectFTP();
    void onConnectFTPS();
    void onConnectSMB();
    void onDownloadFile(const QString &url, const QString &localPath);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);
    void onSslErrors(const QList<QSslError> &errors, QNetworkReply *reply);
    void onCertificateDialogFinished();
    void onConnectSMBFinished();
    void onIPValidationError(bool &continueConnection);
    void startPing(const QString &ip, int port);

    // Новая логика проверки соединения и обзора FTP
    void onPingSocketConnected();
    void onPingSocketError(QAbstractSocket::SocketError socketError);
    void onPingSocketTimeout();

private:
    // Верхняя панель (общая для обоих режимов)
    QPushButton *m_menuButton;
    QLineEdit  *m_pathEdit;
    QMenu      *m_menu;
    QProgressBar *m_progressBar;
    bool        m_isMenuOpen;
    QString     m_currentPath;

    // Стэк: страница локальных файлов / страница FTP
    QStackedWidget *m_stack;
    QWidget    *m_localViewPage;
    QWidget    *m_ftpViewPage;

    // Локальный вид
    QTreeView       *m_treeView;
    QFileSystemModel *m_model;

    // FTP-вид
    QListWidget *m_ftpListWidget;
    QLabel      *m_ftpStatusLabel;
    QPushButton *m_ftpBackButton;

    // Сеть
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentDownload;
    QNetworkReply *m_currentListReply;
    QString m_cacheDir;
    FTPClient *m_ftpClient;

    // Проверка TCP-доступности сервера (замена сломанного HTTP-пинга)
    QTcpSocket *m_pingSocket;
    QTimer     *m_pingTimeoutTimer;

    // Параметры текущего FTP-соединения
    QString m_ftpScheme;      // "ftp" (или "ftp" для FTPS — см. комментарий в onConnectFTPS)
    QString m_ftpHost;
    int     m_ftpPort;
    QString m_ftpUser;
    QString m_ftpPass;
    QString m_ftpCurrentDir;  // текущий путь на FTP, например "/"

    void createMenu();
    void setupUI();
    void createCacheDir();
    void downloadToCache(const QString &remotePath);
    void showCertificateDialog(const QSslCertificate &cert);
    void connectToSMB(const QString &host, int port);

    void testFtpConnectivity(const QString &scheme, const QString &host, int port,
                             const QString &user, const QString &pass);
    void startFtpListing(const QString &dir);
    QUrl buildFtpUrl(const QString &path) const;
    void parseFtpListing(const QString &listing);
    void switchToFtpView();
    void switchToLocalView();

    bool isValidIPv4(const QString &ip);
    bool isValidIPv6(const QString &ip);
};

#endif // FILESWIDGET_H
