#include "mainwindow.h"
#include "mpriscontroller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QInputDialog>
#include <QEvent>
#include <QKeyEvent>
#include <QListWidget>
#include <QProcess>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QImage>
#include <QPixmap>
#include <QFile>
#include <QTextEdit>
#include <QTextStream>
#include <QFileDialog>
#include <QDateTime>
#include <QDir>
#include <unistd.h>
#include <pwd.h>
#include <crypt.h>
#include <fcntl.h>
#include <QSocketNotifier>

static MainWindow *g_mainWindow = nullptr;
static int g_original_stderr = -1;

static void qtMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(ctx)
    if (g_mainWindow) {
        QString prefix;
        switch (type) {
            case QtDebugMsg: prefix = "DEBUG"; break;
            case QtWarningMsg: prefix = "WARN"; break;
            case QtCriticalMsg: prefix = "ERROR"; break;
            case QtFatalMsg: prefix = "FATAL"; break;
            case QtInfoMsg: prefix = "INFO"; break;
        }
        g_mainWindow->addLog(QString("[%1] %2").arg(prefix, msg));
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_menuIndicator(nullptr), m_menuIndicatorY(0), m_menuIndicatorTargetY(0)
{
    setWindowTitle("Медиаплеер");
    resize(1200, 800);

    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    setCentralWidget(central);

    QWidget *menuContainer = new QWidget(this);
    menuContainer->setFixedWidth(200);
    menuContainer->setStyleSheet("background-color: #2b2b2b;");
    QVBoxLayout *menuContLayout = new QVBoxLayout(menuContainer);
    menuContLayout->setContentsMargins(5, 10, 5, 10);

    m_menu = new QListWidget;
    m_menu->addItems({"Устройства", "Плеер", "Плейлисты", "Файлы", "Эквалайзер", "Визуализация", "Параметры"});
    m_menu->setCurrentRow(0);

    m_menu->setSpacing(10);
    m_menu->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { background: transparent; color: white; padding: 12px; border-radius: 8px; }"
        "QListWidget::item:selected { background: transparent; color: white; }"
    );
    menuContLayout->addWidget(m_menu, 1);

    m_userButton = new QPushButton(getCurrentUsername());
    m_userButton->setCursor(Qt::PointingHandCursor);
    updateUserButtonStyle();
    menuContLayout->addWidget(m_userButton);

    connect(m_userButton, &QPushButton::clicked, this, &MainWindow::onUserButtonClicked);

    m_logTimer = new QTimer(this);
    m_logTimer->start(10000);
    connect(m_logTimer, &QTimer::timeout, this, &MainWindow::autoSaveLogs);

    g_mainWindow = this;
    qInstallMessageHandler(qtMessageHandler);

    int pipefd[2];
    pipe(pipefd);
    g_original_stderr = dup(STDERR_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    ::close(pipefd[1]);
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

    QSocketNotifier *stderrNotifier = new QSocketNotifier(pipefd[0], QSocketNotifier::Read, this);
    connect(stderrNotifier, &QSocketNotifier::activated, this, [this](int fd) {
        char buf[4096];
        int n;
        while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = 0;
            QString msg = QString::fromUtf8(buf).trimmed();
            if (!msg.isEmpty())
                addLog(msg);
        }
    });

    addLog("Программа запущена");

    m_menuIndicator = new QWidget(menuContainer);
    m_menuIndicator->setFixedHeight(40);
    m_menuIndicator->setGeometry(5, 0, 190, 40);
    m_menuIndicator->setStyleSheet(
        "background-color: rgba(183, 183, 183, 200);"
        "border-radius: 8px;"
    );
    m_menuIndicator->raise();

    m_menuAnimTimer = new QTimer(this);
    connect(m_menuAnimTimer, &QTimer::timeout, this, &MainWindow::animateMenu);

    if (m_menu->item(0)) {
        m_menuIndicatorY = m_menu->visualItemRect(m_menu->item(0)).y();
        m_menuIndicatorTargetY = m_menuIndicatorY;
        m_menuIndicator->move(5, m_menuIndicatorY);
    }

    mainLayout->addWidget(menuContainer);

    QWidget *rightContainer = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_audioManager = new AudioManager(this);
    m_devicesWidget = new DevicesWidget(this);
    m_playerWidget = new PlayerWidget(m_audioManager, this);
    m_playlistsWidget = new PlaylistsWidget(this);
    m_filesWidget = new FilesWidget(this);
    m_equalizerWidget = new EqualizerWidget(m_audioManager, this);
    m_visualizationWidget = new VisualizationWidget(m_audioManager, this);
    m_settingsWidget = new SettingsWidget(this);

    m_miniPlayerBar = new MiniPlayerBar(m_audioManager, this);
    rightLayout->addWidget(m_miniPlayerBar);

    m_stack = new QStackedWidget;
    rightLayout->addWidget(m_stack, 1);
    mainLayout->addWidget(rightContainer, 1);

    connect(m_playerWidget, &PlayerWidget::stateChanged, this, [this](bool playing) {
        if (playing) {
            const QStringList &pl = m_playerWidget->getCurrentPlaylist();
            if (!pl.isEmpty()) {
                m_playlistsWidget->onPlaylistPlaying(pl);
            }
        } else {
            m_playlistsWidget->onPlaylistStopped();
        }
    });

    connect(m_playerWidget, &PlayerWidget::featuredUpdated, this, &MainWindow::onFeaturedUpdated);

    connect(m_filesWidget, &FilesWidget::fileSelected, this, [this](const QString &path) {
        m_playerWidget->setPlaylist({path});
        m_playerWidget->setCurrentPlaylist({path});
        m_playerWidget->onPlay();
        m_menu->setCurrentRow(1);
    });

    m_stack->addWidget(m_devicesWidget);
    m_stack->addWidget(m_playerWidget);
    m_stack->addWidget(m_playlistsWidget);
    m_stack->addWidget(m_filesWidget);
    m_stack->addWidget(m_equalizerWidget);
    m_stack->addWidget(m_visualizationWidget);
    m_stack->addWidget(m_settingsWidget);

    connect(m_menu, &QListWidget::currentRowChanged, this, [this](int row) {
        QListWidgetItem *item = m_menu->item(row);
        if (item) {
            m_menuIndicatorTargetY = m_menu->visualItemRect(item).y();
            if (!m_menuAnimTimer->isActive()) m_menuAnimTimer->start(16);
        }
        m_stack->setCurrentIndex(row);
        m_miniPlayerBar->setVisible(row != 1);
    });

    connect(m_devicesWidget, &DevicesWidget::deviceChanged, this, &MainWindow::onDeviceChanged);
    connect(m_playlistsWidget, &PlaylistsWidget::playlistSelected, this, [this](const QStringList &tracks) {
        m_playerWidget->setPlaylist(tracks);
        m_playerWidget->setCurrentPlaylist(tracks);
        m_playerWidget->onPlay();
        m_menu->setCurrentRow(1);
    });
    connect(m_playerWidget, &PlayerWidget::currentPlaylistChanged, this, [this](const QStringList &tracks) {
        m_playlistsWidget->onPlaylistPlaying(tracks);
    });

    connect(m_settingsWidget, &SettingsWidget::exitRequested, this, &MainWindow::onExit);
    connect(m_settingsWidget, &SettingsWidget::metadataHeightChanged, m_playerWidget, &PlayerWidget::setMetadataHeight);
    connect(m_settingsWidget, &SettingsWidget::iconSizeChanged, m_playerWidget, &PlayerWidget::setIconSize);
    connect(m_settingsWidget, &SettingsWidget::accentColorChanged, m_playerWidget, &PlayerWidget::setAccentColor);
    connect(m_settingsWidget, &SettingsWidget::accentColorChanged, m_miniPlayerBar, &MiniPlayerBar::setAccentColor);
    connect(m_settingsWidget, &SettingsWidget::powerModeChanged, m_equalizerWidget, &EqualizerWidget::setPowerMode);
    connect(m_settingsWidget, &SettingsWidget::spectrumGainChanged, m_audioManager, &AudioManager::setSpectrumGain);
    connect(m_settingsWidget, &SettingsWidget::spectrumFpsChanged, m_audioManager, &AudioManager::setSpectrumFps);
    connect(m_settingsWidget, &SettingsWidget::spectrumBandsChanged, m_audioManager, &AudioManager::setSpectrumBands);
    connect(m_settingsWidget, &SettingsWidget::projectMPresetSelected, m_visualizationWidget, &VisualizationWidget::loadProjectMPreset);
    connect(m_settingsWidget, &SettingsWidget::maxBitrateChanged, m_audioManager, &AudioManager::setMaxBitrate);
    connect(m_audioManager, &AudioManager::spectrumDataChanged, m_playerWidget, &PlayerWidget::updateSpectrum);
    connect(m_audioManager, &AudioManager::spectrumDataChanged, m_visualizationWidget, &VisualizationWidget::updateSpectrum);

    connect(m_audioManager, &AudioManager::positionChanged, m_miniPlayerBar, &MiniPlayerBar::onPositionChanged);
    connect(m_audioManager, &AudioManager::durationChanged, m_miniPlayerBar, &MiniPlayerBar::onDurationChanged);
    connect(m_audioManager, &AudioManager::stateChanged, m_miniPlayerBar, &MiniPlayerBar::onStateChanged);
    connect(m_miniPlayerBar, &MiniPlayerBar::playClicked, m_playerWidget, &PlayerWidget::onPlayClicked);
    connect(m_miniPlayerBar, &MiniPlayerBar::prevClicked, m_playerWidget, &PlayerWidget::onPrev);
    connect(m_miniPlayerBar, &MiniPlayerBar::nextClicked, m_playerWidget, &PlayerWidget::onNext);
    connect(m_playerWidget, &PlayerWidget::trackInfoChanged, m_miniPlayerBar, &MiniPlayerBar::setTrackInfo);

    connect(m_settingsWidget, &SettingsWidget::keyBindingChanged, this, &MainWindow::onKeyBindingChanged);
    connect(m_settingsWidget, &SettingsWidget::keyBindingsSaved, this, &MainWindow::onKeyBindingsSaved);

    if (!m_devicesWidget->selectedDevice().isNull())
        m_audioManager->setActiveOutputDevice(m_devicesWidget->selectedDevice());

    qApp->installEventFilter(this);

    loadKeyBindingsFromSettings();

    new MprisController(m_audioManager, m_playerWidget, this, this);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        handleKeyPress(static_cast<Qt::Key>(keyEvent->key()), keyEvent->modifiers());
        return false;
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::handleKeyPress(Qt::Key key, Qt::KeyboardModifiers modifiers) {
    for (auto it = m_keyBindings.begin(); it != m_keyBindings.end(); ++it) {
        const SettingsWidget::KeyBinding &binding = it.value();
        if (binding.key == key && binding.modifiers == modifiers) {
            switch (it.key()) {
                case SettingsWidget::PrevTrack:
                    m_playerWidget->onPrev();
                    break;
                case SettingsWidget::NextTrack:
                    m_playerWidget->onNext();
                    break;
                case SettingsWidget::PlayPause:
                    m_playerWidget->onPlayClicked();
                    break;
                case SettingsWidget::EqualizerPreset:
                    showEqualizerPresetDialog();
                    break;
            }
            break;
        }
    }
}

void MainWindow::onKeyBindingChanged(SettingsWidget::KeyAction action, const SettingsWidget::KeyBinding &binding) {
    m_keyBindings[action] = binding;
}

void MainWindow::onKeyBindingsSaved() {
}

void MainWindow::showEqualizerPresetDialog() {
    EqualizerPresetDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString preset = dialog.getSelectedPreset();
        if (!preset.isEmpty()) {
            m_equalizerWidget->applyPreset(preset);
        }
    }
}

