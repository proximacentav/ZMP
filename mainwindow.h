#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include "audiomanager.h"
#include "deviceswidget.h"
#include "playerwidget.h"
#include "fileswidget.h"
#include "settingswidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onMenuChanged(int row);
    void onDeviceChanged(const QAudioDevice &device);   // ← исправлено
    void onFileSelected(const QString &path);
    void onExit();

private:
    QListWidget *m_menu;
    QStackedWidget *m_stack;

    AudioManager *m_audioManager;
    DevicesWidget *m_devicesWidget;
    PlayerWidget *m_playerWidget;
    FilesWidget *m_filesWidget;
    SettingsWidget *m_settingsWidget;
};

#endif // MAINWINDOW_H