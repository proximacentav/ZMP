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

    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    setCentralWidget(central);

    m_menu = new QListWidget;
    m_menu->setFixedWidth(200);
    m_menu->addItems({"Устройства", "Плеер", "Плейлисты", "Эквалайзер", "Параметры"});
    m_menu->setCurrentRow(0);
    mainLayout->addWidget(m_menu);

    m_stack = new QStackedWidget;
    mainLayout->addWidget(m_stack, 1);

    m_audioManager = new AudioManager(this);
    m_devicesWidget = new DevicesWidget(this);
    m_playerWidget = new PlayerWidget(m_audioManager, this);
    m_playlistsWidget = new PlaylistsWidget(this);
    m_equalizerWidget = new EqualizerWidget(m_audioManager, this);
    m_settingsWidget = new SettingsWidget(this);

    m_stack->addWidget(m_devicesWidget);
    m_stack->addWidget(m_playerWidget);
    m_stack->addWidget(m_playlistsWidget);
    m_stack->addWidget(m_equalizerWidget);
    m_stack->addWidget(m_settingsWidget);

    connect(m_menu, &QListWidget::currentRowChanged, this, &MainWindow::onMenuChanged);
    connect(m_devicesWidget, &DevicesWidget::deviceChanged, this, &MainWindow::onDeviceChanged);
    connect(m_playlistsWidget, &PlaylistsWidget::playlistSelected, this, [this](const QStringList &tracks) {
        m_playerWidget->setPlaylist(tracks);
        m_playerWidget->onPlay();
        m_menu->setCurrentRow(1);
    });
    connect(m_settingsWidget, &SettingsWidget::exitRequested, this, &MainWindow::onExit);
    connect(m_settingsWidget, &SettingsWidget::metadataHeightChanged, m_playerWidget, &PlayerWidget::setMetadataHeight);
    connect(m_settingsWidget, &SettingsWidget::accentColorChanged, m_playerWidget, &PlayerWidget::setAccentColor);
    connect(m_settingsWidget, &SettingsWidget::spectrumGainChanged, m_audioManager, &AudioManager::setSpectrumGain);

    if (!m_devicesWidget->selectedDevice().isNull())
        m_audioManager->setActiveOutputDevice(m_devicesWidget->selectedDevice());
}

MainWindow::~MainWindow() {}

void MainWindow::onMenuChanged(int row) { m_stack->setCurrentIndex(row); }
void MainWindow::onDeviceChanged(const QAudioDevice &device) { m_audioManager->setActiveOutputDevice(device); }
void MainWindow::onFileSelected(const QString &path) {
    m_playerWidget->setPlaylist({path});
    m_playerWidget->onPlay();
    m_menu->setCurrentRow(1);
}
void MainWindow::onExit() { QApplication::quit(); }