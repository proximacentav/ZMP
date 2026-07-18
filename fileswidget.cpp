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
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QTextEdit>

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
    , m_searchVisible(false)
    , m_currentPath(QDir::homePath())
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentDownload(nullptr)
    , m_cacheDir(QDir::homePath() + "/zmp_music_cache")
    , m_pingSocket(new QTcpSocket(this))
    , m_pingTimeoutTimer(new QTimer(this))
    , m_ftpClient(new FTPClient(this))
    , m_ftpConnected(false)
    , m_ftpPort(21)
    , m_ftpScheme("ftp")
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

    connect(m_ftpClient, &FTPClient::connected, this, &FilesWidget::onFtpConnected);
    connect(m_ftpClient, &FTPClient::listReceived, this, &FilesWidget::onFtpListReceived);
    connect(m_ftpClient, &FTPClient::error, this, &FilesWidget::onFtpError);
    connect(m_ftpClient, &FTPClient::disconnected, this, &FilesWidget::onFtpDisconnected);
    connect(m_ftpClient, &FTPClient::downloadFinished, this, &FilesWidget::onFtpDownloadFinished);
    connect(m_ftpClient, &FTPClient::sslErrorsOccurred, this, &FilesWidget::onSslErrorsUi);
    connect(m_ftpClient, &FTPClient::pathChanged, this, [this](const QString &path) {
        QString protoName;
        switch (m_ftpClient->protocol()) {
            case FTPClient::FTP: protoName = "ftp"; break;
            case FTPClient::FTPS: protoName = "ftps"; break;
            case FTPClient::SMB: protoName = "smb"; break;
            default: protoName = "?"; break;
        }
        m_pathEdit->setText(QString("%1://%2:%3%4")
                                .arg(protoName).arg(m_ftpHost).arg(m_ftpPort).arg(path));
        m_ftpStatusLabel->setText(QString("... %1").arg(path));
        m_ftpListWidget->clear();
        m_ftpListWidget->addItem("Загрузка...");
        m_ftpClient->list(path);
    });
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
    m_searchEdit->setVisible(false);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &FilesWidget::onSearchTextChanged);
    topLayout->addWidget(m_searchEdit);

    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setText(m_currentPath);
    connect(m_pathEdit, &QLineEdit::textChanged, this, &FilesWidget::onPathChanged);
    topLayout->addWidget(m_pathEdit, 1);

    mainLayout->addLayout(topLayout);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    m_stack = new QStackedWidget(this);

    m_localViewPage = new QWidget();
    QVBoxLayout *localLayout = new QVBoxLayout(m_localViewPage);
    localLayout->setContentsMargins(0, 0, 0, 0);

    m_treeView = new QTreeView();
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
    localLayout->addWidget(m_treeView);
    connect(m_treeView, &QTreeView::doubleClicked, this, &FilesWidget::onDoubleClicked);

    m_ftpViewPage = new QWidget();
    QVBoxLayout *ftpLayout = new QVBoxLayout(m_ftpViewPage);
    ftpLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *ftpTop = new QHBoxLayout();
    m_ftpBackButton = new QPushButton("< Назад");
    m_ftpBackButton->setFixedWidth(100);
    connect(m_ftpBackButton, &QPushButton::clicked, this, &FilesWidget::onFtpBackClicked);
    ftpTop->addWidget(m_ftpBackButton);

    m_ftpStatusLabel = new QLabel("FTP");
    ftpTop->addWidget(m_ftpStatusLabel, 1);
    ftpLayout->addLayout(ftpTop);

    m_ftpListWidget = new QListWidget();
    m_ftpListWidget->setStyleSheet(
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:alternate { background: #f0f0f0; }"
    );
    connect(m_ftpListWidget, &QListWidget::itemDoubleClicked, this, &FilesWidget::onFtpListDoubleClicked);
    ftpLayout->addWidget(m_ftpListWidget);

    m_stack->addWidget(m_localViewPage);
    m_stack->addWidget(m_ftpViewPage);
    m_stack->setCurrentWidget(m_localViewPage);

    mainLayout->addWidget(m_stack, 1);
}

