#include "audiomodecontroller.h"
#include "translator.h"

#include <QApplication>
#include <QProcess>
#include <QRegularExpression>
#include <unistd.h>

AudioModeController *AudioModeController::instance()
{
    static AudioModeController inst(qApp);
    return &inst;
}

AudioModeController::AudioModeController(QObject *parent)
    : QObject(parent)
{
    // При выходе из ZMP виртуальные микрофоны отключаются
    connect(qApp, &QCoreApplication::aboutToQuit, this, &AudioModeController::reset);
}

qint64 AudioModeController::ourPid() const
{
    return static_cast<qint64>(getpid());
}

QString AudioModeController::runPactl(const QStringList &args)
{
    QProcess proc;
    proc.start("pactl", args);
    if (!proc.waitForFinished(2000))
        return {};
    return QString::fromUtf8(proc.readAllStandardOutput());
}

QList<MicInfo> AudioModeController::listMicrophones() const
{
    QList<MicInfo> result;

    // Полный формат: блоки "Source #N" с Name:/Description: — даёт
    // человекочитаемые описания (например "USB PNP SOUND DEVICE")
    const QString out = runPactl({"list", "sources"});
    const QStringList blocks = out.split(QRegularExpression("^Source #",
                                         QRegularExpression::MultilineOption),
                                         Qt::SkipEmptyParts);
    for (const QString &block : blocks) {
        static const QRegularExpression nameRe("^\\s*Name:\\s*(.+)$",
                                               QRegularExpression::MultilineOption);
        static const QRegularExpression descRe("^\\s*Description:\\s*(.+)$",
                                               QRegularExpression::MultilineOption);
        auto nm = nameRe.match(block);
        if (!nm.hasMatch())
            continue;
        const QString name = nm.captured(1).trimmed();
        if (name.endsWith(".monitor"))
            continue;
        if (name.startsWith("zmp_"))
            continue;   // наши служебные источники

        MicInfo info;
        info.paName = name;
        auto dm = descRe.match(block);
        info.description = dm.hasMatch() ? dm.captured(1).trimmed() : name;
        result.append(info);
    }
    return result;
}

int AudioModeController::loadModule(const QString &moduleArgs)
{
    QProcess proc;
    proc.start("pactl", QStringList{"load-module"} << moduleArgs.split(' ', Qt::SkipEmptyParts));
    if (!proc.waitForFinished(2000))
        return -1;
    bool ok = false;
    const int idx = QString::fromUtf8(proc.readAllStandardOutput()).trimmed().toInt(&ok);
    if (ok && idx > 0) {
        m_modules.append(idx);
        return idx;
    }
    emit errorOccurred(ztr("Не удалось загрузить модуль") + ": " + moduleArgs);
    return -1;
}

void AudioModeController::unloadModules()
{
    for (int i = m_modules.size() - 1; i >= 0; --i) {
        QProcess proc;
        proc.start("pactl", {"unload-module", QString::number(m_modules.at(i))});
        proc.waitForFinished(5000);
    }
    m_modules.clear();
}

void AudioModeController::rememberDefaultSource()
{
    if (!m_prevDefaultSource.isEmpty())
        return;   // уже запомнили
    m_prevDefaultSource = runPactl({"get-default-source"}).trimmed();
}

bool AudioModeController::moveOurStreamToSink(const QString &sinkName)
{
    // Ищем sink-input'ы процесса ZMP и перемещаем их в целевой sink
    const QString out = runPactl({"list", "sink-inputs"});
    static const QRegularExpression blockRe("Sink Input #(\\d+)");
    const QStringList blocks = out.split(QRegularExpression("^Sink Input #",
                                         QRegularExpression::MultilineOption),
                                         Qt::SkipEmptyParts);

    bool movedAny = false;
    qint64 pid = ourPid();
    for (const QString &block : blocks) {
        auto m = blockRe.match("Sink Input #" + block);
        if (!m.hasMatch())
            continue;

        const bool isOurs =
            block.contains(QString::number(pid)) ||
            block.contains("ZMP_Linux_bin", Qt::CaseInsensitive);
        if (!isOurs)
            continue;

        QProcess proc;
        proc.start("pactl", {"move-sink-input", m.captured(1), sinkName});
        proc.waitForFinished(5000);
        movedAny = true;
    }
    return movedAny;
}

