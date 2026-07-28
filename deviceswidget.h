#ifndef DEVICESWIDGET_H
#define DEVICESWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QAudioDevice>
#include "translator.h"

class DevicesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DevicesWidget(QWidget *parent = nullptr);
    QAudioDevice selectedDevice() const;

signals:
    void deviceChanged(const QAudioDevice &device);

private slots:
    void onCurrentIndexChanged(int index);

private:
    QComboBox *m_combo;
    QList<QAudioDevice> m_devices;
    RetransList m_retrans;
    void retranslateUi();
};

#endif