#ifndef DEPSMANAGER_H
#define DEPSMANAGER_H

#include <QDialog>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QPointer>

class QComboBox;
class QLineEdit;
class QCheckBox;
class QLabel;
class QListWidget;
class QPushButton;
class QProcess;
class QTextEdit;
class QTimer;
class QVBoxLayout;

// ---------------------------------------------------------------------------
//  DepsProxyDialog — настройка прокси для установки зависимостей.
//  Экран идентичен настройке прокси Jamendo, но без API ключа.
//  Конфиг хранится в ~/zmp_playlists/config.json в секции "deps".
// ---------------------------------------------------------------------------
class DepsProxyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DepsProxyDialog(QWidget *parent = nullptr);

    QString proxyType() const;
    QString proxyHost() const;
    bool proxyAuth() const;
    QString proxyUsername() const;
    QString proxyPassword() const;
    bool proxySslAllow() const;
    bool proxyDnsThrough() const;

    static QJsonObject loadDepsConfig();
    static void saveDepsConfig(const QJsonObject &config);

private slots:
    void onProxyTypeChanged(int index);
    void onAuthToggled(bool checked);
    void onAccept();

private:
    void loadFromConfig();

    QComboBox *m_proxyTypeCombo;
    QWidget *m_proxySettingsWidget;
    QLineEdit *m_proxyHostEdit;
    QCheckBox *m_authCheck;
    QWidget *m_authWidget;
    QLineEdit *m_proxyUserEdit;
    QLineEdit *m_proxyPassEdit;
    QCheckBox *m_sslAllowCheck;
    QCheckBox *m_dnsThroughCheck;
};

// ---------------------------------------------------------------------------
//  DependencyManager — проверка зависимостей и установка через пакетные
//  менеджеры (apt / dnf|yum / pacman). Живёт как синглтон, поэтому установка
//  продолжается после закрытия диалога.
// ---------------------------------------------------------------------------
class DependencyManager : public QObject
{
    Q_OBJECT
public:
    enum Status { Found = 0, Missing = 1, Unknown = 2 };

    struct DepResult {
        QString title;          // отображаемое имя
        QString description;    // что это
        Status status = Unknown;
        QStringList packages;   // имена пакетов для найденного менеджера (пусто если недоступно)
    };

    enum PkgManager { None = 0, Apt, Dnf, Yum, Pacman };

    static DependencyManager *instance();

    // Определение ОС / пакетного менеджера
    PkgManager packageManager() const { return m_pkgManager; }
    QString distroName() const { return m_distroName; }
    QString pkgManagerCommand() const;

    // Синхронная проверка всех зависимостей (быстрая)
    QVector<DepResult> runChecks();

    // Установка выбранных зависимостей (асинхронно)
    bool installRunning() const { return m_installRunning; }
    void startInstall(const QVector<DepResult> &selected);

    // Информация о прокси из секции "deps" конфига (для отображения)
    QString proxyDisplayIp() const;
    bool hasProxy() const;

    // Запущен ли процесс из терминала (есть tty на stdout/stderr)
    static bool launchedFromTerminal();

    // Проверить зависимости и сообщить о недостающих: если запущено из
    // терминала — в логи, иначе открыть окно терминала с текстом.
    // Если Qt доступен — дополнительно диалоговое окно с кнопкой ОК.
    void reportMissingDependencies();

signals:
    void checksFinished(const QVector<DependencyManager::DepResult> &results);
    void installStarted(int totalSteps);
    void installProgress(const QString &currentPackage, int percent, qint64 speedBps);
    void installFinished(bool success, const QStringList &succeeded, const QStringList &failed);

private:
    explicit DependencyManager(QObject *parent = nullptr);

    void detectOs();
    Status checkLibrary(const QString &soname) const;
    Status checkTool(const QString &tool) const;
    QString elevationPrefix() const;
    void runNextStep();
    void applyProxyEnv(QStringList &env);

    PkgManager m_pkgManager = None;
    QString m_distroName;
    QString m_ldconfigCache;
    bool m_ldconfigOk = false;

    QVector<DepResult> m_queue;
    int m_step = 0;
    int m_totalSteps = 0;
    QStringList m_succeeded;
    QStringList m_failed;
    bool m_installRunning = false;
    qint64 m_outputBytes = 0;
    qint64 m_lastSpeedBytes = 0;
    QString m_currentPackage;
    int m_currentPercent = 0;
    QTimer *m_speedTimer = nullptr;
    QProcess *m_proc = nullptr;
};

Q_DECLARE_METATYPE(DependencyManager::DepResult)

// Обработчик фатальных сигналов (SIGSEGV и т.п.): в форкнутом копии процесса
// проверяет зависимости и сообщает о недостающих (stderr / окно терминала),
// затем восстанавливает стандартное поведение.
void zmpInstallCrashHandler();

// Открыть окно терминала с сообщением (используется и при краше, без Qt API).
void zmpOpenTerminalWithMessage(const QString &message);

// ---------------------------------------------------------------------------
//  DependencyCheckDialog — диалог проверки зависимостей.
// ---------------------------------------------------------------------------
class DependencyCheckDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DependencyCheckDialog(QWidget *parent = nullptr);

    // Активный диалог (если открыт) — чтобы MainWindow не дублировал итоги
    static QPointer<QDialog> s_active;

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onInstallClicked();
    void onInstallStarted(int totalSteps);
    void onInstallProgress(const QString &currentPackage, int percent, qint64 speedBps);
    void onInstallFinished(bool success, const QStringList &succeeded, const QStringList &failed);

private:
    void repopulate(const QVector<DependencyManager::DepResult> &results);
    static QString speedColor(qint64 speedBps);

    QListWidget *m_list;
    QLabel *m_statusLabel;
    QPushButton *m_installBtn;
    QPushButton *m_closeBtn;
    QVector<DependencyManager::DepResult> m_results;
};

#endif // DEPSMANAGER_H
