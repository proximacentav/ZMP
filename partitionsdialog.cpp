#include "partitionsdialog.h"
#include "translator.h"

#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStandardPaths>
#include <QFileInfo>
#include <QVBoxLayout>

#include <unistd.h>

PartitionsDialog::PartitionsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(ztr("Разделы"));
    setMinimumSize(1280, 520);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->addWidget(new QLabel(ztr("Диски и разделы (lsblk):")));

    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    QWidget *rowsWidget = new QWidget;
    m_rowsLayout = new QVBoxLayout(rowsWidget);
    m_rowsLayout->setContentsMargins(2, 0, 2, 0);
    m_rowsLayout->setSpacing(1);
    m_rowsLayout->addStretch();
    scroll->setWidget(rowsWidget);
    mainLayout->addWidget(scroll, 1);

    m_statusLabel = new QLabel;
    mainLayout->addWidget(m_statusLabel);

    QPushButton *refreshBtn = new QPushButton(ztr("Обновить"));
    connect(refreshBtn, &QPushButton::clicked, this, &PartitionsDialog::refresh);
    mainLayout->addWidget(refreshBtn);

    refresh();
}

void PartitionsDialog::setRootCredentials(bool rootMode, const QString &password)
{
    m_rootMode = rootMode;
    m_rootPassword = password;
}

QString PartitionsDialog::humanSize(qint64 bytes) const
{
    if (bytes >= 1073741824LL)
        return QString("%1 ГБ").arg(bytes / 1073741824.0, 0, 'f', 1);
    if (bytes >= 1048576)
        return QString("%1 МБ").arg(bytes / 1048576.0, 0, 'f', 1);
    return QString("%1 КБ").arg(bytes / 1024.0, 0, 'f', 1);
}

QList<PartitionInfo> PartitionsDialog::listPartitions() const
{
    QList<PartitionInfo> result;

    QProcess proc;
    proc.start("lsblk", {"-b", "-P",
                         "-o", "NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT,LABEL,MODEL"});
    if (!proc.waitForFinished(5000))
        return result;

    // lsblk -P выдаёт пары KEY="value" — надёжно парсится по регулярке
    static QRegularExpression re(R"((\w+)=\"([^\"]*)\")");
    const QStringList lines = QString::fromUtf8(proc.readAllStandardOutput())
                                  .split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        PartitionInfo p;
        auto it = re.globalMatch(line);
        while (it.hasNext()) {
            auto m = it.next();
            const QString key = m.captured(1);
            const QString val = m.captured(2);
            if (key == "NAME") p.device = "/dev/" + val;
            else if (key == "SIZE") p.sizeBytes = val.toLongLong();
            else if (key == "TYPE") p.type = val;
            else if (key == "FSTYPE") p.fstype = val;
            else if (key == "MOUNTPOINT") p.mountPoint = val;
            else if (key == "LABEL") p.label = val;
            else if (key == "MODEL") p.model = val;
        }

        // Пропускаем служебные устройства
        if (p.type == "loop" || p.type == "ram" || p.device.isEmpty())
            continue;

        result.append(p);
    }
    return result;
}

QString PartitionsDialog::privPrefix() const
{
    if (::getuid() == 0)
        return {};
    if (m_rootMode && !QStandardPaths::findExecutable("sudo").isEmpty())
        return "sudo";
    if (!QStandardPaths::findExecutable("pkexec").isEmpty())
        return "pkexec";
    if (!QStandardPaths::findExecutable("sudo").isEmpty())
        return "sudo";
    return {};
}

// Запуск команды с повышением прав. В root-режиме ZMP пароль подставляется
// через sudo -S — повторная авторизация не требуется.
bool PartitionsDialog::execPrivileged(const QStringList &args, QString *errOut)
{
    QStringList full;
    const QString prefix = privPrefix();
    const bool useSudoPassword = (prefix == "sudo" && m_rootMode);
    if (!prefix.isEmpty()) full << prefix;
    full << args;

    QProcess proc;
    proc.start(full.first(), full.mid(1));
    if (!proc.waitForStarted(5000)) {
        if (errOut) *errOut = ztr("Не удалось запустить команду");
        return false;
    }
    if (useSudoPassword)
        proc.write((m_rootPassword + "\n").toUtf8());
    if (!proc.waitForFinished(30000)) {
        if (errOut) *errOut = ztr("Таймаут выполнения");
        return false;
    }
    if (errOut)
        *errOut = QString::fromUtf8(proc.readAllStandardError()).trimmed();
    return proc.exitCode() == 0;
}

