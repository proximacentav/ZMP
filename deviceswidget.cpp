#include "deviceswidget.h"
#include "audiomodecontroller.h"
#include "devicemanagerdialog.h"
#include "translator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMediaDevices>
#include <QPushButton>
#include <QStackedWidget>

#ifdef Q_OS_WIN
// Windows: только выбор устройства вывода (QMediaDevices), без PipeWire-функций
DevicesWidget::DevicesWidget(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(ztrLabel(m_retrans, "Устройство вывода звука:", this));
    m_combo = new QComboBox(this);
    layout->addWidget(m_combo);
    m_devices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &dev : m_devices)
        m_combo->addItem(dev.description(), QVariant::fromValue(dev));
    if (m_combo->count() > 0) m_combo->setCurrentIndex(0);
    connect(m_combo, &QComboBox::currentIndexChanged,
            this, [this](int idx){ if (idx >= 0 && idx < m_devices.size()) emit deviceChanged(m_devices[idx]); });
    connect(&Translator::instance(), &Translator::languageChanged, this, &DevicesWidget::retranslateUi);
}
#else
DevicesWidget::DevicesWidget(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Верхний ряд: режим вывода + менеджер устройств (Linux/PipeWire only)
    QHBoxLayout *topRow = new QHBoxLayout;
    m_topRow = topRow;

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(ztr("вывод в динамики"));
    m_modeCombo->addItem(ztr("вывод в микрофон (перехват)"));
    m_modeCombo->addItem(ztr("виртуальный микрофон"));
    ztrRegister(m_retrans, [this] {
        m_modeCombo->setItemText(0, ztr("вывод в динамики"));
        m_modeCombo->setItemText(1, ztr("вывод в микрофон (перехват)"));
        m_modeCombo->setItemText(2, ztr("виртуальный микрофон"));
    });
    topRow->addWidget(m_modeCombo, 1);

    m_managerBtn = ztrButton(m_retrans, "Менеджер устройств");
    topRow->addWidget(m_managerBtn);
    layout->addLayout(topRow);

    // Страница 0: обычный выбор устройства вывода (как раньше)
#ifndef Q_OS_WIN
    m_speakersPage = new QWidget(this);
    QVBoxLayout *spLayout = new QVBoxLayout(m_speakersPage);
    spLayout->addWidget(ztrLabel(m_retrans, "Устройство вывода звука:"));
    m_combo = new QComboBox(m_speakersPage);
    spLayout->addWidget(m_combo);
    m_devices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &dev : m_devices)
        m_combo->addItem(dev.description(), QVariant::fromValue(dev));
    if (m_combo->count() > 0) m_combo->setCurrentIndex(0);
    connect(m_combo, &QComboBox::currentIndexChanged,
            this, &DevicesWidget::onCurrentIndexChanged);

    // Страница 1: перехват — список всех микрофонов системы
    m_interceptPage = new QWidget(this);
    QVBoxLayout *inLayout = new QVBoxLayout(m_interceptPage);
    inLayout->addWidget(ztrLabel(m_retrans, "Микрофон для перехвата:"));
    m_micCombo = new QComboBox(m_interceptPage);
    inLayout->addWidget(m_micCombo);
    refreshMicList();

    // Страница 2: виртуальный микрофон — имя + кнопка создать
    m_virtualPage = new QWidget(this);
    QVBoxLayout *vmLayout = new QVBoxLayout(m_virtualPage);
    vmLayout->addWidget(ztrLabel(m_retrans, "Название виртуального микрофона:"));
    m_virtualNameEdit = new QLineEdit(m_virtualPage);
    m_virtualNameEdit->setPlaceholderText(ztr("Например: ZMP Music Mic"));
    vmLayout->addWidget(m_virtualNameEdit);
    m_createVirtualBtn = ztrButton(m_retrans, "Создать", m_virtualPage);
    vmLayout->addWidget(m_createVirtualBtn);
    vmLayout->addStretch();

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_speakersPage);
    m_stack->addWidget(m_interceptPage);
    m_stack->addWidget(m_virtualPage);
    layout->addWidget(m_stack);
#endif
    layout->addStretch();

    connect(m_modeCombo, &QComboBox::currentIndexChanged,
            this, &DevicesWidget::onModeChanged);
    connect(m_managerBtn, &QPushButton::clicked,
            this, &DevicesWidget::openDeviceManager);
    connect(m_micCombo, QOverload<int>::of(&QComboBox::activated),
            this, &DevicesWidget::onInterceptMicActivated);
    connect(m_createVirtualBtn, &QPushButton::clicked,
            this, &DevicesWidget::onCreateVirtualMic);
    connect(AudioModeController::instance(), &AudioModeController::errorOccurred,
            this, [this](const QString &msg) {
                setWindowTitle(msg);
            });
    connect(&Translator::instance(), &Translator::languageChanged,
            this, &DevicesWidget::retranslateUi);
}

void DevicesWidget::setPortraitMode(bool portrait)
{
    if (!m_topRow) return;
    // Портрет: менеджер устройств под выпадающим списком режима вывода
    m_topRow->setDirection(portrait ? QBoxLayout::TopToBottom
                                    : QBoxLayout::LeftToRight);
}

void DevicesWidget::refreshMicList(){
    m_micCombo->blockSignals(true);
    m_micCombo->clear();
    const QList<MicInfo> mics = AudioModeController::instance()->listMicrophones();
    for (const MicInfo &mic : mics) {
        // Показываем понятное описание; внутреннее имя хранится в данных
        m_micCombo->addItem(mic.description, mic.paName);
    }
    m_micCombo->blockSignals(false);
}

void DevicesWidget::retranslateUi() {
    runRetrans(m_retrans);
}

QAudioDevice DevicesWidget::selectedDevice() const {
    int idx = m_combo->currentIndex();
    return (idx >= 0 && idx < m_devices.size()) ? m_devices[idx] : QAudioDevice();
}

void DevicesWidget::onCurrentIndexChanged(int index) {
    if (index >= 0 && index < m_devices.size())
        emit deviceChanged(m_devices[index]);
}

void DevicesWidget::onModeChanged(int index)
{
    if (index != MicIntercept && index != VirtualMic) {
        // Выход из спец-режима: убрать виртуальные микрофоны/перехват и вернуть звук
        AudioModeController::instance()->reset();
    }
    if (index == MicIntercept) {
        // Обновляем список при каждом входе в режим перехвата —
        // микрофон мог быть подключён после запуска ZMP
        refreshMicList();
    }
    m_stack->setCurrentIndex(index == Speakers ? 0 : index);
}

void DevicesWidget::onInterceptMicActivated(int index)
{
    if (index < 0 || index >= m_micCombo->count())
        return;
    const QString paName = m_micCombo->itemData(index).toString();
    AudioModeController::instance()->startIntercept(paName);
}

void DevicesWidget::onCreateVirtualMic()
{
    AudioModeController::instance()->createVirtualMic(m_virtualNameEdit->text());
}

void DevicesWidget::openDeviceManager()
{
    DeviceManagerDialog dlg(this);
    dlg.exec();
}

#endif // !Q_OS_WIN
