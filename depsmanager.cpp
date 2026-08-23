#include "depsmanager.h"
#include "translator.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
//  Отчёт о недостающих зависимостях без Qt (безопасно использовать в
//  форкнутом процессе после SIGSEGV)
// ---------------------------------------------------------------------------

static bool zmpExecutableExists(const char *name)
{
    const char *pathEnv = getenv("PATH");
    if (!pathEnv || !*pathEnv)
        pathEnv = "/usr/local/bin:/usr/bin:/bin";
    std::string paths(pathEnv);
    size_t start = 0;
    while (start <= paths.size()) {
        size_t end = paths.find(':', start);
        if (end == std::string::npos) end = paths.size();
        std::string dir = paths.substr(start, end - start);
        if (!dir.empty()) {
            std::string full = dir + "/" + name;
            if (access(full.c_str(), X_OK) == 0)
                return true;
        }
        if (end == paths.size()) break;
        start = end + 1;
    }
    return false;
}

static std::string zmpMissingDepsReportText()
{
    // Library checks via ldconfig -p
    std::string ldconfigOut;
    {
        FILE *pp = popen("ldconfig -p 2>/dev/null", "r");
        if (pp) {
            char buf[1024];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), pp)) > 0)
                ldconfigOut.append(buf, n);
            pclose(pp);
        }
    }
    auto libFound = [&ldconfigOut](const char *soname) -> int {
        // 1 = found, 0 = missing, -1 = unknown (no ldconfig output)
        if (ldconfigOut.empty()) return -1;
        return ldconfigOut.find(soname) != std::string::npos ? 1 : 0;
    };
    auto toolFound = [](const char *tool) -> int {
        return zmpExecutableExists(tool) ? 1 : 0;
    };

    std::string report;
    auto add = [&report](const char *title, int st) {
        if (st == 1)
            return;
        report += "  ";
        report += title;
        if (st < 0)
            report += " (?)";
        report += "\n";
    };
    add("SoundTouch (libSoundTouch.so)", libFound("libSoundTouch.so"));
    add("TagLib (libtag.so)", libFound("libtag.so"));
    add("projectM (libprojectM.so)", libFound("libprojectM.so"));
    add("FFmpeg (ffmpeg/ffprobe)",
        toolFound("ffmpeg") && toolFound("ffprobe") ? 1 : 0);
    add("SMB-клиент (smbclient)", toolFound("smbclient"));

    return report;
}

void zmpOpenTerminalWithMessage(const QString &message)
{
    const char *terminals[] = {
        "x-terminal-emulator", "gnome-terminal", "konsole",
        "xfce4-terminal", "tilix", "alacritty", "kitty", "xterm"
    };

    // Write the message to a temp file so quoting stays simple
    const char *tmpPath = "/tmp/zmp_deps_report.txt";
    FILE *f = fopen(tmpPath, "w");
    if (!f)
        return;
    const QByteArray utf8 = message.toUtf8();
    fwrite(utf8.constData(), 1, utf8.size(), f);
    fclose(f);

    std::string script = std::string("cat ") + tmpPath +
                         "; echo; echo 'Press Enter to close...'; read";

    for (const char *term : terminals) {
        if (!zmpExecutableExists(term))
            continue;
        pid_t pid = fork();
        if (pid == 0) {
            setsid();
            execlp(term, term, "-e", "bash", "-c", script.c_str(), (char *)nullptr);
            _exit(127);
        }
        return; // запущено — не пробуем другие терминалы
    }
}

