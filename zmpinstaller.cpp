#include "zmpinstaller.h"
#include "translator.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QPushButton>
#include <QVBoxLayout>

#include <cstring>
#include <unistd.h>

// ---------------------------------------------------------------------------
//  Проверка установлен ли ZMP как приложение
// ---------------------------------------------------------------------------

bool zmpInstalledAsApp()
{
#ifdef Q_OS_WIN
    const bool win = QFileInfo::exists("C:/Program Files/ZMP/zmp.exe");
    qDebug() << "startup: checking C:/Program Files/ZMP ->"
             << (win ? "installed" : "not found");
    return win;
#else
    qDebug() << "startup: checking /usr/bin for installed ZMP binary";
    QFile f(QDir::homePath() + "/zmp_playlists/config.json");
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        const QString bin = root.value("installed_bin").toString();
        if (!bin.isEmpty()) {
            const bool ok = QFileInfo::exists("/usr/bin/" + bin);
            qDebug() << "startup: /usr/bin check result:" << bin
                     << "->" << (ok ? "installed" : "not found");
            return ok;
        }
    }
    const bool def = QFileInfo::exists("/usr/bin/zmp");
    qDebug() << "startup: /usr/bin check result: zmp ->"
             << (def ? "installed" : "not found");
    return def;
#endif
}

QString ZmpInstallDialog::installedBinaryPath()
{
#ifdef Q_OS_WIN
    return QFileInfo::exists("C:/Program Files/ZMP/zmp.exe")
               ? QStringLiteral("C:/Program Files/ZMP/zmp.exe") : QString();
#else
    QFile f(QDir::homePath() + "/zmp_playlists/config.json");
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        const QString bin = root.value("installed_bin").toString().trimmed();
        if (!bin.isEmpty() && QFileInfo::exists("/usr/bin/" + bin))
            return "/usr/bin/" + bin;
    }
    return QFileInfo::exists("/usr/bin/zmp") ? QStringLiteral("/usr/bin/zmp")
                                             : QString();
#endif
}

QString ZmpInstallDialog::md5OfFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return ztr("файл не найден");
    QCryptographicHash hash(QCryptographicHash::Md5);
    char buf[65536];
    qint64 n;
    while ((n = f.read(buf, sizeof(buf))) > 0)
        hash.addData(QByteArrayView(buf, static_cast<int>(n)));
    return QString::fromLatin1(hash.result().toHex());
}

// ---------------------------------------------------------------------------
//  Привилегированные операции: Linux — sudo -S с паролем (запрашивается один
//  раз), Windows — UAC через PowerShell Start-Process -Verb RunAs.
// ---------------------------------------------------------------------------

void ZmpInstallDialog::wipeSudoPassword()
{
    QByteArray raw = m_sudoPassword.toUtf8();
    std::memset(raw.data(), 0, static_cast<size_t>(raw.size()));
    m_sudoPassword.clear();
}

#ifdef Q_OS_WIN

bool ZmpInstallDialog::ensureSudoPassword() { return true; }

bool ZmpInstallDialog::runPrivileged(const QStringList &args)
{
    QString cmdLine;
    for (const QString &a : args)
        cmdLine += "\"" + a + "\" ";
    QProcess proc;
    proc.start("powershell", {"-NoProfile", "-Command",
        QStringLiteral("Start-Process -Verb RunAs -Wait -WindowStyle Hidden "
                       "cmd -ArgumentList '/c %1'").arg(cmdLine.trimmed())});
    if (!proc.waitForStarted(5000))
        return false;
    return proc.waitForFinished(120000) && proc.exitCode() == 0;
}

#else

bool ZmpInstallDialog::ensureSudoPassword()
{
    if (::getuid() == 0)
        return true;   // уже root
    if (!m_sudoPassword.isEmpty())
        return true;

    bool ok = false;
    m_sudoPassword = QInputDialog::getText(this, ztr("Установка ZMP"),
                                           ztr("sudo password:"),
                                           QLineEdit::Password, QString(), &ok);
    if (!ok || m_sudoPassword.isEmpty())
        return false;

    // Проверяем пароль сразу, чтобы не спросить посреди установки
    QProcess proc;
    proc.start("sudo", {"-S", "-v"});
    proc.waitForStarted(5000);
    proc.write(m_sudoPassword.toUtf8() + "\n");
    proc.closeWriteChannel();
    proc.waitForFinished(15000);
    if (proc.exitCode() != 0) {
        wipeSudoPassword();
        QMessageBox::critical(this, ztr("Ошибка"), ztr("Неверный пароль sudo"));
        return false;
    }
    return true;
}

