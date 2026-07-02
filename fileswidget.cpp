#include "fileswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QStyleFactory>
#include <QStyleOptionViewItem>
#include <QPainter>
#include <QDir>
#include <QFileInfo>
#include <QStyledItemDelegate>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QSslCertificate>
#include <QSslError>
#include <QSslSocket>
#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>
#include <QLabel>
#include <QTimer>
#include <QRegularExpression>
#include <QFile>
#include <QUrlQuery>
#include <QSortFilterProxyModel>

class MusicHighlightDelegate : public QStyledItemDelegate
{
public:
    explicit MusicHighlightDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        QString filePath = index.data(QFileSystemModel::FilePathRole).toString();
        if (QFileInfo(filePath).isFile()) {
            QString suffix = QFileInfo(filePath).suffix().toLower();
            if (suffix == "mp3" || suffix == "wav" || suffix == "flac" || suffix == "ogg" || suffix == "m4a") {
                opt.palette.setColor(QPalette::Text, Qt::darkGreen);
                opt.palette.setColor(QPalette::Highlight, QColor(144, 238, 144));
            }
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

FilesWidget::FilesWidget(QWidget *parent)
    : QWidget(parent)
    , m_menuButton(nullptr)
    , m_pathEdit(nullptr)
    , m_searchEdit(nullptr)
    , m_proxyModel(nullptr)
    , m_menu(nullptr)
    , m_progressBar(nullptr)
    , m_isMenuOpen(false)
    , m_currentPath(QDir::homePath())
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentDownload(nullptr)
    , m_cacheDir(QDir::homePath() + "/zmp_music_cache")
    , m_pingSocket(new QTcpSocket(this))
    , m_pingTimeoutTimer(new QTimer(this))
{
    if (!QDir(m_cacheDir).exists()) {
        QDir().mkpath(m_cacheDir);
    }

    setupUI();
    createMenu();

    connect(m_networkManager, &QNetworkAccessManager::finished, this, &FilesWidget::onDownloadFinished);
    connect(m_pingSocket, &QTcpSocket::connected, this, &FilesWidget::onPingSocketConnected);
    connect(m_pingSocket, &QTcpSocket::errorOccurred, this, &FilesWidget::onPingSocketError);
    connect(m_pingTimeoutTimer, &QTimer::timeout, this, &FilesWidget::onPingSocketTimeout);
}

void FilesWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(5, 5, 5, 5);
    topLayout->setSpacing(5);

    m_menuButton = new QPushButton("=Меню", this);
    m_menuButton->setFixedWidth(100);
    connect(m_menuButton, &QPushButton::clicked, this, &FilesWidget::onMenuButtonClicked);
    topLayout->addWidget(m_menuButton);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Поиск по названию и расширению...");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &FilesWidget::onSearchTextChanged);
    topLayout->addWidget(m_searchEdit, 1);

    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setText(m_currentPath);
    connect(m_pathEdit, &QLineEdit::textChanged, this, &FilesWidget::onPathChanged);
    topLayout->addWidget(m_pathEdit, 1);

    mainLayout->addLayout(topLayout);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    m_treeView = new QTreeView(this);
    m_model = new QFileSystemModel(this);
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_model->setRootPath(QDir::rootPath());
    m_treeView->setModel(m_proxyModel);
    m_treeView->setRootIndex(m_proxyModel->mapFromSource(m_model->index(m_currentPath)));
    m_treeView->setHeaderHidden(false);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(20);
    m_treeView->setSortingEnabled(true);
    m_treeView->setItemDelegate(new MusicHighlightDelegate(this));

    mainLayout->addWidget(m_treeView);

    connect(m_treeView, &QTreeView::doubleClicked, this, &FilesWidget::onDoubleClicked);
}

void FilesWidget::createMenu()
{
    m_menu = new QMenu(this);

    QAction *homeAction = m_menu->addAction("~");
    connect(homeAction, &QAction::triggered, this, &FilesWidget::onBrowseHome);

    QAction *rootAction = m_menu->addAction("/");
    connect(rootAction, &QAction::triggered, this, &FilesWidget::onBrowseRoot);
}

void FilesWidget::onMenuButtonClicked()
{
    if (m_isMenuOpen) {
        m_menu->hide();
        m_isMenuOpen = false;
    } else {
        m_menu->popup(m_menuButton->mapToGlobal(QPoint(0, m_menuButton->height())));
        m_isMenuOpen = true;
    }
}

void FilesWidget::onPathChanged(const QString &path) // внимание FTP функционал был отключен из за большого количества багов возможно
// будет в новых версиях
{
    QFileInfo info(path);
    if (info.exists() && info.isDir()) {
        m_currentPath = path;
        m_treeView->setRootIndex(m_proxyModel->mapFromSource(m_model->index(m_currentPath)));
    }
}

void FilesWidget::onSearchTextChanged(const QString &text)
{
    QString searchText = text.trimmed();
    
    if (searchText.isEmpty()) {
        m_proxyModel->setFilterFixedString("");
    } else {
        m_proxyModel->setFilterFixedString(searchText);
    }
}

void FilesWidget::onBrowseHome()
{
    m_currentPath = QDir::homePath();
    m_pathEdit->setText(m_currentPath);
    m_treeView->setRootIndex(m_proxyModel->mapFromSource(m_model->index(m_currentPath)));
}

void FilesWidget::onBrowseRoot()
{
    m_currentPath = "/";
    m_pathEdit->setText(m_currentPath);
    m_treeView->setRootIndex(m_proxyModel->mapFromSource(m_model->index(m_currentPath)));
}

void FilesWidget::onDoubleClicked(const QModelIndex &index)
{
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    if (!m_model->isDir(sourceIndex)) {
        QString path = m_model->filePath(sourceIndex);
        emit fileSelected(path);
    }
}

QString FilesWidget::currentSelectedFile() const
{
    QModelIndex idx = m_treeView->currentIndex();
    if (idx.isValid()) {
        QModelIndex sourceIndex = m_proxyModel->mapToSource(idx);
        if (!m_model->isDir(sourceIndex))
            return m_model->filePath(sourceIndex);
    }
    return QString();
}

bool FilesWidget::isValidIPv4(const QString &ip)
{
    QRegularExpression regex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    return regex.match(ip).hasMatch();
}

bool FilesWidget::isValidIPv6(const QString &ip)
{
    QRegularExpression regex("^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$|^:(([0-9a-fA-F]{1,4}:){0,6}[0-9a-fA-F]{1,4})?$|^::([0-9a-fA-F]{1,4}:){0,5}[0-9a-fA-F]{1,4}$|^([0-9a-fA-F]{1,4}:){1,7}:$|^([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}$|^([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}$|^([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}$|^([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}$|^([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}$|^[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})$|^:((:[0-9a-fA-F]{1,4}){1,7}|:)$");
    return regex.match(ip).hasMatch();
}

void FilesWidget::onIPValidationError(bool &continueConnection)
{
    continueConnection = false;
    
    QDialog dialog(this);
    dialog.setWindowTitle("Неверный IP-адрес");
    dialog.setFixedSize(300, 120);
    
    QLabel *label = new QLabel("IP-адрес не действителен. Продолжить попытку подключения?", &dialog);
    label->setAlignment(Qt::AlignCenter);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("Отмена");
    QPushButton *continueBtn = new QPushButton("Продолжить");
    
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(continueBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(continueBtn);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->addWidget(label);
    layout->addLayout(buttonLayout);
    
    if (dialog.exec() == QDialog::Accepted) {
        continueConnection = true;
    }
}

void FilesWidget::startPing(const QString &ip, int port)
{
    if (!isValidIPv4(ip) && !isValidIPv6(ip)) {
        QMessageBox::warning(this, "Ошибка", "Неверный IP-адрес");
        return;
    }
    
    m_pingSocket->abort();
    m_pingSocket->connectToHost(ip, port);
    m_pingTimeoutTimer->start(1000);
}

void FilesWidget::onPingSocketConnected()
{
    m_pingSocket->disconnectFromHost();
    m_pingTimeoutTimer->stop();
    m_pingSocket->deleteLater();
}

void FilesWidget::onPingSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    m_pingTimeoutTimer->stop();
    m_pingSocket->deleteLater();
}

void FilesWidget::onPingSocketTimeout()
{
    m_pingSocket->abort();
    m_pingTimeoutTimer->stop();
    m_pingSocket->deleteLater();
    QMessageBox::critical(this, "Ошибка", "Сервер не доступен");
}

void FilesWidget::onDownloadFinished()
{
    if (m_currentDownload) {
        if (m_currentDownload->error() == QNetworkReply::NoError) {
            QByteArray data = m_currentDownload->readAll();
            QString fileName = m_currentDownload->url().fileName();
            if (fileName.isEmpty()) {
                fileName = "downloaded_file";
            }
            QString localPath = m_cacheDir + "/" + fileName;
            
            QFile file(localPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
                QMessageBox::information(this, "Скачивание", 
                    QString("Файл сохранен в %1").arg(localPath));
            } else {
                QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл");
            }
        } else {
            QMessageBox::critical(this, "Ошибка скачивания", 
                m_currentDownload->errorString());
        }
        
        m_currentDownload->deleteLater();
        m_currentDownload = nullptr;
        m_progressBar->setVisible(false);
    }
}

void FilesWidget::onDownloadFile(const QString &url, const QString &localPath)
{
    Q_UNUSED(url)
    Q_UNUSED(localPath)
}

void FilesWidget::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_UNUSED(bytesReceived)
    Q_UNUSED(bytesTotal)
}

void FilesWidget::onDownloadError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
    if (m_currentDownload) {
        QMessageBox::critical(this, "Ошибка", m_currentDownload->errorString());
        m_currentDownload->deleteLater();
        m_currentDownload = nullptr;
        m_progressBar->setVisible(false);
    }
}

void FilesWidget::onSslErrors(const QList<QSslError> &errors, QNetworkReply *reply)
{
    Q_UNUSED(errors)
    Q_UNUSED(reply)
}

void FilesWidget::onCertificateDialogFinished()
{
}

void FilesWidget::onConnectSMBFinished()
{
}

void FilesWidget::showCertificateDialog(const QSslCertificate &cert)
{
    Q_UNUSED(cert)
}