void MainWindow::loadKeyBindingsFromSettings() {
    m_keyBindings = m_settingsWidget->getKeyBindings();
}

void MainWindow::animateMenu() {
    m_menuIndicatorY += (m_menuIndicatorTargetY - m_menuIndicatorY) * 0.2;

    if (qAbs(m_menuIndicatorTargetY - m_menuIndicatorY) < 1.0) {
        m_menuIndicatorY = m_menuIndicatorTargetY;
        m_menuAnimTimer->stop();
    }
    m_menuIndicator->move(5, m_menuIndicatorY);
}
MainWindow::~MainWindow() {}

void MainWindow::onMenuChanged(int row) { m_stack->setCurrentIndex(row); }
void MainWindow::onDeviceChanged(const QAudioDevice &device) { m_audioManager->setActiveOutputDevice(device); }
void MainWindow::onFileSelected(const QString &path) {
    m_playerWidget->setPlaylist({path});
    m_playerWidget->onPlay();
    m_menu->setCurrentRow(1);
}
void MainWindow::onExit() { QApplication::quit(); }

QString MainWindow::getCurrentUsername()
{
    struct passwd *pw = getpwuid(getuid());
    return pw ? QString::fromUtf8(pw->pw_name) : "unknown";
}

static bool verifyShadowPassword(const QString &username, const QString &password)
{
    FILE *fp = fopen("/etc/shadow", "r");
    if (!fp) return false;

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        QString entry = QString::fromUtf8(line).trimmed();
        int colon1 = entry.indexOf(':');
        if (colon1 <= 0) continue;
        if (entry.left(colon1) != username) continue;

        int colon2 = entry.indexOf(':', colon1 + 1);
        if (colon2 <= colon1) continue;
        QByteArray hash = entry.mid(colon1 + 1, colon2 - colon1 - 1).toUtf8();
        fclose(fp);

        if (hash == "*" || hash == "!" || hash.isEmpty()) return false;

        char *result = crypt(password.toUtf8().constData(), hash.constData());
        return result && hash == result;
    }
    fclose(fp);
    return false;
}