static void zmpCrashSignalHandler(int sig)
{
    // Report from a forked copy: the dying parent stays untouched
    pid_t pid = fork();
    if (pid == 0) {
        std::string missing = zmpMissingDepsReportText();
        if (!missing.empty()) {
            std::string msg = "ZMP\n\nНе найдены зависимости:\n" + missing +
                              "\nПожалуйста, установите их.\n";
            fputs(msg.c_str(), stderr);
            fflush(stderr);
            if (!isatty(STDOUT_FILENO) && !isatty(STDERR_FILENO)) {
                const char *terminals[] = {
                    "x-terminal-emulator", "gnome-terminal", "konsole",
                    "xfce4-terminal", "tilix", "alacritty", "kitty", "xterm"
                };
                const char *tmpPath = "/tmp/zmp_deps_report.txt";
                if (FILE *f = fopen(tmpPath, "w")) {
                    fwrite(msg.data(), 1, msg.size(), f);
                    fclose(f);
                }
                std::string script = std::string("cat ") + tmpPath +
                                     "; echo; echo 'Press Enter to close...'; read";
                for (const char *term : terminals) {
                    if (!zmpExecutableExists(term))
                        continue;
                    pid_t tpid = fork();
                    if (tpid == 0) {
                        setsid();
                        execlp(term, term, "-e", "bash", "-c", script.c_str(), (char *)nullptr);
                        _exit(127);
                    }
                    break;
                }
            }
        }
        _exit(0);
    } else if (pid > 0) {
        int st;
        waitpid(pid, &st, 0);
    }

    // Restore default behaviour and re-raise (core dump etc.)
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

void zmpInstallCrashHandler()
{
    std::signal(SIGSEGV, zmpCrashSignalHandler);
    std::signal(SIGABRT, zmpCrashSignalHandler);
    std::signal(SIGFPE, zmpCrashSignalHandler);
    std::signal(SIGILL, zmpCrashSignalHandler);
}

// ---------------------------------------------------------------------------
//  DepsProxyDialog
// ---------------------------------------------------------------------------

DepsProxyDialog::DepsProxyDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(ztr("Настройка прокси установки"));
    setMinimumWidth(500);
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(new QLabel(ztr("Тип прокси:")));
    m_proxyTypeCombo = new QComboBox;
    m_proxyTypeCombo->addItem(ztr("Без прокси"), QString("none"));
    m_proxyTypeCombo->addItem("SOCKS5", QString("socks5"));
    m_proxyTypeCombo->addItem("HTTP", QString("http"));
    m_proxyTypeCombo->addItem("HTTPS", QString("https"));
    mainLayout->addWidget(m_proxyTypeCombo);

    m_proxySettingsWidget = new QWidget;
    QVBoxLayout *proxyLayout = new QVBoxLayout(m_proxySettingsWidget);
    proxyLayout->setContentsMargins(0, 0, 0, 0);

    proxyLayout->addWidget(new QLabel(ztr("IP:Порт прокси:")));
    m_proxyHostEdit = new QLineEdit;
    m_proxyHostEdit->setPlaceholderText("127.0.0.1:1080");
    proxyLayout->addWidget(m_proxyHostEdit);

    m_authCheck = new QCheckBox(ztr("Требовать авторизацию"));
    proxyLayout->addWidget(m_authCheck);

    m_authWidget = new QWidget;
    QVBoxLayout *authLayout = new QVBoxLayout(m_authWidget);
    authLayout->setContentsMargins(0, 0, 0, 0);
    authLayout->addWidget(new QLabel(ztr("Имя пользователя:")));
    m_proxyUserEdit = new QLineEdit;
    authLayout->addWidget(m_proxyUserEdit);
    authLayout->addWidget(new QLabel(ztr("Пароль:")));
    m_proxyPassEdit = new QLineEdit;
    m_proxyPassEdit->setEchoMode(QLineEdit::Password);
    authLayout->addWidget(m_proxyPassEdit);
    m_authWidget->setVisible(false);
    proxyLayout->addWidget(m_authWidget);

    m_sslAllowCheck = new QCheckBox(ztr("Разрешить SSL сертификаты (включая самоподписанные)"));
    proxyLayout->addWidget(m_sslAllowCheck);

    m_dnsThroughCheck = new QCheckBox(ztr("DNS запросы через прокси"));
    proxyLayout->addWidget(m_dnsThroughCheck);

    m_proxySettingsWidget->setVisible(false);
    mainLayout->addWidget(m_proxySettingsWidget);

    mainLayout->addStretch();

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &DepsProxyDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_proxyTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DepsProxyDialog::onProxyTypeChanged);
    connect(m_authCheck, &QCheckBox::toggled, this, &DepsProxyDialog::onAuthToggled);

    loadFromConfig();
}

