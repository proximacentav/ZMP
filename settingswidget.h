#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>

class QSlider;
class QComboBox;
class QLineEdit;
class QPushButton;

class SettingsWidget : public QWidget // новая версия!!
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    int maxBitrate() const;

signals:
    void accentColorChanged(const QColor &color);
    void metadataHeightChanged(int height);
    void exitRequested();
    void spectrumGainChanged(float gain);

private slots:
    void onSliderChanged(int v);
    void onLineEditChanged();
    void toggleTheme();
    void onColorChanged(int index);
    void onHeightSliderChanged(int v);
    void showAboutDialog();
    void onSpectrumGainChanged(int value);

private:
    QSlider *m_bitrateSlider;
    QLineEdit *m_bitrateEdit;
    QSlider *m_heightSlider;
    QPushButton *m_aboutButton;
    QPushButton *m_themeButton;
    QComboBox *m_colorCombo;
    QPushButton *m_exitButton;
    QSlider *m_spectrumGainSlider;
    
    bool m_darkTheme;
    QColor m_accentColor;
    QMap<QString, QColor> m_colorMap;
    void applyTheme(bool dark);
    void applyAccentColor();
};

#endif // SETTINGSWIDGET_H