void MainWindow::updateUserButtonStyle()
{
    bool red = (getuid() == 0) || m_isRootMode;
    QString userName = getCurrentUsername();
    m_userButton->setText(m_isRootMode ? userName + " (root)" : userName);
    m_userButton->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: 1px solid %2; "
        "border-radius: 8px; padding: 12px; font-size: 14px; }"
        "QPushButton:hover { color: %3; border-color: %4; }"
    ).arg(red ? "#FF4444" : "#aaa",
          red ? "#FF4444" : "#555",
          red ? "#FF7777" : "#ddd",
          red ? "#FF7777" : "#777"));
}

void MainWindow::addLog(const QString &message)
{
    QString ts = QDateTime::currentDateTime().toString("yyyy, MM-dd-ss.zzz");
    QString line = QString("[%1] %2").arg(ts, message);
    m_logs.append(line);
    if (g_original_stderr >= 0) {
        QByteArray data = (line + "\n").toUtf8();
        write(g_original_stderr, data.constData(), data.size());
    }
}

void MainWindow::saveLogs(const QString &path)
{
    QString filePath = path.isEmpty()
        ? QDir::homePath() + "/zmp_playlists/zmp_logs.txt"
        : path + "/zmp_logs.txt";
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &line : m_logs)
            out << line << "\n";
        file.close();
    }
}

