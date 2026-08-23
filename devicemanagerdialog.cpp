#include "devicemanagerdialog.h"
#include "translator.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <QVBoxLayout>
#include <unistd.h>

static QString runCmd(const QString &prog, const QStringList &args)
{
    QProcess proc;
    proc.start(prog, args);
    if (!proc.waitForFinished(8000))
        return {};
    return QString::fromUtf8(proc.readAllStandardOutput());
}

QList<PaDeviceEntry> DeviceManagerDialog::listDevices() const
{
    QList<PaDeviceEntry> result;
    const bool wantInput = m_categoryCombo->currentIndex() == 0;

    // wpctl status: секции "Sinks:", "Sources:" и "Devices:" (карты).
    // Выключенные устройства (профиль off) исчезают из Sinks/Sources,
    // но остаются видны как карты в секции Devices — показываем и их.
    const QString status = runCmd("wpctl", {"status"});
    const QStringList lines = status.split('\n');

    enum Section { None, Cards, Sinks, Sources } section = None;
    static const QRegularExpression entryRe("(\\d+)\\.\\s+([^\\[]*?)\\s*(\\[|$)");
    for (const QString &line : lines) {
        if (line.contains("Sinks:", Qt::CaseSensitive)) { section = Sinks; continue; }
        if (line.contains("Sources:", Qt::CaseSensitive)) { section = Sources; continue; }
        // Важно: до проверки "Devices:", т.к. строка содержит слово "Devices:"
        if (line.contains("Default Configured Devices:")) { section = None; continue; }
        if (line.contains("Devices:", Qt::CaseSensitive)) { section = Cards; continue; }
        if (line.contains("Clients:") || line.contains("Filters:")) {
            section = None;
            continue;
        }
        if (section == None)
            continue;

        auto m = entryRe.match(line);
        if (!m.hasMatch())
            continue;

        QString name = m.captured(2).trimmed();
        if (name.startsWith("zmp_"))
            continue;   // служебные устройства ZMP

        PaDeviceEntry e;
        e.id = m.captured(1).toInt();

        if (section == Cards) {
            // Карта видна даже когда выключена (профиль off)
            e.name = ztr("[карта, возможно выключена]") + " " + name;
            e.isCard = true;
            result.append(e);
            continue;
        }

        if (name.contains(".monitor"))
            continue;   // внутренние мониторы не показываем

        const bool isInput = (section == Sources);
        if (isInput != wantInput)
            continue;

        e.name = name;
        e.isInput = isInput;
        result.append(e);
    }
    return result;
}

DeviceManagerDialog::DeviceManagerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(ztr("Менеджер устройств"));
    setMinimumSize(560, 480);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *top = new QHBoxLayout;
    top->addWidget(new QLabel(ztr("Категория:")));
    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItem(ztr("Ввод"));
    m_categoryCombo->addItem(ztr("Вывод"));
    top->addWidget(m_categoryCombo, 1);
    layout->addLayout(top);

    m_list = new QListWidget;
    layout->addWidget(m_list, 1);

    m_statusLabel = new QLabel;
    layout->addWidget(m_statusLabel);

    QHBoxLayout *btnRow = new QHBoxLayout;
    QPushButton *destroyBtn = new QPushButton(ztr("Уничтожить"));
    destroyBtn->setStyleSheet(
        "QPushButton { background-color: #7b1fa2; color: white; font-weight: bold;"
        " padding: 6px 14px; border-radius: 4px; }");
    QPushButton *raskBtn = new QPushButton(ztr("Раскулачить"));
    raskBtn->setStyleSheet(
        "QPushButton { background-color: #ef6c00; color: white; font-weight: bold;"
        " padding: 6px 14px; border-radius: 4px; }");
    btnRow->addWidget(destroyBtn, 1);
    btnRow->addWidget(raskBtn, 1);
    layout->addLayout(btnRow);

    connect(m_categoryCombo, &QComboBox::currentIndexChanged, this,
            [this](int) { refreshList(); });
    connect(destroyBtn, &QPushButton::clicked, this, &DeviceManagerDialog::onDestroyClicked);
    connect(raskBtn, &QPushButton::clicked, this, &DeviceManagerDialog::onRaskulachitClicked);

    refreshList();
}

void DeviceManagerDialog::refreshList()
{
    m_list->clear();
    for (const PaDeviceEntry &e : listDevices()) {
        QListWidgetItem *item = new QListWidgetItem(
            QString("%1 — %2").arg(e.id).arg(e.name), m_list);
        item->setData(Qt::UserRole, e.id);
        item->setData(Qt::UserRole + 1, e.isInput);
        item->setData(Qt::UserRole + 2, e.isCard);
    }
}

