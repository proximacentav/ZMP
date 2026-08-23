#ifndef DEVICEMANAGERDIALOG_H
#define DEVICEMANAGERDIALOG_H

#include <QDialog>
#include <QList>
#include <QSet>
#include <QString>

class QComboBox;
class QListWidget;
class QLabel;
class QPushButton;

struct PaDeviceEntry {
    int id = 0;          // PipeWire/WirePlumber object id (совпадает с pactl)
    QString name;        // описание из wpctl status
    bool isInput = false;
    bool isCard = false; // карта из секции Devices (видна даже выключенной)
};

// "Менеджер устройств": список устройств ввода/вывода и операции
// уничтожить / раскулачить (отключить, изменить работу, стандартным, освободить).
class DeviceManagerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DeviceManagerDialog(QWidget *parent = nullptr);

private slots:
    void refreshList();
    void onDestroyClicked();
    void onRaskulachitClicked();

private:
    QList<PaDeviceEntry> listDevices() const;

    QComboBox *m_categoryCombo;
    QListWidget *m_list;
    QLabel *m_statusLabel;
    QSet<int> m_swappedNodes;   // узлы с изменённой ролью (кнопка «Изменить работу»)
};

#endif // DEVICEMANAGERDIALOG_H
