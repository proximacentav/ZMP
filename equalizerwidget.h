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
#include "audiomanager.h"

class EqualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EqualizerWidget(AudioManager *audioManager, QWidget *parent = nullptr);
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

private:
    AudioManager *m_audioManager;
    struct Band {
        double freq;
        QSlider *slider;
        QSpinBox *spinBox;  
    };
    QVector<Band> m_bands;
    QSlider *m_preampSlider;
    QSpinBox *m_preampSpinBox;
    QSlider *m_speedSlider;
    QDoubleSpinBox *m_speedSpinBox;
    QSlider *m_pitchSlider;
    QDoubleSpinBox *m_pitchSpinBox;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QGridLayout *m_layout;

    void createBands();
};

#endif // EQUALIZERWIDGET_H