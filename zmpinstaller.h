#ifndef ZMPINSTALLER_H
#define ZMPINSTALLER_H

#include <QDialog>
#include <QString>
#include <QStringList>

class QLineEdit;
class QCheckBox;
class QLabel;

// Проверка: установлен ли бинарник ZMP в /usr/bin
// (учитывается нестандартное имя из прошлого раза)
bool zmpInstalledAsApp();

// Диалог установки/обновления: копирование бинарника в /usr/bin, ярлык в меню
// приложений и на рабочий стол, ассоциация с аудиоформатами.
class ZmpInstallDialog : public QDialog
{
    Q_OBJECT
public:
    enum class Mode { Install, Update };

    explicit ZmpInstallDialog(Mode mode = Mode::Install, QWidget *parent = nullptr);

    // Путь к установленной версии (/usr/bin/<имя из конфига> или /usr/bin/zmp)
    static QString installedBinaryPath();
    static QString md5OfFile(const QString &path);

private slots:
    void onExecute();

private:
    bool runPrivileged(const QStringList &args);
    bool ensureSudoPassword();
    void wipeSudoPassword();

    QLabel *m_hashInfoLabel = nullptr;
    QLineEdit *m_nameEdit;
    QCheckBox *m_mp3Check;
    QCheckBox *m_wavCheck;
    QCheckBox *m_alacCheck;
    QCheckBox *m_flacCheck;
    QString m_sudoPassword;   // запрашивается один раз, стирается после установки
};

#endif // ZMPINSTALLER_H