void FilesWidget::createMenu()
{
    m_menu = new QMenu(this);

    QAction *homeAction = m_menu->addAction("~");
    connect(homeAction, &QAction::triggered, this, &FilesWidget::onBrowseHome);

    QAction *rootAction = m_menu->addAction("/");
    connect(rootAction, &QAction::triggered, this, &FilesWidget::onBrowseRoot);

    m_menu->addSeparator();

    QAction *ftpAction = m_menu->addAction("FTP");
    connect(ftpAction, &QAction::triggered, this, &FilesWidget::onFtpConnect);

    QAction *ftpsAction = m_menu->addAction("FTPS");
    connect(ftpsAction, &QAction::triggered, this, &FilesWidget::onFtpsConnect);

    QAction *smbAction = m_menu->addAction("SMB");
    connect(smbAction, &QAction::triggered, this, &FilesWidget::onSmbConnect);

    if (m_ftpConnected) {
        m_menu->addSeparator();
        QAction *disconnectAction = m_menu->addAction("Отключиться от FTP/FTPS/SMB");
        connect(disconnectAction, &QAction::triggered, this, [this]() {
            if (m_ftpConnected) {
                m_ftpClient->disconnect();
                m_ftpConnected = false;
                switchToLocalView();
            }
        });
    }
}

void FilesWidget::onMenuButtonClicked()
{
    m_menu->clear();

    QAction *homeAction = m_menu->addAction("~");
    connect(homeAction, &QAction::triggered, this, &FilesWidget::onBrowseHome);

    QAction *rootAction = m_menu->addAction("/");
    connect(rootAction, &QAction::triggered, this, &FilesWidget::onBrowseRoot);

    m_menu->addSeparator();

    QAction *ftpAction = m_menu->addAction("FTP");
    connect(ftpAction, &QAction::triggered, this, &FilesWidget::onFtpConnect);

    QAction *ftpsAction = m_menu->addAction("FTPS");
    connect(ftpsAction, &QAction::triggered, this, &FilesWidget::onFtpsConnect);

    QAction *smbAction = m_menu->addAction("SMB");
    connect(smbAction, &QAction::triggered, this, &FilesWidget::onSmbConnect);

    m_menu->addSeparator();

    QAction *searchAction = m_menu->addAction(m_searchVisible ? "Скрыть поиск" : "Поиск");
    connect(searchAction, &QAction::triggered, this, &FilesWidget::onToggleSearch);

    if (m_ftpConnected) {
        m_menu->addSeparator();
        QAction *disconnectAction = m_menu->addAction("Отключиться от FTP/FTPS/SMB");
        connect(disconnectAction, &QAction::triggered, this, [this]() {
            if (m_ftpConnected) {
                m_ftpClient->disconnect();
                m_ftpConnected = false;
                switchToLocalView();
            }
        });
    }

    if (m_isMenuOpen) {
        m_menu->hide();
        m_isMenuOpen = false;
    } else {
        m_menu->popup(m_menuButton->mapToGlobal(QPoint(0, m_menuButton->height())));
        m_isMenuOpen = true;
    }
}

void FilesWidget::onPathChanged(const QString &path)
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

void FilesWidget::onToggleSearch()
{
    m_searchVisible = !m_searchVisible;
    m_searchEdit->setVisible(m_searchVisible);
    if (!m_searchVisible) {
        m_searchEdit->clear();
        m_proxyModel->setFilterFixedString("");
    }
}

void FilesWidget::onBrowseHome()
{
    if (m_ftpConnected) {
        m_ftpClient->disconnect();
        m_ftpConnected = false;
        switchToLocalView();
    }
    m_currentPath = QDir::homePath();
    m_pathEdit->setText(m_currentPath);
    m_treeView->setRootIndex(m_proxyModel->mapFromSource(m_model->index(m_currentPath)));
}