bool ZmpInstallDialog::runPrivileged(const QStringList &args)
{
    if (::getuid() == 0) {
        QProcess proc;
        proc.start(args.first(), args.mid(1));
        if (!proc.waitForStarted(5000))
            return false;
        proc.waitForFinished(30000);
        return proc.exitCode() == 0;
    }

    QStringList full;
    full << "sudo" << "-S" << args;
    QProcess proc;
    proc.start(full.first(), full.mid(1));
    if (!proc.waitForStarted(5000))
        return false;
    proc.write(m_sudoPassword.toUtf8() + "\n");
    proc.closeWriteChannel();
    if (!proc.waitForFinished(30000))
        return false;
    return proc.exitCode() == 0;
}

#endif // Q_OS_WIN

// ---------------------------------------------------------------------------
//  Диалог установки/обновления
// ---------------------------------------------------------------------------

ZmpInstallDialog::ZmpInstallDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(mode == Mode::Update ? ztr("Обновление ZMP") : ztr("Установка ZMP"));
    setMinimumWidth(520);

    QVBoxLayout *layout = new QVBoxLayout(this);

    if (mode == Mode::Update) {
        const QString running = QFileInfo("/proc/self/exe").symLinkTarget();
        const QString installed = installedBinaryPath();
        const QString hRun = md5OfFile(running);
        const QString hInst = installed.isEmpty() ? QString("-") : md5OfFile(installed);

        m_hashInfoLabel = new QLabel(
            ztr("md5 хэш установщика (путь до zmp который запущен сейчас):") +
            "\n  " + (running.isEmpty() ? "?" : running) + " = " + hRun + "\n" +
            ztr("md5 хэш установленной версии (обычно /usr/bin/zmp):") +
            "\n  " + (installed.isEmpty() ? "?" : installed) + " = " + hInst);
        m_hashInfoLabel->setWordWrap(true);
        m_hashInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(m_hashInfoLabel);
    }

    layout->addWidget(new QLabel(ztr("Бинарный файл zmp будет добавлен в /usr/bin, "
                                     "а также в меню приложений")));

    layout->addWidget(new QLabel(ztr("Сделать ZMP плеером по умолчанию для:")));
    m_mp3Check  = new QCheckBox(".mp3");
    m_wavCheck  = new QCheckBox(".wav");
    m_alacCheck = new QCheckBox("ALAC (.m4a/.alac)");
    m_flacCheck = new QCheckBox(".flac");
    m_mp3Check->setChecked(true);
    QVBoxLayout *checks = new QVBoxLayout;
    checks->addWidget(m_mp3Check);
    checks->addWidget(m_wavCheck);
    checks->addWidget(m_alacCheck);
    checks->addWidget(m_flacCheck);
    layout->addLayout(checks);

    layout->addWidget(new QLabel(ztr("Как назвать zmp для запуска из терминала и меню приложений:")));
    m_nameEdit = new QLineEdit("zmp");
#ifdef Q_OS_WIN
    m_nameEdit->setEnabled(false);   // на Windows всегда zmp.exe
#endif
    layout->addWidget(m_nameEdit);

    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *cancel = new QPushButton(ztr("Отмена"));
    QPushButton *run = new QPushButton(ztr("Выполнить"));
    run->setStyleSheet("QPushButton { background-color: #2a82da; color: white;"
                       " font-weight: bold; padding: 6px 18px; border-radius: 4px; }");
    btns->addWidget(cancel);
    btns->addWidget(run);
    layout->addLayout(btns);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(run, &QPushButton::clicked, this, &ZmpInstallDialog::onExecute);
}

static void saveInstalledBinToConfig(const QString &name)
{
    QDir().mkpath(QDir::homePath() + "/zmp_playlists");
    const QString cfgPath = QDir::homePath() + "/zmp_playlists/config.json";
    QFile f(cfgPath);
    QJsonObject root;
    if (f.open(QIODevice::ReadOnly)) {
        root = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
    }
    root["installed_bin"] = name;
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
    }
}