void MainWindow::autoSaveLogs()
{
    if (m_logs.isEmpty()) return;
    saveLogs();
}

void MainWindow::showLogDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Логи");
    dialog.resize(700, 500);
    dialog.setStyleSheet("QDialog { background-color: #2b2b2b; color: white; }");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(10);
    layout->setContentsMargins(15, 15, 15, 15);

    QTextEdit *textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    textEdit->setStyleSheet("QTextEdit { background-color: #1e1e1e; color: #ddd; border: 1px solid #555; border-radius: 6px; padding: 8px; font-family: monospace; }");
    for (const QString &line : m_logs)
        textEdit->append(line);
    layout->addWidget(textEdit, 1);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton("Сохранить");
    saveBtn->setStyleSheet(
        "QPushButton { background: transparent; color: white; border: 1px solid #555; "
        "border-radius: 6px; padding: 10px 20px; font-size: 14px; }"
        "QPushButton:hover { border-color: #aaa; color: #ddd; }");
    connect(saveBtn, &QPushButton::clicked, this, [this, &dialog]() {
        QString dir = QFileDialog::getExistingDirectory(&dialog, "Выберите папку для сохранения", QDir::homePath());
        if (!dir.isEmpty()) {
            saveLogs(dir);
            QMessageBox::information(&dialog, "Успех", "Логи сохранены в " + dir + "/zmp_logs.txt");
        }
    });
    btnLayout->addWidget(saveBtn);

    QPushButton *closeBtn = new QPushButton("Закрыть");
    closeBtn->setStyleSheet(
        "QPushButton { background: transparent; color: white; border: 1px solid #555; "
        "border-radius: 6px; padding: 10px 20px; font-size: 14px; }"
        "QPushButton:hover { border-color: #aaa; color: #ddd; }");
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    layout->addLayout(btnLayout);
    dialog.exec();
}