void FilesWidget::onBrowseRoot()
{
    if (m_ftpConnected) {
        m_ftpClient->disconnect();
        m_ftpConnected = false;
        switchToLocalView();
    }
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

void FilesWidget::showConnectionDialog(FTPClient::Protocol proto)
{
    if (m_ftpConnected) {
        m_ftpClient->disconnect();
        m_ftpConnected = false;
        switchToLocalView();
    }

    QDialog dialog(this);
    QString title;
    int defaultPort = 21;
    if (proto == FTPClient::FTPS) {
        title = "Подключение к FTPS";
        defaultPort = 990;
    } else if (proto == FTPClient::SMB) {
        title = "Подключение к SMB";
        defaultPort = 445;
    } else {
        title = "Подключение к FTP";
        defaultPort = 21;
    }
    dialog.setWindowTitle(title);
    dialog.setMinimumWidth(350);

    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *hostEdit = new QLineEdit();
    hostEdit->setPlaceholderText("example.com или IP");
    form->addRow("Хост:", hostEdit);

    QSpinBox *portSpin = new QSpinBox();
    portSpin->setRange(1, 65535);
    portSpin->setValue(defaultPort);
    form->addRow("Порт:", portSpin);

    QLineEdit *userEdit = new QLineEdit();
    userEdit->setPlaceholderText("anonymous");
    form->addRow("Пользователь:", userEdit);

    QLineEdit *passEdit = new QLineEdit();
    passEdit->setPlaceholderText("email@example.com");
    passEdit->setEchoMode(QLineEdit::Password);
    form->addRow("Пароль:", passEdit);

    QLineEdit *shareEdit = nullptr;
    if (proto == FTPClient::SMB) {
        shareEdit = new QLineEdit();
        shareEdit->setPlaceholderText("share_name");
        form->addRow("Ресурс (share):", shareEdit);
    }

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    m_ftpHost = hostEdit->text().trimmed();
    m_ftpPort = portSpin->value();
    m_ftpUser = userEdit->text().trimmed();
    if (m_ftpUser.isEmpty()) m_ftpUser = "anonymous";
    m_ftpPass = passEdit->text().trimmed();

    if (m_ftpHost.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Хост не может быть пустым");
        return;
    }

    if (proto == FTPClient::SMB && shareEdit) {
        QString share = shareEdit->text().trimmed();
        if (share.isEmpty()) share = "share";
        m_ftpClient->setSmbShare(share);
    }

    m_ftpStatusLabel->setText(QString("Подключение к %1:%2...").arg(m_ftpHost).arg(m_ftpPort));
    switchToFtpView();
    m_ftpClient->connectToHost(m_ftpHost, m_ftpPort, m_ftpUser, m_ftpPass, proto);
}

void FilesWidget::onFtpConnect()
{
    showConnectionDialog(FTPClient::FTP);
}

void FilesWidget::onFtpsConnect()
{
    showConnectionDialog(FTPClient::FTPS);
}

void FilesWidget::onSmbConnect()
{
    showConnectionDialog(FTPClient::SMB);
}

void FilesWidget::onFtpConnected()
{
    m_ftpConnected = true;
    QString protoName, protoPrefix;
    switch (m_ftpClient->protocol()) {
        case FTPClient::FTP: protoName = "FTP"; protoPrefix = "ftp"; break;
        case FTPClient::FTPS: protoName = "FTPS"; protoPrefix = "ftps"; break;
        case FTPClient::SMB: protoName = "SMB"; protoPrefix = "smb"; break;
        default: protoName = "?"; protoPrefix = "?"; break;
    }
    m_pathEdit->setText(QString("%1://%2:%3/")
                            .arg(protoPrefix).arg(m_ftpHost).arg(m_ftpPort));
    m_ftpStatusLabel->setText(QString("%1: %2:%3 - /")
                                  .arg(protoName)
                                  .arg(m_ftpHost)
                                  .arg(m_ftpPort));
    m_ftpListWidget->clear();
    m_ftpListWidget->addItem("Загрузка списка файлов...");
    m_ftpClient->list("/");
}

void FilesWidget::onFtpError(const QString &error)
{
    m_ftpStatusLabel->setText("Ошибка: " + error);
    if (!m_ftpConnected) {
        QMessageBox::critical(this, "Ошибка подключения", error);
        switchToLocalView();
    } else {
        QMessageBox::warning(this, "Ошибка", error);
    }
}

void FilesWidget::onFtpDisconnected()
{
    m_ftpConnected = false;
    m_ftpStatusLabel->setText("Отключено");
    m_pathEdit->setText(QDir::homePath());
}

void FilesWidget::onFtpListReceived(const QList<FileEntry> &list)
{
    m_ftpListWidget->clear();
    if (list.isEmpty()) {
        m_ftpListWidget->addItem("(пусто)");
        return;
    }

    for (const FileEntry &entry : list) {
        QString display;
        if (entry.isDir) {
            display = "📁 " + entry.name + "/";
        } else {
            QString sizeStr;
            qint64 sz = entry.size;
            if (sz < 1024)
                sizeStr = QString("%1 B").arg(sz);
            else if (sz < 1024 * 1024)
                sizeStr = QString("%1 KB").arg(sz / 1024);
            else
                sizeStr = QString("%1 MB").arg(sz / (1024 * 1024));
            display = "📄 " + entry.name + "  (" + sizeStr + ")";
        }

        QListWidgetItem *item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, entry.name);
        item->setData(Qt::UserRole + 1, entry.isDir);
        item->setData(Qt::UserRole + 2, entry.size);

        if (!entry.isDir) {
            QString suffix = QFileInfo(entry.name).suffix().toLower();
            if (suffix == "mp3" || suffix == "wav" || suffix == "flac" || suffix == "ogg" || suffix == "m4a") {
                item->setForeground(Qt::darkGreen);
            }
        }
        m_ftpListWidget->addItem(item);
    }
}

