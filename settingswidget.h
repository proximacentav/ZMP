#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#error "install qt6"
#endif

#include <QWidget>
#include <QSlider>
#include <QLineEdit>
#include <QPushButton>

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    int maxBitrate() const;

signals:
    void exitRequested();

private slots:
    void onSliderChanged(int value);
    void onLineEditChanged();
    void toggleTheme();

private:
    QSlider *m_bitrateSlider;
    QLineEdit *m_bitrateEdit;
    QPushButton *m_exitButton;
    QPushButton *m_themeButton;
    bool m_darkTheme;
};

#endif // SETTINGSWIDGET_H