void DepsProxyDialog::onProxyTypeChanged(int index)
{
    QString type = m_proxyTypeCombo->itemData(index).toString();
    m_proxySettingsWidget->setVisible(type != "none");
}

void DepsProxyDialog::onAuthToggled(bool checked)
{
    m_authWidget->setVisible(checked);
}

void DepsProxyDialog::onAccept()
{
    QString type = m_proxyTypeCombo->currentData().toString();
    if (type != "none" && m_proxyHostEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, ztr("Ошибка"), ztr("Введите IP:Порт прокси"));
        return;
    }

    QJsonObject config;
    config["proxy_type"] = type;
    config["proxy_host"] = m_proxyHostEdit->text().trimmed();
    config["proxy_auth"] = m_authCheck->isChecked();
    config["proxy_username"] = m_proxyUserEdit->text();
    config["proxy_password"] = m_proxyPassEdit->text();
    config["proxy_ssl_allow"] = m_sslAllowCheck->isChecked();
    config["proxy_dns_through"] = m_dnsThroughCheck->isChecked();
    saveDepsConfig(config);
    accept();
}

QString DepsProxyDialog::proxyType() const { return m_proxyTypeCombo->currentData().toString(); }
QString DepsProxyDialog::proxyHost() const { return m_proxyHostEdit->text().trimmed(); }
bool DepsProxyDialog::proxyAuth() const { return m_authCheck->isChecked(); }
QString DepsProxyDialog::proxyUsername() const { return m_proxyUserEdit->text(); }
QString DepsProxyDialog::proxyPassword() const { return m_proxyPassEdit->text(); }
bool DepsProxyDialog::proxySslAllow() const { return m_sslAllowCheck->isChecked(); }
bool DepsProxyDialog::proxyDnsThrough() const { return m_dnsThroughCheck->isChecked(); }

void DepsProxyDialog::loadFromConfig()
{
    QJsonObject config = loadDepsConfig();
    QString type = config["proxy_type"].toString("none");
    int idx = m_proxyTypeCombo->findData(type);
    if (idx >= 0) m_proxyTypeCombo->setCurrentIndex(idx);
    else if (type != "none") m_proxyTypeCombo->setCurrentIndex(1);
    m_proxyHostEdit->setText(config["proxy_host"].toString());
    m_authCheck->setChecked(config["proxy_auth"].toBool());
    m_proxyUserEdit->setText(config["proxy_username"].toString());
    m_proxyPassEdit->setText(config["proxy_password"].toString());
    m_sslAllowCheck->setChecked(config["proxy_ssl_allow"].toBool());
    m_dnsThroughCheck->setChecked(config["proxy_dns_through"].toBool());
}

QJsonObject DepsProxyDialog::loadDepsConfig()
{
    QString configPath = QDir::homePath() + "/zmp_playlists/config.json";
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly))
        return QJsonObject();

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return QJsonObject();

    return doc.object()["deps"].toObject();
}

void DepsProxyDialog::saveDepsConfig(const QJsonObject &config)
{
    QDir().mkpath(QDir::homePath() + "/zmp_playlists");
    QString configPath = QDir::homePath() + "/zmp_playlists/config.json";
    QFile file(configPath);

    QJsonObject root;
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject())
            root = doc.object();
        file.close();
    }

    root["deps"] = config;

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

// ---------------------------------------------------------------------------
//  DependencyManager
// ---------------------------------------------------------------------------

DependencyManager *DependencyManager::instance()
{
    static DependencyManager inst(qApp);
    return &inst;
}

DependencyManager::DependencyManager(QObject *parent)
    : QObject(parent)
{
    detectOs();

    // Speed sampler: emits progress with measured download speed once per second
    m_speedTimer = new QTimer(this);
    connect(m_speedTimer, &QTimer::timeout, this, [this]() {
        if (!m_installRunning)
            return;
        qint64 bps = m_outputBytes - m_lastSpeedBytes;
        if (bps < 0) bps = 0;
        m_lastSpeedBytes = m_outputBytes;
        emit installProgress(m_currentPackage, m_currentPercent, bps);
    });
}

