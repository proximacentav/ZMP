#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_menuIndicator(nullptr), m_menuIndicatorY(0), m_menuIndicatorTargetY(0)
{
    setWindowTitle("Медиаплеер");
    resize(1200, 800);

    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    setCentralWidget(central);

    QWidget *menuContainer = new QWidget(this);
    menuContainer->setFixedWidth(200);
    menuContainer->setStyleSheet("background-color: #2b2b2b;");
    QVBoxLayout *menuContLayout = new QVBoxLayout(menuContainer);
    menuContLayout->setContentsMargins(5, 10, 5, 10);

    m_menu = new QListWidget;
    m_menu->addItems({"Устройства", "Плеер", "Плейлисты", "Файлы", "Эквалайзер", "Параметры"});
    m_menu->setCurrentRow(0);
    
    m_menu->setSpacing(10);
    m_menu->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { background: transparent; color: white; padding: 12px; border-radius: 8px; }"
        "QListWidget::item:selected { background: transparent; color: white; }" // Прозрачный при выделении
    );
    menuContLayout->addWidget(m_menu);

    m_menuIndicator = new QWidget(menuContainer);
    m_menuIndicator->setFixedHeight(40);
    m_menuIndicator->setGeometry(5, 0, 190, 40);
    m_menuIndicator->setStyleSheet(
        "background-color: rgba(183, 183, 183, 200);"
        "border-radius: 8px;"
    );
    m_menuIndicator->raise();

    m_menuAnimTimer = new QTimer(this);
    connect(m_menuAnimTimer, &QTimer::timeout, this, &MainWindow::animateMenu);
    
    if (m_menu->item(0)) {
        m_menuIndicatorY = m_menu->visualItemRect(m_menu->item(0)).y();
        m_menuIndicatorTargetY = m_menuIndicatorY;
        m_menuIndicator->move(5, m_menuIndicatorY);
    }

    mainLayout->addWidget(menuContainer);

    m_stack = new QStackedWidget;
    mainLayout->addWidget(m_stack, 1);

    m_audioManager = new AudioManager(this);
    m_devicesWidget = new DevicesWidget(this);
    m_playerWidget = new PlayerWidget(m_audioManager, this);
    m_playlistsWidget = new PlaylistsWidget(this);
    m_filesWidget = new FilesWidget(this);
    m_equalizerWidget = new EqualizerWidget(m_audioManager, this);
    m_settingsWidget = new SettingsWidget(this);

    m_stack->addWidget(m_devicesWidget);
    m_stack->addWidget(m_playerWidget);
    m_stack->addWidget(m_playlistsWidget);
    m_stack->addWidget(m_filesWidget);
    m_stack->addWidget(m_equalizerWidget);
    m_stack->addWidget(m_settingsWidget);

    connect(m_menu, &QListWidget::currentRowChanged, this, [this](int row) {
        QListWidgetItem *item = m_menu->item(row);
        if (item) {
            m_menuIndicatorTargetY = m_menu->visualItemRect(item).y();
            if (!m_menuAnimTimer->isActive()) m_menuAnimTimer->start(16);
        }
        m_stack->setCurrentIndex(row);
    });

    connect(m_devicesWidget, &DevicesWidget::deviceChanged, this, &MainWindow::onDeviceChanged);
    connect(m_playlistsWidget, &PlaylistsWidget::playlistSelected, this, [this](const QStringList &tracks) {
        m_playerWidget->setPlaylist(tracks);
        m_playerWidget->onPlay();
        m_menu->setCurrentRow(1);
    });
    connect(m_filesWidget, &FilesWidget::fileSelected, this, [this](const QString &path) {
        m_playerWidget->setPlaylist({path});
        m_playerWidget->onPlay();
        m_menu->setCurrentRow(1);
    });

    connect(m_settingsWidget, &SettingsWidget::exitRequested, this, &MainWindow::onExit);
    connect(m_settingsWidget, &SettingsWidget::metadataHeightChanged, m_playerWidget, &PlayerWidget::setMetadataHeight);
    connect(m_settingsWidget, &SettingsWidget::accentColorChanged, m_playerWidget, &PlayerWidget::setAccentColor);
    connect(m_settingsWidget, &SettingsWidget::spectrumGainChanged, m_audioManager, &AudioManager::setSpectrumGain);
    connect(m_settingsWidget, &SettingsWidget::spectrumFpsChanged, m_audioManager, &AudioManager::setSpectrumFps);
    connect(m_audioManager, &AudioManager::spectrumDataChanged, m_playerWidget, &PlayerWidget::updateSpectrum);

    if (!m_devicesWidget->selectedDevice().isNull())
        m_audioManager->setActiveOutputDevice(m_devicesWidget->selectedDevice());
}

void MainWindow::animateMenu() {
    m_menuIndicatorY += (m_menuIndicatorTargetY - m_menuIndicatorY) * 0.2;
    
    if (qAbs(m_menuIndicatorTargetY - m_menuIndicatorY) < 1.0) {
        m_menuIndicatorY = m_menuIndicatorTargetY;
        m_menuAnimTimer->stop();
    }
    m_menuIndicator->move(5, m_menuIndicatorY);
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