void FilesWidget::onFtpListDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;

    QString name = item->data(Qt::UserRole).toString();
    bool isDir = item->data(Qt::UserRole + 1).toBool();

    if (isDir) {
        QString newPath = m_ftpClient->currentPath() + "/" + name;
        while (newPath.contains("//")) newPath.replace("//", "/");
        m_ftpClient->cd(newPath);
        return;
    }

    QString suffix = QFileInfo(name).suffix().toLower();
    bool isAudio = (suffix == "mp3" || suffix == "wav" || suffix == "flac" || suffix == "ogg" || suffix == "m4a");

    if (!isAudio) {
        downloadToCache(name);
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Сохранение файла");
    msgBox.setText("Куда сохранить файл \"" + name + "\"?");
    QPushButton *playlistBtn = msgBox.addButton("Плейлист", QMessageBox::ActionRole);
    QPushButton *dirBtn = msgBox.addButton("Директория", QMessageBox::ActionRole);
    msgBox.addButton("Отмена", QMessageBox::RejectRole);
    msgBox.setDefaultButton(static_cast<QPushButton*>(msgBox.buttons().last()));
    msgBox.exec();

    if (msgBox.clickedButton() == playlistBtn) {
        showPlaylistSaveDialog(name);
    } else if (msgBox.clickedButton() == dirBtn) {
        showDirectorySaveDialog(name);
    }
}

void FilesWidget::onFtpBackClicked()
{
    QString cur = m_ftpClient->currentPath();
    if (cur == "/") {
        m_ftpClient->disconnect();
        m_ftpConnected = false;
        switchToLocalView();
        return;
    }
    m_ftpClient->cdUp();
}

void FilesWidget::downloadToCache(const QString &remotePath)
{
    m_pendingLocalPath.clear();
    QString localPath = m_cacheDir + "/" + remotePath;
    m_pendingRemotePath = remotePath;
    QString protoName;
    switch (m_ftpClient->protocol()) {
        case FTPClient::FTP: protoName = "FTP"; break;
        case FTPClient::FTPS: protoName = "FTPS"; break;
        case FTPClient::SMB: protoName = "SMB"; break;
        default: protoName = "?"; break;
    }
    m_ftpStatusLabel->setText(QString("Скачивание %1: %2...").arg(protoName).arg(remotePath));
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);
    m_ftpClient->download(remotePath, localPath);
}

