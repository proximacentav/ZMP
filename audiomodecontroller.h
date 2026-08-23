#ifndef AUDIOMODECONTROLLER_H
#define AUDIOMODECONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct MicInfo {
    QString paName;        // внутреннее имя для pactl
    QString description;   // человекочитаемое описание
};

// Управление специальными режимами вывода звука:
//  - перехват микрофона (музыка микшируется в реальный микрофон);
//  - виртуальный микрофон (чистая музыка в новый микрофон с заданным именем).
//
// Реализовано поверх PipeWire/pipewire-pulse (pactl-модули):
//   null-sink -> loopback(mic) -> remap-source -> set-default-source,
// а поток ZMP перемещается в служебный sink через move-sink-input.
class AudioModeController : public QObject
{
    Q_OBJECT
public:
    static AudioModeController *instance();

    // Список источников (микрофонов) из pactl, без .monitor и служебных zmp_
    QList<MicInfo> listMicrophones() const;

    // Перехват: музыка принудительно микшируется в выбранный микрофон,
    // даже если им уже пользуются (loopback читает источник параллельно).
    bool startIntercept(const QString &paSourceName);

    // Виртуальный микрофон с произвольным именем; в него играет только ZMP.
    bool createVirtualMic(const QString &name);

    // Вернуть всё как было (выгрузить модули, восстановить источник по умолчанию)
    void reset();

signals:
    void errorOccurred(const QString &message);

private:
    explicit AudioModeController(QObject *parent = nullptr);

    static QString runPactl(const QStringList &args);
    int loadModule(const QString &moduleArgs);
    void unloadModules();
    void rememberDefaultSource();
    bool moveOurStreamToSink(const QString &sinkName);
    int moveCaptureStreamsFromSource(const QString &paSourceName,
                                     const QString &targetSource);
    qint64 ourPid() const;

    QVector<int> m_modules;
    QString m_prevDefaultSource;
    int m_virtualCounter = 0;
};

#endif // AUDIOMODECONTROLLER_H
