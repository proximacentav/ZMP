#ifndef MAINWINDOW_H
#define MAINWINDOW_H

 // и помните все отсортировано по алфавиту
#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QAudioDevice>
#include "audiomanager.h"
#include "deviceswidget.h"
#include "playerwidget.h"
#include "playlistswidget.h"
#include "equalizerwidget.h"
#include "settingswidget.h"
#include "fileswidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onMenuChanged(int row);
    void onDeviceChanged(const QAudioDevice &device);
    void onFileSelected(const QString &path);
    void onExit();
    void animateMenu();

private:
    QListWidget *m_menu;
    QStackedWidget *m_stack;
    AudioManager *m_audioManager;
    DevicesWidget *m_devicesWidget;
    PlayerWidget *m_playerWidget;
    PlaylistsWidget *m_playlistsWidget;
    EqualizerWidget *m_equalizerWidget;
    SettingsWidget *m_settingsWidget;
    FilesWidget *m_filesWidget;
    QWidget *m_menuIndicator;
    QTimer *m_menuAnimTimer;
    qreal m_menuIndicatorY;
    qreal m_menuIndicatorTargetY;
};

#endif