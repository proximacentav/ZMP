#ifndef DEVICESWIDGET_H
#define DEVICESWIDGET_H


#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#error "install qt6"
#endif

#include <QWidget>
#include <QComboBox>
#include <QAudioDevice>

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
};

#endif // DEVICESWIDGET_H