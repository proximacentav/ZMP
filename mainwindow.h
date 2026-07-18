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
#include "miniplayerbar.h"

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

private slots:
    void onMenuChanged(int row);
    void onDeviceChanged(const QAudioDevice &device);
    void onFileSelected(const QString &path);
    void onExit();
    void animateMenu();
    void onHiddenButtonClicked();
    void onSelectionTimeout();
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
    QWidget *m_menuIndicator;
    QTimer *m_menuAnimTimer;
    qreal m_menuIndicatorY;
    qreal m_menuIndicatorTargetY;
    enum State { Idle, Deciding, SelectionMode, ActionMode };
    State m_state;
    int m_clickCount;
    QTimer *m_selectionTimer;
    QPushButton *m_hiddenButton;
    QMap<SettingsWidget::KeyAction, SettingsWidget::KeyBinding> m_keyBindings;
    
    void performAction(int count);
    void enterSelectionMode();
    void resetButton();
    void onFeaturedUpdated();
    void handleKeyPress(Qt::Key key, Qt::KeyboardModifiers modifiers);
    void loadKeyBindingsFromSettings();
    void showEqualizerPresetDialog();
};

#endif