void DependencyManager::detectOs()
{
    QFile f("/etc/os-release");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString id, idLike;
        while (!f.atEnd()) {
            QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.startsWith("NAME="))
                m_distroName = line.mid(5).remove('"');
            else if (line.startsWith("ID="))
                id = line.mid(3).remove('"').toLower();
            else if (line.startsWith("ID_LIKE="))
                idLike = line.mid(8).remove('"').toLower();
        }
        f.close();

        QStringList fam = idLike.split(' ', Qt::SkipEmptyParts);
        fam << id;
        if (fam.contains("arch")) m_pkgManager = Pacman;
        else if (id == "fedora" || fam.contains("fedora") || fam.contains("rhel") ||
                 id == "centos" || id == "rocky" || id == "almalinux" || id == "amzn")
            m_pkgManager = Dnf;
        else if (fam.contains("debian") || id == "debian" || id == "ubuntu")
            m_pkgManager = Apt;
    }

    if (m_pkgManager == Apt && QStandardPaths::findExecutable("apt-get").isEmpty())
        m_pkgManager = None;
    else if (m_pkgManager == Dnf && QStandardPaths::findExecutable("dnf").isEmpty()) {
        m_pkgManager = QStandardPaths::findExecutable("yum").isEmpty() ? None : Yum;
    } else if (m_pkgManager == Pacman && QStandardPaths::findExecutable("pacman").isEmpty())
        m_pkgManager = None;

    QProcess ldconfig;
    ldconfig.start("ldconfig", {"-p"});
    if (ldconfig.waitForFinished(3000)) {
        m_ldconfigCache = QString::fromUtf8(ldconfig.readAllStandardOutput());
        m_ldconfigOk = (ldconfig.exitCode() == 0 && !m_ldconfigCache.isEmpty());
    }
}

DependencyManager::Status DependencyManager::checkLibrary(const QString &soname) const
{
    if (!m_ldconfigOk)
        return Unknown;

    QRegularExpression re(QString("\\s%1(?:\\.\\d+)?\\b").arg(QRegularExpression::escape(soname)));
    return m_ldconfigCache.contains(re) ? Found : Missing;
}

DependencyManager::Status DependencyManager::checkTool(const QString &tool) const
{
    return QStandardPaths::findExecutable(tool).isEmpty() ? Missing : Found;
}

QString DependencyManager::pkgManagerCommand() const
{
    switch (m_pkgManager) {
        case Apt: return "apt-get";
        case Dnf: return "dnf";
        case Yum: return "yum";
        case Pacman: return "pacman";
        default: return {};
    }
}

