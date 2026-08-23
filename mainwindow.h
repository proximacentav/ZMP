#ifndef MAINWINDOW_H
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

private slots:
    void onMenuChanged(int row);
    void onDeviceChanged(const QAudioDevice &device);
    void onFileSelected(const QString &path);
    void onExit();
    void animateMenu();
    void onUserButtonClicked();
    void showRootPasswordDialog();
    void showLogDialog();
    void autoSaveLogs();
    void onKeyBindingChanged(SettingsWidget::KeyAction action, const SettingsWidget::KeyBinding &binding);
    void onKeyBindingsSaved();

private:
    QListWidget *m_menu;
    QStackedWidget *m_stack;
    MiniPlayerBar *m_miniPlayerBar;
    AudioManager *m_audioManager;
    DevicesWidget *m_devicesWidget;
    PlayerWidget *m_playerWidget;
    PlaylistsWidget *m_playlistsWidget;
    EqualizerWidget *m_equalizerWidget;
    VisualizationWidget *m_visualizationWidget;
    SettingsWidget *m_settingsWidget;
    FilesWidget *m_filesWidget;
    JamendoWidget *m_jamendoWidget;
    QWidget *m_menuIndicator;
    QTimer *m_menuAnimTimer;
    qreal m_menuIndicatorY;
    qreal m_menuIndicatorTargetY;
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
    void updateDepsBanner(const QString &pkg, int percent, qint64 speedBps);

    RetransList m_retrans;   // live-retranslation registry (window title + menu)
    void retranslateUi();
};

#endif