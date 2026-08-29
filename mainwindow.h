#ifndef MAINWINDOW_H
class LiquidIndicator;
#define MAINWINDOW_H

// и помните все отсортировано по алфавиту
#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QPushButton>
#include <QAudioDevice>
#include <QKeyEvent>
#include <QDialog>
#include <QListWidget>
#include "audiomanager.h"
#include "deviceswidget.h"
#include "playerwidget.h"
#include "playlistswidget.h"
#include "equalizerwidget.h"
#include "visualizationwidget.h"
#include "settingswidget.h"
#include "fileswidget.h"
#include "jamendowidget.h"
#include "miniplayerbar.h"
#include "translator.h"
#include <QNetworkReply>
#include <QLabel>
#include <QVBoxLayout>
#include <QSplitter>
#include "depsmanager.h"

class EqualizerPresetDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EqualizerPresetDialog(QWidget *parent = nullptr);
    QString getSelectedPreset() const { return m_selectedPreset; }

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QListWidget *m_presetList;
    QString m_selectedPreset;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    bool eventFilter(QObject *watched, QEvent *event) override;
    void addLog(const QString &message);
    static void setSplash(class QSplashScreen *splash);   // окно "ZMP is starting..."
    void openFilesInQueue(const QStringList &files);
    void playExternalPlaylist(const QString &cluster, const QString &name);

private slots:
    void onMenuChanged(int row);
    void onDeviceChanged(const QAudioDevice &device);
    void onFileSelected(const QString &path);
    void onExit();
    void onUserButtonClicked();
    void showRootPasswordDialog();
    void showLogDialog();
    void autoSaveLogs();
    void onKeyBindingChanged(SettingsWidget::KeyAction action, const SettingsWidget::KeyBinding &binding);
    void onKeyBindingsSaved();

private:
    QListWidget *m_menu;

    // Положение бокового меню и его сворачивание
    enum class MenuSide { Left = 0, Top = 1, Right = 2, Bottom = 3 };
    MenuSide m_menuSide = MenuSide::Left;
    bool m_menuCollapsed = false;
    QWidget *m_menuContainer = nullptr;
    QBoxLayout *m_menuContLay = nullptr;
    QWidget *m_rightContainer = nullptr;
    QBoxLayout *m_mainLay = nullptr;
    QPushButton *m_menuToggleBtn = nullptr;
    void applyMenuLayout();
    void applyMenuGeometry();
    void positionMenuToggle();
    void updateLiquidTarget();
    bool isHorizontalMenu() const;
    bool isPortraitLayout() const;
    void applyResponsiveLayout();
    QTimer *m_portraitDebounce = nullptr;
protected:
    void resizeEvent(QResizeEvent *event) override;

private:

    QSplitter *m_splitter = nullptr;   // делитель половин (всегда активен)
    QStackedWidget *m_leftStack, *m_rightStack;
    QVector<QWidget*> m_wrapLeft, m_wrapRight;
    bool m_splitMode = false;
    int m_leftRow = 0;
    int m_splitLeft = -1, m_splitRight = -1;
    QVector<QWidget*> m_pages;         // страницы вкладок в порядке меню
    MiniPlayerBar *m_miniPlayerBar;
    AudioManager *m_audioManager = nullptr;
    DevicesWidget *m_devicesWidget;
    PlayerWidget *m_playerWidget;
    PlaylistsWidget *m_playlistsWidget;
    EqualizerWidget *m_equalizerWidget;
    VisualizationWidget *m_visualizationWidget;
    SettingsWidget *m_settingsWidget;
    FilesWidget *m_filesWidget;
    JamendoWidget *m_jamendoWidget;
    LiquidIndicator *m_liquid = nullptr;   // liquid glass линза вкладки
    QPushButton *m_userButton;
    QString m_rootPassword;
    bool m_isRootMode = false;
    QStringList m_logs;
    QTimer *m_logTimer;
    QMap<SettingsWidget::KeyAction, SettingsWidget::KeyBinding> m_keyBindings;
    void saveLogs(const QString &path = QString());
    
    void onFeaturedUpdated();
    static QString getCurrentUsername();
    void updateUserButtonStyle();
    void handleKeyPress(Qt::Key key, Qt::KeyboardModifiers modifiers);
    void loadKeyBindingsFromSettings();
    void showEqualizerPresetDialog();
    QNetworkReply *m_currentDownloadReply = nullptr;

    QLabel *m_depsBanner = nullptr;   // "Установка зависимостей: ..." над вкладками
    QWidget *m_depsWarnBanner = nullptr; // красный баннер "не хватает зависимостей"
    QWidget *m_installBanner = nullptr;  // баннер "установите ZMP как приложение"
    void updateDepsBanner(const QString &pkg, int percent, qint64 speedBps);
    void createDepsWarningBanner(QVBoxLayout *rightLayout);
    void checkDependenciesAtStartup();
    void createInstallBanner(QVBoxLayout *rightLayout);
    void checkInstallAtStartup();
    void handleMenuSelection(int row, bool shift);
    void placeContent(int row, bool rightSide);
    void showSingleTab(int row);
    void enterSplitMode(int leftRow, int rightRow);
    void exitSplitMode(int gotoRow = -1);
    void setRightPane(int row);

    RetransList m_retrans;   // live-retranslation registry (window title + menu)
    void retranslateUi();
};

#endif