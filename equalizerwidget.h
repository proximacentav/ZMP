#ifndef EQUALIZERWIDGET_H
#define EQUALIZERWIDGET_H

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QScrollArea>
#include <QGridLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include "audiomanager.h"

class EqualizerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EqualizerWidget(AudioManager *audioManager, QWidget *parent = nullptr);
    void setPowerMode(bool enabled);
    QMap<double, int> getBandGains() const;
    int getPreampGain() const;

signals:
    void bandGainChanged(double frequencyHz, int gainDb);
    void preampGainChanged(int gainDb);
    void speedChanged(double speed);
    void pitchChanged(double pitch);

private slots:
    void onSliderMoved(int bandIndex, int value);
    void onSpinBoxChanged(int bandIndex, int value);
    void onPreampSliderMoved(int value);
    void onPreampSpinBoxChanged(int value);
    void onSpeedSliderMoved(int value);
    void onSpeedSpinBoxChanged(double value);
    void onPitchSliderMoved(int value);
    void onPitchSpinBoxChanged(double value);
    void onResetClicked();
    void onModeChanged(int index);

private:
    AudioManager *m_audioManager;
    struct Band { double freq; QSlider *slider; QSpinBox *spinBox; };
    QVector<Band> m_bands;
    static const int NUM_BANDS = 16;
    QSlider *m_preampSlider;
    QSpinBox *m_preampSpinBox;
    QSlider *m_speedSlider;
    QDoubleSpinBox *m_speedSpinBox;
    QSlider *m_pitchSlider;
    QDoubleSpinBox *m_pitchSpinBox;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QGridLayout *m_layout;
    QPushButton *m_resetButton;
    QComboBox *m_modeCombo;
    QLabel *m_powerModeLabel;
    bool m_isApplyingPreset = false;
    bool m_powerMode = false;
    void createBands();
    void updateRanges();
    void setBandValue(int index, int value);
    void setPreampValue(int value);
    void setSpeedValue(double value);
    void setPitchValue(double value);
    void applyPreset(const QString &name);
};

#endif
