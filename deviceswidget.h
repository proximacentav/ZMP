#ifndef DEVICESWIDGET_H
#define DEVICESWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QAudioDevice>
#include <QPushButton>
#include "translator.h"

class QLineEdit;
class QStackedWidget;

class DevicesWidget : public QWidget
{
    Q_OBJECT
public:
    enum OutputMode {
        Speakers = 0,      // вывод в динамики (обычный)
        MicIntercept = 1,  // вывод в микрофон (перехват)
        VirtualMic = 2     // виртуальный микрофон
    };

    explicit DevicesWidget(QWidget *parent = nullptr);
    QAudioDevice selectedDevice() const;

signals:
    void deviceChanged(const QAudioDevice &device);

private slots:
    void onCurrentIndexChanged(int index);
    void onModeChanged(int index);
    void onInterceptMicActivated(int index);
    void onCreateVirtualMic();
    void openDeviceManager();

private:
    void refreshMicList();

    QComboBox *m_modeCombo;
    QPushButton *m_managerBtn;
    QStackedWidget *m_stack;

    // Страница 0 — обычный вывод в динамики
    QWidget *m_speakersPage;
    QComboBox *m_combo;
    QList<QAudioDevice> m_devices;

    // Страница 1 — перехват микрофона
    QWidget *m_interceptPage;
    QComboBox *m_micCombo;

    // Страница 2 — виртуальный микрофон
    QWidget *m_virtualPage;
    QLineEdit *m_virtualNameEdit;
    QPushButton *m_createVirtualBtn;

    RetransList m_retrans;
    void retranslateUi();
};

#endif
