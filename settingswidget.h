#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QMap>
#include <QColor>

class QSlider;
class QComboBox;
class QLineEdit;
class QPushButton;

class SettingsWidget : public QWidget
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
    void spectrumFpsChanged(int fps);

private slots:
    void onSliderChanged(int v);
    void onLineEditChanged();
    void toggleTheme();
    void onColorChanged(int index);
    void onHeightSliderChanged(int v);
    void onSpectrumGainChanged(int value);
    void showAboutDialog();

private:
    QSlider *m_bitrateSlider;
    QLineEdit *m_bitrateEdit;
    QSlider *m_heightSlider;
    QPushButton *m_aboutButton;
    QPushButton *m_themeButton;
    QComboBox *m_colorCombo;
    QPushButton *m_exitButton;
    QSlider *m_spectrumGainSlider;
    QComboBox *m_spectrumFpsCombo;
    
    bool m_darkTheme;
    QColor m_accentColor;
    QMap<QString, QColor> m_colorMap;

    void applyTheme(bool dark);
    void applyAccentColor();
};

#endif // SETTINGSWIDGET_H