void DeviceManagerDialog::onDestroyClicked()
{
    QListWidgetItem *item = m_list->currentItem();
    if (!item) {
        QMessageBox::information(this, ztr("Менеджер устройств"),
                                 ztr("Выберите устройство в списке"));
        return;
    }
    const int id = item->data(Qt::UserRole).toInt();
    if (item->data(Qt::UserRole + 2).toBool()) {
        QMessageBox::information(this, ztr("Менеджер устройств"),
                                 ztr("Карту (аппаратное устройство) уничтожить нельзя — только отключить"));
        return;
    }
    QProcess proc;
    proc.start("pw-cli", {"destroy", QString::number(id)});
    proc.waitForFinished(8000);
    if (proc.exitCode() == 0)
        m_statusLabel->setText(ztr("Уничтожено устройство") + " #" + QString::number(id));
    else
        m_statusLabel->setText(ztr("Не удалось уничтожить (аппаратные устройства защищены системой)"));
    QTimer::singleShot(700, this, &DeviceManagerDialog::refreshList);
}

void DeviceManagerDialog::onRaskulachitClicked()
{
    QListWidgetItem *item = m_list->currentItem();
    if (!item) {
        QMessageBox::information(this, ztr("Менеджер устройств"),
                                 ztr("Выберите устройство в списке"));
        return;
    }
    const int id = item->data(Qt::UserRole).toInt();
    const bool isInput = item->data(Qt::UserRole + 1).toBool();
    const bool isCard = item->data(Qt::UserRole + 2).toBool();

    QDialog menu(this);
    menu.setWindowTitle(ztr("Раскулачить") + " #" + QString::number(id));
    QVBoxLayout *l = new QVBoxLayout(&menu);

    auto addBtn = [&l](const QString &text, const char *color) {
        QPushButton *b = new QPushButton(text);
        b->setStyleSheet(QString("QPushButton { text-align:left; padding: 8px;"
                                 " border: 1px solid %1; border-radius: 4px; }")
                             .arg(color));
        l->addWidget(b);
        return b;
    };

    // Состояние "изменённой работы" храним в самом диалоге
    QPushButton *offBtn = addBtn(ztr("Отключить"), "#888");
    QPushButton *onBtn = addBtn(ztr("Включить"), "#888");
    QPushButton *swapBtn = addBtn(ztr("Изменить работу"),
                                  m_swappedNodes.contains(id) ? "#4CAF50" : "#ffffff");
    QPushButton *defBtn = addBtn(ztr("Назначить стандартным"), "#2a82da");
    QPushButton *killBtn = addBtn(ztr("Уничтожить"), "#7b1fa2");
    QPushButton *freeBtn = addBtn(ztr("Освободить"), "#d32f2f");

    auto setStatus = [this](const QString &s) { m_statusLabel->setText(s); };
    auto findCardId = [id, isCard]() -> int {
        if (isCard)
            return id;   // выбранная запись — сама карта
        const QString inspect = runCmd("wpctl", {"inspect", QString::number(id)});
        static const QRegularExpression re("device\\.id\\s*=\\s*\"?(\\d+)\"?");
        auto m = re.match(inspect);
        return m.hasMatch() ? m.captured(1).toInt() : -1;
    };

    connect(offBtn, &QPushButton::clicked, &menu, [&]() {
        int card = findCardId();
        if (card < 0 || QProcess::execute("wpctl",
                {"set-profile", QString::number(card), "0"}) != 0) {
            setStatus(ztr("Не удалось отключить (виртуальные устройства не отключаются)"));
        } else {
            setStatus(ztr("Отключено: профиль off для карты") + " " + QString::number(card));
        }
        menu.accept();
    });

    connect(onBtn, &QPushButton::clicked, &menu, [&]() {
        int card = findCardId();
        if (card < 0 || QProcess::execute("wpctl",
                {"set-profile", QString::number(card), "1"}) != 0) {
            setStatus(ztr("Не удалось включить"));
        } else {
            setStatus(ztr("Включено: базовый профиль карты") + " " + QString::number(card));
        }
        menu.accept();
    });

    // Изменить работу: микрофон принудительно звучит как динамик
    // (loopback на текущий sink по умолчанию); динамик становится микрофоном.
    connect(swapBtn, &QPushButton::clicked, &menu,
            [this, id, isInput, swapBtn, setStatus, &menu]() {
        if (m_swappedNodes.contains(id)) {
            m_swappedNodes.remove(id);
            swapBtn->setStyleSheet("QPushButton { text-align:left; padding: 8px;"
                                   " border: 1px solid #ffffff; border-radius: 4px; }");
            setStatus(ztr("Работа восстановлена (частично — модули loopback остаются до перезапуска)"));
            menu.accept();
            return;
        }
        m_swappedNodes.insert(id);
        swapBtn->setStyleSheet("QPushButton { text-align:left; padding: 8px;"
                               " background-color:#4CAF50; color:white;"
                               " border: 1px solid #4CAF50; border-radius: 4px; }");

        if (isInput) {
            // Микрофон как динамик: источник -> текущий sink по умолчанию
            const QString defSink = runCmd("pactl", {"get-default-sink"}).trimmed();
            const QString srcName = runCmd("wpctl", {"inspect", QString::number(id)});
            static const QRegularExpression nameRe("node\\.name\\s*=\\s*\"([^\"]+)\"");
            auto m = nameRe.match(srcName);
            if (m.hasMatch() && !defSink.isEmpty()) {
                runCmd("pactl", {"load-module", "module-loopback",
                                 QString("source=%1 sink=%2").arg(m.captured(1), defSink)});
            }
        } else {
            // Динамик как микрофон: монитор sink -> виртуальный источник -> default
            const QString defSink = runCmd("pactl", {"get-default-sink"}).trimmed();
            const QString srcName = QString("zmp_swapped_%1").arg(id);
            if (!defSink.isEmpty()) {
                runCmd("pactl", {"load-module", "module-remap-source",
                                 QString("source_name=%1 master=%2.monitor").arg(srcName, defSink)});
                QProcess::execute("pactl", {"set-default-source", srcName});
            }
        }
        setStatus(ztr("Роль устройства изменена"));
        menu.accept();
    });

    connect(defBtn, &QPushButton::clicked, &menu, [&]() {
        if (QProcess::execute("wpctl", {"set-default", QString::number(id)}) == 0)
            setStatus(ztr("Назначено стандартным") + ": #" + QString::number(id));
        else
            setStatus(ztr("Не удалось назначить стандартным"));
        menu.accept();
    });

    connect(killBtn, &QPushButton::clicked, &menu, [&]() {
        onDestroyClicked();
        menu.accept();
    });

    // Освободить: завершить процессы, держащие устройство. Если это сам ZMP —
    // предупреждение об угрозе самоуничтожения.
    connect(freeBtn, &QPushButton::clicked, &menu, [this, isInput, &menu, setStatus]() {
        const QString dirArgs = isInput ? "/dev/snd/pcmC*D*c" : "/dev/snd/pcmC*D*p";
        QProcess fuserProc;
        fuserProc.start("fuser", {"-v", dirArgs});
        fuserProc.waitForFinished(5000);
        const QString out = QString::fromUtf8(fuserProc.readAllStandardOutput());

        QSet<qint64> pids;
        static const QRegularExpression numRe("\\b(\\d{2,7})\\b");
        auto it = numRe.globalMatch(out);
        while (it.hasNext()) {
            auto m = it.next();
            qint64 pid = m.captured(1).toLongLong();
            if (pid > 1)
                pids.insert(pid);
        }

        if (pids.isEmpty()) {
            setStatus(ztr("Процессы, использующие устройство, не найдены"));
            menu.accept();
            return;
        }
        if (pids.contains(static_cast<qint64>(getpid()))) {
            QMessageBox warn(this);
            warn.setWindowTitle(ztr("Угроза самоуничтожения"));
            warn.setText(ztr("Устройство используется самим ZMP. Всё равно снять процесс?"));
            QPushButton *cancel = warn.addButton(ztr("Отмена"), QMessageBox::RejectRole);
            QPushButton *killMe = warn.addButton(ztr("Завершить процесс"), QMessageBox::AcceptRole);
            warn.exec();
            if (warn.clickedButton() != killMe)
                return;
            menu.accept();
            QCoreApplication::quit();
            return;
        }

        QStringList pidStrs;
        for (qint64 pid : pids) {
            QProcess::startDetached("kill", {QString::number(pid)});
            pidStrs << QString::number(pid);
        }
        setStatus(ztr("Завершены процессы:") + " " + pidStrs.join(", "));
        menu.accept();
    });

    menu.exec();

    refreshList();
}