void FilesWidget::onFtpDownloadFinished()
{
    m_progressBar->setVisible(false);
    QString protoName;
    switch (m_ftpClient->protocol()) {
        case FTPClient::FTP: protoName = "FTP"; break;
        case FTPClient::FTPS: protoName = "FTPS"; break;
        case FTPClient::SMB: protoName = "SMB"; break;
        default: protoName = "?"; break;
    }
    m_ftpStatusLabel->setText(QString("%1: %2:%3 - %4")
                                  .arg(protoName)
                                  .arg(m_ftpHost)
                                  .arg(m_ftpPort)
                                  .arg(m_ftpClient->currentPath()));

    if (m_pendingRemotePath.isEmpty()) return;
    QString localPath = m_pendingLocalPath.isEmpty()
        ? m_cacheDir + "/" + m_pendingRemotePath
        : m_pendingLocalPath;
    if (QFileInfo::exists(localPath)) {
        emit fileSelected(localPath);
    }
}

void FilesWidget::showPlaylistSaveDialog(const QString &fileName)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Выберите плейлист");
    dialog.setMinimumSize(300, 350);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *label = new QLabel("Выберите плейлист для сохранения:");
    layout->addWidget(label);

    QListWidget *listWidget = new QListWidget;
    QString basePath = QDir::homePath() + "/zmp_playlists";
    QDir dir(basePath);
    QStringList playlists = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &p : playlists) {
        if (p != "featured" && p != ".backup") {
            listWidget->addItem(p);
        }
    }
    layout->addWidget(listWidget);

    QPushButton *cancelBtn = new QPushButton("Отмена");
    layout->addWidget(cancelBtn);

    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(listWidget, &QListWidget::itemDoubleClicked, this, [this, fileName, basePath, &dialog](QListWidgetItem *item) {
        QString playlistName = item->text();
        QString savePath = basePath + "/" + playlistName + "/" + fileName;
        m_pendingLocalPath = savePath;
        m_pendingRemotePath = fileName;
        QString protoName;
        switch (m_ftpClient->protocol()) {
            case FTPClient::FTP: protoName = "FTP"; break;
            case FTPClient::FTPS: protoName = "FTPS"; break;
            case FTPClient::SMB: protoName = "SMB"; break;
            default: protoName = "?"; break;
        }
        m_ftpStatusLabel->setText(QString("Скачивание %1: %2...").arg(protoName).arg(fileName));
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 0);
        m_ftpClient->download(fileName, savePath);
        dialog.accept();
    });

    dialog.exec();
}

void FilesWidget::showDirectorySaveDialog(const QString &fileName)
{
    QString dirPath = QFileDialog::getExistingDirectory(this,
        "Выберите папку для сохранения", QDir::homePath());
    if (dirPath.isEmpty()) return;

    QString savePath = dirPath + "/" + fileName;
    m_pendingLocalPath = savePath;
    m_pendingRemotePath = fileName;
    QString protoName;
    switch (m_ftpClient->protocol()) {
        case FTPClient::FTP: protoName = "FTP"; break;
        case FTPClient::FTPS: protoName = "FTPS"; break;
        case FTPClient::SMB: protoName = "SMB"; break;
        default: protoName = "?"; break;
    }
    m_ftpStatusLabel->setText(QString("Скачивание %1: %2...").arg(protoName).arg(fileName));
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);
    m_ftpClient->download(fileName, savePath);
}

