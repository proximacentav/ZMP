#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QLineEdit>    // версия 0.3.0 ! ! ! 
#include <QPushButton>
#include <QComboBox>
#include <QMap>

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    int maxBitrate() const;

signals:
    void exitRequested();
    void metadataHeightChanged(int height);

private slots:
    void onSliderChanged(int value);
    void onLineEditChanged();
    void toggleTheme();
    void onColorChanged(int index);
    void onHeightSliderChanged(int value);   
    void showAboutDialog();

private:
    void applyTheme(bool dark);
    void applyAccentColor();

    QSlider *m_bitrateSlider;
    QLineEdit *m_bitrateEdit;
    QPushButton *m_exitButton;
    QPushButton *m_themeButton;
    QComboBox *m_colorCombo;
    QSlider *m_heightSlider;        
    QPushButton *m_aboutButton;     
    bool m_darkTheme;
    QColor m_accentColor;
    QMap<QString, QColor> m_colorMap;
};

#endif // SETTINGSWIDGET_H