#include "deviceswidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QMediaDevices>

DevicesWidget::DevicesWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Устройство вывода звука:", this));
    m_combo = new QComboBox(this);
    layout->addWidget(m_combo);

    m_devices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &dev : m_devices) {
        m_combo->addItem(dev.description(), QVariant::fromValue(dev));
    }

    if (m_combo->count() > 0)
        m_combo->setCurrentIndex(0);

    connect(m_combo, &QComboBox::currentIndexChanged, this, &DevicesWidget::onCurrentIndexChanged);
}

QAudioDevice DevicesWidget::selectedDevice() const
{
    int idx = m_combo->currentIndex();
    if (idx >= 0 && idx < m_devices.size())
        return m_devices[idx];
    return QAudioDevice();
}

void DevicesWidget::onCurrentIndexChanged(int index)
{
    if (index >= 0 && index < m_devices.size()) {
        emit deviceChanged(m_devices[index]);
    }
}