void FilesWidget::onSslErrorsUi(const QList<QSslError> &errors)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Ошибка SSL сертификата");
    dialog.setMinimumSize(450, 350);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *titleLabel = new QLabel("Сертификат сервера не действителен:");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(titleLabel);

    QString info;
    for (const QSslError &err : errors) {
        info += "- " + err.errorString() + "\n";
        QSslCertificate cert = err.certificate();
        if (!cert.isNull()) {
            info += "\n  Издатель: " + cert.issuerInfo(QSslCertificate::CommonName).join(", ") + "\n";
            info += "  Владелец: " + cert.subjectInfo(QSslCertificate::CommonName).join(", ") + "\n";
            info += "  Срок действия: " + cert.effectiveDate().toString("dd.MM.yyyy")
                    + " - " + cert.expiryDate().toString("dd.MM.yyyy") + "\n";
        }
        info += "\n";
    }

    QTextEdit *infoEdit = new QTextEdit();
    infoEdit->setPlainText(info);
    infoEdit->setReadOnly(true);
    layout->addWidget(infoEdit);

    QHBoxLayout *btnLayout = new QHBoxLayout();

    QPushButton *allowBtn = new QPushButton("Разрешить");
    QPushButton *denyBtn = new QPushButton("Запретить");
    btnLayout->addWidget(allowBtn);
    btnLayout->addWidget(denyBtn);

    layout->addLayout(btnLayout);

    connect(denyBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(allowBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted) {
        m_ftpClient->ignoreSslErrors();
    } else {
        m_ftpClient->disconnect();
        m_ftpConnected = false;
        switchToLocalView();
    }
}

void FilesWidget::switchToFtpView()
{
    m_stack->setCurrentWidget(m_ftpViewPage);
}

void FilesWidget::switchToLocalView()
{
    m_stack->setCurrentWidget(m_localViewPage);
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
    m_pingSocket = new QTcpSocket(this);
    connect(m_pingSocket, &QTcpSocket::connected, this, &FilesWidget::onPingSocketConnected);
    connect(m_pingSocket, &QTcpSocket::errorOccurred, this, &FilesWidget::onPingSocketError);
}

void FilesWidget::onPingSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    m_pingTimeoutTimer->stop();
    m_pingSocket->deleteLater();
    m_pingSocket = new QTcpSocket(this);
    connect(m_pingSocket, &QTcpSocket::connected, this, &FilesWidget::onPingSocketConnected);
    connect(m_pingSocket, &QTcpSocket::errorOccurred, this, &FilesWidget::onPingSocketError);
}

void FilesWidget::onPingSocketTimeout()
{
    m_pingSocket->abort();
    m_pingTimeoutTimer->stop();
    m_pingSocket->deleteLater();
    m_pingSocket = new QTcpSocket(this);
    connect(m_pingSocket, &QTcpSocket::connected, this, &FilesWidget::onPingSocketConnected);
    connect(m_pingSocket, &QTcpSocket::errorOccurred, this, &FilesWidget::onPingSocketError);
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

void FilesWidget::testFtpConnectivity(const QString &scheme, const QString &host, int port,
                                       const QString &user, const QString &pass)
{
    Q_UNUSED(scheme)
    m_ftpHost = host;
    m_ftpPort = port;
    m_ftpUser = user;
    m_ftpPass = pass;
    showConnectionDialog(FTPClient::FTP);
}

void FilesWidget::startFtpListing(const QString &dir)
{
    m_ftpClient->list(dir);
}

QUrl FilesWidget::buildFtpUrl(const QString &path) const
{
    QUrl url;
    url.setScheme(m_ftpScheme);
    url.setHost(m_ftpHost);
    url.setPort(m_ftpPort);
    url.setUserName(m_ftpUser);
    url.setPassword(m_ftpPass);
    url.setPath(path);
    return url;
}

void FilesWidget::parseFtpListing(const QString &listing)
{
    Q_UNUSED(listing)
}

void FilesWidget::connectToSMB(const QString &host, int port)
{
    m_ftpHost = host;
    m_ftpPort = port;
    m_ftpUser = "guest";
    m_ftpPass = "";
    showConnectionDialog(FTPClient::SMB);
}

void FilesWidget::createCacheDir()
{
    if (!QDir(m_cacheDir).exists()) {
        QDir().mkpath(m_cacheDir);
    }
}