bool AudioModeController::startIntercept(const QString &paSourceName)
{
    if (paSourceName.isEmpty())
        return false;

    reset();
    rememberDefaultSource();

    // Описание оригинального микрофона — комбинированный источник будет
    // выглядеть в приложениях так же, как оригинал
    QString origDesc = paSourceName;
    {
        const QString out = runPactl({"list", "sources"});
        const QStringList blocks = out.split(QRegularExpression("^Source #",
                                             QRegularExpression::MultilineOption),
                                             Qt::SkipEmptyParts);
        static const QRegularExpression nameRe("^\\s*Name:\\s*(.+)$",
                                               QRegularExpression::MultilineOption);
        static const QRegularExpression descRe("^\\s*Description:\\s*(.+)$",
                                               QRegularExpression::MultilineOption);
        for (const QString &block : blocks) {
            auto nm = nameRe.match(block);
            if (nm.hasMatch() && nm.captured(1).trimmed() == paSourceName) {
                auto dm = descRe.match(block);
                if (dm.hasMatch())
                    origDesc = dm.captured(1).trimmed();
                break;
            }
        }
    }
    const QString escDesc = QString(origDesc).replace('"', "'");

    // 1. Служебный sink — точка смешивания музыки и голоса
    if (loadModule("module-null-sink sink_name=zmp_intercept_sink "
                   "sink_properties=device.description=ZMP_Intercept_Mix") < 0)
        return false;

    // 2. Реальный микрофон подмешивается в этот sink параллельно с его
    //    обычными потребителями — никто не лишается микрофона.
    loadModule(QString("module-loopback source=%1 sink=zmp_intercept_sink")
                   .arg(paSourceName));

    // 3. Смешанный источник = содержимое mix-sink (голос + музыка),
    //    с описанием оригинального микрофона
    loadModule(QString("module-remap-source source_name=zmp_intercept_src "
                       "master=zmp_intercept_sink.monitor "
                       "source_properties=\"device.description='%1'\"").arg(escDesc));

    // 4. Делаем смешанный источник стандартным
    QProcess::execute("pactl", {"set-default-source", "zmp_intercept_src"});

    // 5. Перенаправляем музыку ZMP в mix-sink
    moveOurStreamToSink("zmp_intercept_sink");

    // 6. Все приложения, слушавшие оригинальный микрофон, переключаются на
    //    смешанный источник — они получают голос + музыку, как будто звук
    //    добавлен прямо в оригинальный микрофон.
    moveCaptureStreamsFromSource(paSourceName, "zmp_intercept_src");
    return true;
}

int AudioModeController::moveCaptureStreamsFromSource(const QString &paSourceName,
                                                      const QString &targetSource)
{
    // Индекс источника по имени (для сравнения с полем "Source:" у source-output)
    int sourceIndex = -1;
    {
        const QString out = runPactl({"list", "sources"});
        const QStringList blocks = out.split(QRegularExpression("^Source #",
                                             QRegularExpression::MultilineOption),
                                             Qt::SkipEmptyParts);
        static const QRegularExpression idxRe("^(\\d+)");
        static const QRegularExpression nameRe("^\\s*Name:\\s*(.+)$",
                                               QRegularExpression::MultilineOption);
        for (const QString &block : blocks) {
            auto im = idxRe.match(block);
            auto nm = nameRe.match(block);
            if (im.hasMatch() && nm.hasMatch() &&
                nm.captured(1).trimmed() == paSourceName) {
                sourceIndex = im.captured(1).toInt();
                break;
            }
        }
    }
    if (sourceIndex < 0)
        return 0;

    // Перемещаем все потоки захвата, слушающие этот источник
    const QString out = runPactl({"list", "source-outputs"});
    const QStringList blocks = out.split(QRegularExpression("^Source Output #",
                                         QRegularExpression::MultilineOption),
                                         Qt::SkipEmptyParts);
    static const QRegularExpression idxRe("^(\\d+)");
    static const QRegularExpression srcRe("^\\s*Source:\\s*(\\d+)\\s*$",
                                          QRegularExpression::MultilineOption);

    int moved = 0;
    for (const QString &block : blocks) {
        auto im = idxRe.match(block);
        auto sm = srcRe.match(block);
        if (!im.hasMatch() || !sm.hasMatch())
            continue;
        if (sm.captured(1).toInt() != sourceIndex)
            continue;

        QProcess proc;
        proc.start("pactl", {"move-source-output", im.captured(1), targetSource});
        proc.waitForFinished(5000);
        ++moved;
    }
    return moved;
}

bool AudioModeController::createVirtualMic(const QString &name)
{
    const QString safeName = name.trimmed().isEmpty()
                                 ? QString("ZMP Virtual %1").arg(++m_virtualCounter)
                                 : name.trimmed();
    ++m_virtualCounter;

    const QString escName = QString(safeName).replace('"', "'");
    const int n = m_virtualCounter;
    const QString sinkName = QString("zmp_virtual_sink_%1").arg(n);
    const QString srcName = QString("zmp_virtual_src_%1").arg(n);

    rememberDefaultSource();

    if (loadModule(QString("module-null-sink sink_name=%1 "
                           "sink_properties=\"device.description='%2'\"")
                       .arg(sinkName, escName)) < 0)
        return false;

    loadModule(QString("module-remap-source source_name=%1 master=%2.monitor "
                       "source_properties=\"device.description='%2'\"")
                   .arg(srcName, sinkName, escName));

    // Музыка ZMP идёт в виртуальный микрофон
    moveOurStreamToSink(sinkName);
    return true;
}

void AudioModeController::reset()
{
    unloadModules();
    if (!m_prevDefaultSource.isEmpty()) {
        QProcess::execute("pactl", {"set-default-source", m_prevDefaultSource});
        m_prevDefaultSource.clear();
    }
}