QVector<DependencyManager::DepResult> DependencyManager::runChecks()
{
    qDebug() << "deps: checking dependencies (os-release, ldconfig, PATH)";
    detectOs();

    QVector<DepResult> results;

    {
        DepResult r;
        r.title = "Qt6";
        r.description = ztr("Библиотеки интерфейса приложения");
        r.status = Found;
        results.append(r);
    }
    auto libEntry = [this, &results](const QString &title, const QString &desc, const QString &soname,
                           const QStringList &aptPkgs, const QStringList &dnfPkgs,
                           const QStringList &pacPkgs) {
        DepResult r;
        r.title = title;
        r.description = desc;
        r.status = checkLibrary(soname);
        switch (m_pkgManager) {
            case Apt: r.packages = aptPkgs; break;
            case Dnf:
            case Yum: r.packages = dnfPkgs; break;
            case Pacman: r.packages = pacPkgs; break;
            default: break;
        }
        results.append(r);
    };
    auto toolEntry = [this, &results](const QString &title, const QString &desc,
                            const QStringList &tools, Status *combined,
                            const QStringList &pkgs) {
        DepResult r;
        r.title = title;
        r.description = desc;
        Status s = Found;
        for (const QString &t : tools) {
            Status ts = checkTool(t);
            if (ts == Missing) s = Missing;
        }
        r.status = combined ? *combined : s;
        r.packages = pkgs;
        results.append(r);
    };

    libEntry("SoundTouch", ztr("Скорость и питч аудио (libSoundTouch.so)"), "libSoundTouch.so",
             {"libsoundtouch1-dev", "libsoundtouch-float1"}, {"soundtouch-devel"}, {"soundtouch"});
    libEntry("TagLib", ztr("Чтение тегов и обложек (libtag.so)"), "libtag.so",
             {"libtag1v5", "libtag1-dev"}, {"taglib"}, {"taglib"});
    libEntry("projectM", ztr("Визуализация projectM (libprojectM.so)"), "libprojectM.so",
             {"libprojectm-dev"}, {"projectM-devel"}, {"projectm"});

    {
        DepResult r;
        r.title = "FFmpeg";
        r.description = ztr("Определение битрейта и транскодирование (ffmpeg, ffprobe)");
        Status s1 = checkTool("ffmpeg");
        Status s2 = checkTool("ffprobe");
        r.status = (s1 == Unknown || s2 == Unknown) ? Unknown
                 : (s1 == Found && s2 == Found) ? Found : Missing;
        switch (m_pkgManager) {
            case Apt:
            case Dnf:
            case Yum:
            case Pacman: r.packages = {"ffmpeg"}; break;
            default: break;
        }
        results.append(r);
    }

    toolEntry("SMB-клиент", ztr("Загрузка файлов по сети Windows (smbclient)"),
              {"smbclient"}, nullptr,
              m_pkgManager == Apt ? QStringList{"smbclient"}
              : (m_pkgManager == Pacman) ? QStringList{"smbclient"}
              : (m_pkgManager == Dnf || m_pkgManager == Yum) ? QStringList{"samba-client"}
              : QStringList{});

    for (const DepResult &r : results) {
        qDebug() << "deps:" << r.title << "->"
                 << (r.status == Found ? "found"
                     : r.status == Missing ? "missing" : "unknown");
    }

    emit checksFinished(results);
    return results;
}

QString DependencyManager::elevationPrefix() const
{
    if (::getuid() == 0)
        return {};
    if (!QStandardPaths::findExecutable("pkexec").isEmpty())
        return "pkexec";
    if (!QStandardPaths::findExecutable("sudo").isEmpty())
        return "sudo";
    return {};
}

QString DependencyManager::proxyDisplayIp() const
{
    QJsonObject cfg = DepsProxyDialog::loadDepsConfig();
    if (cfg["proxy_type"].toString("none") == "none")
        return {};
    return cfg["proxy_host"].toString();
}

bool DependencyManager::hasProxy() const
{
    return DepsProxyDialog::loadDepsConfig()["proxy_type"].toString("none") != "none";
}

bool DependencyManager::launchedFromTerminal()
{
    return isatty(STDOUT_FILENO) || isatty(STDERR_FILENO);
}

void DependencyManager::reportMissingDependencies()
{
    QVector<DepResult> results = runChecks();

    QStringList missing;
    for (const DepResult &r : results) {
        if (r.status == Found)
            continue;
        QString line = r.title;
        if (r.status == Unknown)
            line += ztr(" (не удалось проверить)");
        if (!r.packages.isEmpty())
            line += " [" + r.packages.join(" ") + "]";
        missing << line;
    }
    if (missing.isEmpty())
        return;

    const QString msg = ztr("Не найдены зависимости:") + "\n" +
                        missing.join("\n") + "\n\n" +
                        ztr("Пожалуйста, установите их");

    if (launchedFromTerminal()) {
        // Запущено из терминала — пишем в логи/stderr
        qCritical().noquote() << msg;
    } else {
        zmpOpenTerminalWithMessage(msg);
    }

    // Qt установлен — показываем диалоговое окно с кнопкой ОК
    if (qApp)
        QMessageBox::warning(nullptr, ztr("Зависимости"), msg);
}