void ZmpInstallDialog::onExecute()
{
#ifndef Q_OS_WIN
    if (!ensureSudoPassword())
        return;
#endif

#ifdef Q_OS_WIN
    // ------------------------- Windows 10/11 --------------------------------
    QString self = QCoreApplication::applicationFilePath();
    self = QDir::toNativeSeparators(self);

    const QString installDir = QStringLiteral("C:\\Program Files\\ZMP");
    const QString exeDst = installDir + QStringLiteral("\\zmp.exe");

    // Копирование в Program Files требует прав администратора — UAC-запрос
    {
        const QString shellCmd = QString(
            "if not exist \"%1\" mkdir \"%1\" && copy /Y \"%2\" \"%3\"")
            .arg(installDir, self, exeDst);
        QProcess proc;
        proc.start("powershell", {"-NoProfile", "-Command",
            QStringLiteral("Start-Process -Verb RunAs -Wait -WindowStyle Hidden "
                           "cmd -ArgumentList '/c %1'").arg(shellCmd)});
        if (!proc.waitForStarted(5000) ||
            !(proc.waitForFinished(120000) && proc.exitCode() == 0)) {
            QMessageBox::critical(this, ztr("Ошибка"),
                                  ztr("Не удалось скопировать бинарник в /usr/bin"));
            return;
        }
    }

    // Ярлык в меню Пуск и на рабочий стол (права администратора не нужны)
    auto makeShortcut = [](const QString &dir, const QString &exePath) {
        const QString workDir = QFileInfo(exePath).absolutePath();
        QProcess p;
        p.start("powershell", {"-NoProfile", "-Command",
            QStringLiteral(
                "$ws = New-Object -ComObject WScript.Shell; "
                "$s = $ws.CreateShortcut('%1\\ZMP.lnk'); "
                "$s.TargetPath = '%2'; $s.WorkingDirectory = '%3'; $s.Save()")
                .arg(QDir::toNativeSeparators(dir),
                     QDir::toNativeSeparators(exePath),
                     QDir::toNativeSeparators(workDir))});
        p.waitForFinished(15000);
    };
    makeShortcut(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation),
                 exeDst);
    makeShortcut(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                 exeDst);

    saveInstalledBinToConfig("zmp.exe");

    QMessageBox::information(this, ztr("Установка ZMP"),
                             ztr("Готово! Запуск:") + " \"ZMP\" " +
                             ztr("из терминала или из меню приложений"));
#else
    // ------------------------------ Linux ------------------------------------
    QString name = m_nameEdit->text().trimmed();
    name.replace(QRegularExpression("[^A-Za-z0-9_\\-]"), "");
    if (name.isEmpty())
        name = "zmp";

    QString self = QFileInfo("/proc/self/exe").symLinkTarget();
    if (self.isEmpty())
        self = QCoreApplication::applicationFilePath();

    if (!runPrivileged({"cp", "-f", self, "/usr/bin/" + name})) {
        QMessageBox::critical(this, ztr("Ошибка"),
                              ztr("Не удалось скопировать бинарник в /usr/bin"));
        wipeSudoPassword();
        return;
    }
    runPrivileged({"chmod", "755", "/usr/bin/" + name});

    QStringList mimeTypes;
    if (m_mp3Check->isChecked())  mimeTypes << "audio/mpeg";
    if (m_wavCheck->isChecked())  mimeTypes << "audio/x-wav" << "audio/wav";
    if (m_alacCheck->isChecked()) mimeTypes << "audio/x-m4a" << "audio/mp4";
    if (m_flacCheck->isChecked()) mimeTypes << "audio/flac" << "audio/x-flac";

    const QString desktopEntry =
        QString("[Desktop Entry]\n"
                "Type=Application\n"
                "Name=ZMP\n"
                "GenericName=Music Player\n"
                "Exec=%1 %U\n"
                "Terminal=false\n"
                "Categories=AudioVideo;Audio;Player;\n"
                "StartupWMClass=ZMP\n"
                "%2")
            .arg(name,
                 mimeTypes.isEmpty() ? QString()
                                     : "MimeType=" + mimeTypes.join(';') + "\n");

    const QString tmpDesktop = QDir::tempPath() + "/zmp_install.desktop";
    {
        QFile f(tmpDesktop);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(desktopEntry.toUtf8());
            f.close();
        }
    }
    runPrivileged({"cp", "-f", tmpDesktop,
                   "/usr/share/applications/" + name + ".desktop"});
    runPrivileged({"chmod", "644", "/usr/share/applications/" + name + ".desktop"});

    QProcess::startDetached("update-desktop-database", {});

    for (const QString &mime : mimeTypes)
        QProcess::execute("xdg-mime", {"default", name + ".desktop", mime});

    const QStringList desktopDirs = {
        QDir::homePath() + "/Desktop",
        QDir::homePath() + "/Рабочий стол",
        QDir::homePath() + "/Рабочий Стол",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
    };
    for (const QString &dir : desktopDirs) {
        if (dir.isEmpty() || !QDir(dir).exists())
            continue;
        const QString target = dir + "/" + name + ".desktop";
        QFile::copy(tmpDesktop, target);
        QFile::setPermissions(target,
                              QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner |
                              QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther);
    }

    saveInstalledBinToConfig(name);

    QMessageBox::information(this, ztr("Установка ZMP"),
                             ztr("Готово! Запуск:") + " \"" + name +
                             "\" " + ztr("из терминала или из меню приложений"));
#endif

    // Пароль стирается из памяти после завершения установки/обновления
    wipeSudoPassword();
    accept();
}