bool PartitionsDialog::runMount(const PartitionInfo &p, const QString &dir, QString *errOut)
{
    const bool hasRoot = ::getuid() == 0 || !privPrefix().isEmpty();

    QStringList args;
    args << "mount";

    // Если каталог создать не удалось обычным способом — пробуем --mkdir от root
    if (hasRoot && !QDir(dir).exists())
        args << "--mkdir";

    args << p.device << dir;

    if (!execPrivileged(args, errOut)) {
        // Старые util-linux могут не знать --mkdir — пробуем без него
        if (args.contains("--mkdir")) {
            args.removeAll("--mkdir");
            return execPrivileged(args, errOut);
        }
        return false;
    }
    return true;
}

bool PartitionsDialog::runUnmount(const PartitionInfo &p, QString *errOut)
{
    QString target = p.mountPoint.isEmpty() ? p.device : p.mountPoint;
    return execPrivileged({"umount", target}, errOut);
}

QWidget *PartitionsDialog::makeRow(const PartitionInfo &p)
{
    QWidget *row = new QWidget;
    QHBoxLayout *lay = new QHBoxLayout(row);
    lay->setContentsMargins(2, 1, 2, 1);
    lay->setSpacing(4);

    QString desc = QString("%1  %2").arg(p.device, humanSize(p.sizeBytes));
    if (!p.fstype.isEmpty()) desc += "  [" + p.fstype + "]";
    if (!p.label.isEmpty()) desc += "  \"" + p.label + "\"";
    if (!p.model.isEmpty()) desc += "  (" + p.model + ")";

    QLabel *lbl = new QLabel(desc);
    lbl->setToolTip(p.mountPoint);
    lay->addWidget(lbl, 1);

    const bool mounted = !p.mountPoint.isEmpty();

    if (mounted) {
        QLabel *mp = new QLabel(ztr("примонтирован в ") + p.mountPoint);
        mp->setStyleSheet("color: #888;");
        lay->addWidget(mp);

        QPushButton *gotoBtn = new QPushButton(ztr("Перейти в точку монтирования"));
        gotoBtn->setFixedHeight(26);
        gotoBtn->setStyleSheet(
            "QPushButton { background-color: #2a82da; color: white; font-weight: bold;"
            " padding: 2px 8px; border-radius: 4px; }");
        const QString targetPath = p.mountPoint;
        connect(gotoBtn, &QPushButton::clicked, this, [this, targetPath]() {
            emit openPathRequested(targetPath);
            accept();
        });
        lay->addWidget(gotoBtn);

        QPushButton *umountBtn = new QPushButton(ztr("ОТКЛЮЧИТЬ"));
        umountBtn->setFixedHeight(26);
        umountBtn->setStyleSheet(
            "QPushButton { background-color: #d32f2f; color: white; font-weight: bold;"
            " padding: 2px 8px; border-radius: 4px; }");
        connect(umountBtn, &QPushButton::clicked, this, [this, p]() {
            QString err;

            // Корень системы — отдельное предупреждение
            if (p.mountPoint == "/") {
                QMessageBox warn(this);
                warn.setWindowTitle(ztr("Системный раздел"));
                warn.setText(ztr("Этот диск возможно системный раздел. Демонтировать?"));
                QPushButton *cancel = warn.addButton(ztr("Отмена"), QMessageBox::RejectRole);
                QPushButton *off = warn.addButton(ztr("Отключить"), QMessageBox::AcceptRole);
                warn.exec();
                if (warn.clickedButton() == cancel || warn.clickedButton() != off)
                    return;
            }

            if (runUnmount(p, &err)) {
                m_statusLabel->setText(ztr("Демонтировано:") + " " + p.device);
                refresh();
                return;
            }

            // Занят — предупреждение об опасности
            bool busy = err.contains("busy", Qt::CaseInsensitive) ||
                        err.contains("EBUSY", Qt::CaseInsensitive);
            if (busy) {
                QMessageBox warn(this);
                warn.setWindowTitle(ztr("Диск используется"));
                warn.setText(ztr("Этот диск сейчас используется. Демонтирование опасно. Продолжить?"));
                QPushButton *cancel = warn.addButton(ztr("Отмена"), QMessageBox::RejectRole);
                QPushButton *cont = warn.addButton(ztr("Продолжить"), QMessageBox::AcceptRole);
                warn.exec();
                if (warn.clickedButton() != cont)
                    return;

                // Ленивое демонтирование
                QString lazyErr;
                if (execPrivileged({"umount", "-l",
                                    p.mountPoint.isEmpty() ? p.device : p.mountPoint},
                                   &lazyErr)) {
                    m_statusLabel->setText(ztr("Демонтировано (lazy):") + " " + p.device);
                    refresh();
                } else {
                    m_statusLabel->setText(ztr("Не удалось демонтировать:") + " " + lazyErr);
                }
            } else {
                m_statusLabel->setText(ztr("Не удалось демонтировать:") + " " + err);
            }
        });
        lay->addWidget(umountBtn);
    } else {
        QLabel *mp = new QLabel(ztr("не примонтирован"));
        mp->setStyleSheet("color: #888;");
        lay->addWidget(mp);

        const bool mountable = (p.type == "part" || p.type == "crypt" || p.type == "lvm");
        if (mountable) {
            QPushButton *mountBtn = new QPushButton(ztr("Подключить"));
            mountBtn->setFixedHeight(26);
            connect(mountBtn, &QPushButton::clicked, this, [this, p]() {
                QString def = "/mnt/" + QFileInfo(p.device).fileName();
                QString dir = QInputDialog::getText(this, ztr("Подключить раздел"),
                                                    ztr("Точка монтирования:"),
                                                    QLineEdit::Normal, def);
                dir = dir.trimmed();
                if (dir.isEmpty())
                    return;

                if (!QDir(dir).exists()) {
                    QMessageBox ask(this);
                    ask.setWindowTitle(ztr("Нет директории"));
                    ask.setText(ztr("Директории для монтирования нет:") + "\n" + dir);
                    QPushButton *cancel = ask.addButton(ztr("Отмена"), QMessageBox::RejectRole);
                    QPushButton *mk = ask.addButton(ztr("Создать (--mkdir)"), QMessageBox::AcceptRole);
                    ask.exec();
                    if (ask.clickedButton() != mk)
                        return;
                    if (!QDir().mkpath(dir)) {
                        // Не смогли создать сами — оставляем mount --mkdir сделать это от root
                        qDebug() << "mkdir failed for" << dir << ", will use mount --mkdir";
                    }
                }

                QString err;
                if (runMount(p, dir, &err)) {
                    m_statusLabel->setText(ztr("Примонтировано:") + " " + p.device + " -> " + dir);
                    refresh();
                } else {
                    m_statusLabel->setText(ztr("Ошибка монтирования:") + " " +
                                           (err.isEmpty() ? ztr("неизвестная ошибка") : err));
                }
            });
            lay->addWidget(mountBtn);
        }
    }

    return row;
}

void PartitionsDialog::refresh()
{
    // Очищаем строки (кроме stretch)
    while (m_rowsLayout->count() > 1) {
        QLayoutItem *item = m_rowsLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    const QList<PartitionInfo> parts = listPartitions();
    if (parts.isEmpty()) {
        m_rowsLayout->insertWidget(0, new QLabel(ztr("lsblk не вернул данных")));
        return;
    }

    for (int i = 0; i < parts.size(); ++i)
        m_rowsLayout->insertWidget(i, makeRow(parts.at(i)));
}