void DependencyManager::applyProxyEnv(QStringList &env)
{
    QJsonObject cfg = DepsProxyDialog::loadDepsConfig();
    QString type = cfg["proxy_type"].toString("none");
    if (type == "none")
        return;

    QString hostPort = cfg["proxy_host"].toString();
    if (hostPort.isEmpty())
        return;

    QString url;
    if (type == "socks5") {
        url = cfg["proxy_dns_through"].toBool() ? "socks5h://" : "socks5://";
        url += hostPort;
    } else {
        url = type + "://";
        if (cfg["proxy_auth"].toBool())
            url += QUrl::toPercentEncoding(cfg["proxy_username"].toString()) + ":" +
                   QUrl::toPercentEncoding(cfg["proxy_password"].toString()) + "@";
        url += hostPort;
    }

    env << ("http_proxy=" + url)
        << ("https_proxy=" + url)
        << ("HTTP_PROXY=" + url)
        << ("HTTPS_PROXY=" + url)
        << ("all_proxy=" + url)
        << ("ALL_PROXY=" + url);
}

void DependencyManager::startInstall(const QVector<DepResult> &selected)
{
    if (m_installRunning)
        return;

    m_queue.clear();
    for (const DepResult &r : selected) {
        if (!r.packages.isEmpty() && r.status != Found)
            m_queue.append(r);
    }

    if (m_queue.isEmpty()) {
        emit installFinished(true, {}, {});
        return;
    }

    m_succeeded.clear();
    m_failed.clear();
    m_step = 0;
    m_totalSteps = m_queue.size();
    m_outputBytes = 0;
    m_lastSpeedBytes = 0;
    m_currentPackage.clear();
    m_currentPercent = 0;
    m_installRunning = true;

    emit installStarted(m_totalSteps);
    m_speedTimer->start(1000);

    QTimer::singleShot(0, this, [this]() { runNextStep(); });
}

void DependencyManager::runNextStep()
{
    if (m_proc) {
        disconnect(m_proc, nullptr, this, nullptr);
        m_proc->deleteLater();
        m_proc = nullptr;
    }

    if (m_step >= m_queue.size()) {
        m_installRunning = false;
        m_speedTimer->stop();
        emit installFinished(m_failed.isEmpty(), m_succeeded, m_failed);
        return;
    }

    const DepResult dep = m_queue.at(m_step);
    m_currentPackage = dep.title;
    m_currentPercent = static_cast<int>(100.0 * m_step / qMax(1, m_totalSteps));
    emit installProgress(dep.title, m_currentPercent, 0);

    QStringList args;
    QString elevate = elevationPrefix();
    if (!elevate.isEmpty())
        args << elevate;
    args << pkgManagerCommand();

    switch (m_pkgManager) {
        case Apt:
            args << "install" << "-y";
            break;
        case Dnf:
        case Yum:
            args << "install" << "-y";
            break;
        case Pacman:
            args << "-S" << "--noconfirm" << "--needed";
            break;
        default:
            m_failed.append(dep.title);
            m_step++;
            QTimer::singleShot(0, this, [this]() { runNextStep(); });
            return;
    }
    for (const QString &p : dep.packages)
        args << p;

    m_outputBytes = 0;
    m_lastSpeedBytes = 0;

    m_proc = new QProcess(this);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, [this]() {
        m_outputBytes += m_proc->readAllStandardOutput().size();
    });
    connect(m_proc, &QProcess::readyReadStandardError, this, [this]() {
        m_outputBytes += m_proc->readAllStandardError().size();
    });

    QProcessEnvironment penv = QProcessEnvironment::systemEnvironment();
    QStringList extraEnv;
    applyProxyEnv(extraEnv);
    for (const QString &kv : extraEnv) {
        int eq = kv.indexOf('=');
        if (eq > 0) penv.insert(kv.left(eq), kv.mid(eq + 1));
    }
    penv.insert("DEBIAN_FRONTEND", "noninteractive");
    m_proc->setProcessEnvironment(penv);

    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus st) {
        const DepResult dep = m_queue.value(m_step);
        if (code == 0 && st == QProcess::NormalExit)
            m_succeeded.append(dep.title);
        else
            m_failed.append(dep.title);
        m_step++;
        m_currentPercent = static_cast<int>(100.0 * m_step / qMax(1, m_totalSteps));
        emit installProgress(dep.title, m_currentPercent, 0);

        QTimer::singleShot(150, this, [this]() { runNextStep(); });
    });

    m_proc->start(args.first(), args.mid(1));
}