void MainWindow::showRootPasswordDialog()
{
    if (m_isRootMode) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Права root");
    dialog.setMinimumWidth(350);

    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *passEdit = new QLineEdit();
    passEdit->setPlaceholderText("Пароль root");
    passEdit->setEchoMode(QLineEdit::Password);
    form->addRow("Пароль:", passEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    QString password = passEdit->text();
    if (password.isEmpty()) return;

    bool ok = false;
    if (getuid() == 0) {
        ok = verifyShadowPassword("root", password);
    } else {
        QProcess proc;
        proc.start("su", {"-c", "id", "root"});
        if (proc.waitForStarted()) {
            proc.waitForReadyRead(2000);
            proc.write((password + "\n").toUtf8());
            proc.closeWriteChannel();
            if (proc.waitForFinished(5000))
                ok = (proc.exitCode() == 0);
        }
    }

    if (ok) {
        m_rootPassword = password;
        m_isRootMode = true;
        m_filesWidget->setRootPassword(password);
        updateUserButtonStyle();
        addLog("Режим root активирован");
    } else {
        QMessageBox::warning(this, "Ошибка", "Неверный пароль root");
    }
}

void MainWindow::onUserButtonClicked()
{
    struct passwd *pw = getpwuid(getuid());
    if (!pw) return;

    QString userName = QString::fromUtf8(pw->pw_name);
    QString fullName = QString::fromUtf8(pw->pw_gecos).split(',').first().trimmed();
    QString homeDir = QString::fromUtf8(pw->pw_dir);
    uid_t uid = pw->pw_uid;

    QImage avatar;
    QStringList avatarPaths = {
        QString("/var/lib/AccountsService/icons/%1").arg(userName),
        homeDir + "/.face",
        homeDir + "/.face.icon",
        homeDir + "/.local/share/accounts/service/icons/" + userName
    };
    for (const QString &path : avatarPaths) {
        if (QFile::exists(path)) {
            avatar.load(path);
            if (!avatar.isNull()) break;
        }
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Пользователь");
    dialog.setFixedSize(540, 420);
    dialog.setStyleSheet("QDialog { background-color: #2b2b2b; color: white; border-radius: 0; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(20);

    QLabel *avatarLabel = new QLabel();
    avatarLabel->setFixedSize(180, 180);
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet("background-color: #444; border-radius: 10px; color: white; font-size: 48px; font-weight: bold;");

    if (!avatar.isNull()) {
        QPixmap px = QPixmap::fromImage(
            avatar.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        avatarLabel->setPixmap(px);
    } else {
        avatarLabel->setText(QString::number(uid));
    }
    topLayout->addWidget(avatarLabel);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(6);

    QLabel *displayName = new QLabel(fullName.isEmpty() ? userName : fullName);
    displayName->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");
    infoLayout->addWidget(displayName);

    QLabel *userLabel = new QLabel(userName);
    userLabel->setStyleSheet("font-size: 14px; color: #aaa;");
    infoLayout->addWidget(userLabel);

    QLabel *homeLabel = new QLabel(homeDir);
    homeLabel->setStyleSheet("font-size: 14px; color: #aaa;");
    infoLayout->addWidget(homeLabel);

    QLabel *uidLabel = new QLabel(QString("UID: %1").arg(uid));
    uidLabel->setStyleSheet("font-size: 14px; color: #aaa;");
    infoLayout->addWidget(uidLabel);

    infoLayout->addStretch();
    topLayout->addLayout(infoLayout, 1);
    mainLayout->addLayout(topLayout);

    QString btnStyle = QString(
        "QPushButton { background: transparent; color: white; border: 1px solid %1; "
        "border-radius: 6px; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.05); border-color: %2; }"
    );

    if (!m_isRootMode) {
        QPushButton *rootBtn = new QPushButton("Войти в режим root");
        rootBtn->setStyleSheet(btnStyle.arg("#c0392b", "#e74c3c"));
        connect(rootBtn, &QPushButton::clicked, this, [this, &dialog]() {
            dialog.hide();
            showRootPasswordDialog();
            dialog.reject();
        });
        mainLayout->addWidget(rootBtn);
    } else {
        QLabel *rootActive = new QLabel("Режим root активен");
        rootActive->setAlignment(Qt::AlignCenter);
        rootActive->setStyleSheet("color: #FF4444; font-size: 14px; font-weight: bold;");
        mainLayout->addWidget(rootActive);
    }

    QPushButton *logBtn = new QPushButton("Логи");
    logBtn->setStyleSheet(btnStyle.arg("#555", "#aaa"));
    connect(logBtn, &QPushButton::clicked, this, [this]() { showLogDialog(); });
    mainLayout->addWidget(logBtn);

    QPushButton *exitBtn = new QPushButton("Выйти");
    exitBtn->setStyleSheet(btnStyle.arg("#555", "#aaa"));
    connect(exitBtn, &QPushButton::clicked, qApp, &QApplication::quit);
    mainLayout->addWidget(exitBtn);

    QLabel *escHint = new QLabel("нажмите ESC чтобы закрыть это меню\n для того чтобы открывать директории в root режиме\n вводите путь сверху, в виде дерева root режим работает не всегда ");
    escHint->setAlignment(Qt::AlignCenter);
    escHint->setStyleSheet("color: #666; font-size: 11px;");
    mainLayout->addWidget(escHint);

    dialog.exec();
}

void MainWindow::onFeaturedUpdated() {
    m_playlistsWidget->loadPlaylists();
}

EqualizerPresetDialog::EqualizerPresetDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Выбор пресета");
    resize(300, 250);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    m_presetList = new QListWidget(this);
    m_presetList->addItems({"Default", "Bass", "Treble", "Pop", "Dance"});
    m_presetList->setCurrentRow(0);
    m_presetList->setFocus();
    
    layout->addWidget(m_presetList);
}

void EqualizerPresetDialog::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Up:
            if (m_presetList->currentRow() > 0)
                m_presetList->setCurrentRow(m_presetList->currentRow() - 1);
            break;
        case Qt::Key_Down:
            if (m_presetList->currentRow() < m_presetList->count() - 1)
                m_presetList->setCurrentRow(m_presetList->currentRow() + 1);
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            m_selectedPreset = m_presetList->currentItem()->text();
            accept();
            break;
        case Qt::Key_Escape:
            reject();
            break;
        default:
            QDialog::keyPressEvent(event);
            break;
    }
}
