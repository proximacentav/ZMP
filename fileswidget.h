#ifndef FILESWIDGET_H
#define FILESWIDGET_H

#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#error "install qt6"
#endif

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QPushButton>
#include <QLineEdit>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslError>
#include <QSslCertificate>
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
#include "translator.h"

class PartitionsDialog;

class FilesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FilesWidget(QWidget *parent = nullptr);
    QString currentSelectedFile() const;
    void setRootPassword(const QString &password);
    bool hasRootAccess() const { return m_hasRootAccess; }
    QString rootPassword() const { return m_rootPassword; }
    void disconnectFromServer();

signals:
    void fileSelected(const QString &path);

private slots:
    void onDoubleClicked(const QModelIndex &index);
    void onMenuButtonClicked();
    void onPathChanged(const QString &path);
    void onSearchTextChanged(const QString &text);
    void onBrowseHome();
    void onBrowseRoot();
    void onDownloadFile(const QString &url, const QString &localPath);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);
    void onSslErrors(const QList<QSslError> &errors, QNetworkReply *reply);
    void onCertificateDialogFinished();
    void onConnectSMBFinished();
    void onIPValidationError(bool &continueConnection);
    void startPing(const QString &ip, int port);
    void onPingSocketConnected();
    void onPingSocketError(QAbstractSocket::SocketError socketError);
    void onPingSocketTimeout();

    void onFtpConnect();
    void onFtpsConnect();
    void onSmbConnect();
    void onToggleSearch();
    void onShowPartitions();
    void openLocalPath(const QString &path);
    void onFtpListReceived(const QList<FileEntry> &list);
    void onFtpConnected();
    void onFtpError(const QString &error);
    void onFtpDisconnected();
    void onFtpListDoubleClicked(QListWidgetItem *item);
    void onFtpDownloadFinished();
    void onFtpBackClicked();
    void onRootListDoubleClicked(QListWidgetItem *item);
    void onRootBackClicked();
    void onBrowseRootSearch();
    void switchToRootView(const QString &path);
    void showPlaylistSaveDialog(const QString &fileName);
    void showDirectorySaveDialog(const QString &fileName);
    void onSslErrorsUi(const QList<QSslError> &errors);

private:
    QPushButton *m_menuButton;
    QLineEdit  *m_pathEdit;
    QLineEdit  *m_searchEdit;
    QMenu      *m_menu;
    QProgressBar *m_progressBar;
    bool        m_isMenuOpen;
    bool        m_searchVisible;
    QString     m_currentPath;

    QStackedWidget *m_stack;
    QWidget    *m_localViewPage;
    QWidget    *m_ftpViewPage;

    QTreeView       *m_treeView;
    QFileSystemModel *m_model;
    QSortFilterProxyModel *m_proxyModel;

    QListWidget *m_ftpListWidget;
    QLabel      *m_ftpStatusLabel;
    QPushButton *m_ftpBackButton;

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentDownload;
    QNetworkReply *m_currentListReply;
    QString m_cacheDir;

    QTcpSocket *m_pingSocket;
    QTimer     *m_pingTimeoutTimer;

    FTPClient  *m_ftpClient;
    bool        m_ftpConnected;
    bool        m_hasRootAccess = false;
    QString     m_rootPassword;
    QString     m_rootCurrentPath;

    QWidget    *m_rootViewPage;
    QLabel     *m_rootPathLabel;
    QPushButton *m_rootBackButton;
    QListWidget *m_rootListWidget;
    QLineEdit  *m_rootPathEdit;
    QString     m_ftpScheme;
    QString     m_ftpHost;
    int         m_ftpPort;
    QString     m_ftpUser;
    QString     m_ftpPass;
    QString     m_pendingRemotePath;
    QString     m_pendingLocalPath;

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
    void showConnectionDialog(FTPClient::Protocol proto);

    bool isValidIPv4(const QString &ip);
    bool isValidIPv6(const QString &ip);

    RetransList m_retrans;
    void retranslateUi();
};

#endif