// ---------------------------------------------------------------------------
//  DependencyCheckDialog
// ---------------------------------------------------------------------------

QPointer<QDialog> DependencyCheckDialog::s_active = nullptr;

DependencyCheckDialog::DependencyCheckDialog(QWidget *parent)
    : QDialog(parent)
{
    s_active = this;
    setWindowTitle(ztr("Проверка зависимостей"));
    setMinimumWidth(560);
    setMinimumHeight(420);
    setModal(false);

    QVBoxLayout *layout = new QVBoxLayout(this);

    DependencyManager *mgr = DependencyManager::instance();
    QString osInfo = mgr->distroName().isEmpty() ? "?" : mgr->distroName();
    QString pmInfo;
    switch (mgr->packageManager()) {
        case DependencyManager::Apt: pmInfo = "APT"; break;
        case DependencyManager::Dnf: pmInfo = "DNF"; break;
        case DependencyManager::Yum: pmInfo = "YUM"; break;
        case DependencyManager::Pacman: pmInfo = "pacman"; break;
        default: pmInfo = ztr("не определён"); break;
    }
    layout->addWidget(new QLabel(ztr("ОС:") + " " + osInfo + "   |   " +
                                 ztr("Пакетный менеджер:") + " " + pmInfo));

    m_statusLabel = new QLabel(ztr("Проверка..."));
    layout->addWidget(m_statusLabel);

    m_list = new QListWidget;
    layout->addWidget(m_list, 1);

    QHBoxLayout *btnRow = new QHBoxLayout;
    m_installBtn = new QPushButton(ztr("Доустановить"));
    m_closeBtn = new QPushButton(ztr("Закрыть"));
    btnRow->addWidget(m_installBtn, 1);
    btnRow->addWidget(m_closeBtn);
    layout->addLayout(btnRow);

    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
    connect(m_installBtn, &QPushButton::clicked, this, &DependencyCheckDialog::onInstallClicked);

    connect(mgr, &DependencyManager::installStarted,
            this, &DependencyCheckDialog::onInstallStarted);
    connect(mgr, &DependencyManager::installProgress,
            this, &DependencyCheckDialog::onInstallProgress);
    connect(mgr, &DependencyManager::installFinished,
            this, &DependencyCheckDialog::onInstallFinished);

    repopulate(mgr->runChecks());
}

static QString statusText(DependencyManager::Status s)
{
    switch (s) {
        case DependencyManager::Found: return ztr("найдено");
        case DependencyManager::Missing: return ztr("отсутствует");
        default: return ztr("не удалось проверить");
    }
}

static QColor statusColor(DependencyManager::Status s)
{
    switch (s) {
        case DependencyManager::Found: return QColor("#4CAF50");
        case DependencyManager::Missing: return QColor("#F44336");
        default: return QColor("#FFC107");
    }
}

void DependencyCheckDialog::repopulate(const QVector<DependencyManager::DepResult> &results)
{
    m_results = results;
    m_list->clear();

    int unknown = 0, missing = 0;
    for (const auto &r : m_results) {
        if (r.status == DependencyManager::Unknown) unknown++;
        if (r.status == DependencyManager::Missing) missing++;

        QListWidgetItem *item = new QListWidgetItem(m_list);
        item->setText(QString("%1 — %2").arg(r.title, r.description));
        item->setForeground(statusColor(r.status));
        item->setData(Qt::UserRole, r.status);

        if (r.status != DependencyManager::Found) {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        } else {
            item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
        }
    }

    if (unknown > 0)
        m_statusLabel->setText(ztr("Не удалось проверить элементов:") + " " + QString::number(unknown) +
                               ". " + ztr("Отметьте нужные для установки."));
    else if (missing > 0)
        m_statusLabel->setText(ztr("Отсутствующих зависимостей:") + " " + QString::number(missing));
    else
        m_statusLabel->setText(ztr("Все зависимости найдены"));

    bool canInstall = (missing > 0 || unknown > 0);
    m_installBtn->setEnabled(canInstall &&
                             DependencyManager::instance()->packageManager() != DependencyManager::None &&
                             !DependencyManager::instance()->installRunning());
}

