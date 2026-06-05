#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Медиаплеер");
    resize(1200, 800);
    setMinimumSize(800, 600);
    
    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    setCentralWidget(central);

    m_menu = new QListWidget(this);
    m_menu->setFixedWidth(200);
    m_menu->setStyleSheet("QListWidget::item { padding: 8px; }");

    m_menu->addItem("Устройства");
    m_menu->addItem("Плеер");
    m_menu->addItem("Файлы");
    m_menu->addItem("Эквалайзер");
    m_menu->addItem("Параметры");
    m_menu->setCurrentRow(0);

    mainLayout->addWidget(m_menu);

    m_stack = new QStackedWidget(this);
    mainLayout->addWidget(m_stack, 1);

    m_audioManager = new AudioManager(this);
    m_devicesWidget = new DevicesWidget(this);
    m_playerWidget = new PlayerWidget(m_audioManager, this);
    m_filesWidget = new FilesWidget(this);
    m_equalizerWidget = new EqualizerWidget(m_audioManager, this);
    m_settingsWidget = new SettingsWidget(this);

    m_stack->addWidget(m_devicesWidget);
    m_stack->addWidget(m_playerWidget);
    m_stack->addWidget(m_filesWidget);
    m_stack->addWidget(m_equalizerWidget);
    m_stack->addWidget(m_settingsWidget);


    connect(m_menu, &QListWidget::currentRowChanged, this, &MainWindow::onMenuChanged);
    connect(m_devicesWidget, &DevicesWidget::deviceChanged, this, &MainWindow::onDeviceChanged);
    connect(m_filesWidget, &FilesWidget::fileSelected, this, &MainWindow::onFileSelected);
    connect(m_settingsWidget, &SettingsWidget::exitRequested, this, &MainWindow::onExit);

    if (!m_devicesWidget->selectedDevice().isNull())
        m_audioManager->setActiveOutputDevice(m_devicesWidget->selectedDevice());

    qDebug() << "MainWindow initialized, menu visible";
}

MainWindow::~MainWindow() {}

void MainWindow::onMenuChanged(int row)
{
    m_stack->setCurrentIndex(row);
}

void MainWindow::onDeviceChanged(const QAudioDevice &device)
{
    m_audioManager->setActiveOutputDevice(device);
}

void MainWindow::onFileSelected(const QString &path)
{
    QStringList playlist;
    playlist.append(path);
    m_playerWidget->setPlaylist(playlist);
    m_playerWidget->onPlay();     // вотч дэмо шедевро вскоде точнее пк с 4гб озу выдал прикол с вылетом
    m_menu->setCurrentRow(1);    
}

void MainWindow::onExit()
{
    QApplication::quit();
}