void DependencyCheckDialog::onInstallClicked()
{
    DependencyManager *mgr = DependencyManager::instance();

    // Configure proxy first (same screen as Jamendo, without API key)
    DepsProxyDialog proxyDlg(this);
    if (proxyDlg.exec() != QDialog::Accepted)
        return;

    QVector<DependencyManager::DepResult> selected;
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        if (item->checkState() == Qt::Checked && i < m_results.size())
            selected.append(m_results[i]);
    }

    if (selected.isEmpty()) {
        QMessageBox::information(this, ztr("Установка зависимостей"),
                                 ztr("Ничего не выбрано для установки"));
        return;
    }

    mgr->startInstall(selected);
}

void DependencyCheckDialog::onInstallStarted(int totalSteps)
{
    m_installBtn->setEnabled(false);
    m_list->setEnabled(false);
    m_statusLabel->setTextFormat(Qt::PlainText);
    m_statusLabel->setText(ztr("Установка зависимостей") + ": 0/" + QString::number(totalSteps));
}

QString DependencyCheckDialog::speedColor(qint64 speedBps)
{
    double kb = speedBps / 1024.0;
    if (kb > 900) return "#4CAF50";
    if (kb > 200) return "#FFC107";
    if (kb > 5) return "#F44336";
    return "#888888";
}

void DependencyCheckDialog::onInstallProgress(const QString &currentPackage, int percent, qint64 speedBps)
{
    DependencyManager *mgr = DependencyManager::instance();
    QString color = speedColor(speedBps);

    QString speedStr;
    double kb = speedBps / 1024.0;
    if (kb > 0.01)
        speedStr = QString("%1 КБ/с").arg(kb, 0, 'f', 1);
    else
        speedStr = ztr("ожидание...");

    QString proxPart;
    QString proxyIp = mgr->proxyDisplayIp();
    if (mgr->hasProxy() && !proxyIp.isEmpty())
        proxPart = QString("<span style='color:%1;'>%2</span>").arg(color, proxyIp);
    else
        proxPart = QString("<span style='color:#4CAF50;'>%1</span>").arg(ztr("без прокси"));

    m_statusLabel->setTextFormat(Qt::RichText);
    m_statusLabel->setText(QString("%1: %2 %3% | <span style='color:%4;'>%5</span> | %6")
        .arg(ztr("Установка зависимостей"), currentPackage)
        .arg(percent)
        .arg(color, speedStr, proxPart));
}

void DependencyCheckDialog::onInstallFinished(bool success, const QStringList &succeeded,
                                              const QStringList &failed)
{
    m_installBtn->setEnabled(true);
    m_list->setEnabled(true);

    repopulate(DependencyManager::instance()->runChecks());

    if (!isVisible())
        return;

    QString msg;
    if (!succeeded.isEmpty()) {
        msg += ztr("Успешно установлено:") + "\n  " + succeeded.join("\n  ") + "\n";
    }
    if (!failed.isEmpty()) {
        if (!msg.isEmpty()) msg += "\n";
        msg += ztr("Не удалось установить:") + "\n  " + failed.join("\n  ");
    }
    if (msg.isEmpty())
        msg = success ? ztr("Готово") : ztr("Установка завершилась с ошибками");

    QMessageBox::information(this, ztr("Установка зависимостей"), msg.trimmed());
}

void DependencyCheckDialog::closeEvent(QCloseEvent *event)
{
    if (s_active == this)
        s_active = nullptr;
    event->accept();
}

void DependencyCheckDialog::showEvent(QShowEvent *event)
{
    s_active = this;
    QDialog::showEvent(event);
}
