#ifndef EQUALIZERWIDGET_H
#define EQUALIZERWIDGET_H


#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#error "install qt6"
#endif

#include <QWidget>
#include <QVector>
#include <QMap>
#include "audiomanager.h"   // этость нужность для эквалайзерности

class QSlider;
class QSpinBox;
class QScrollArea;
class QGridLayout;

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

private slots:
    void onSliderMoved(int bandIndex, int value);
    void onSpinBoxChanged(int bandIndex, int value);
    void onPreampSliderMoved(int value);
    void onPreampSpinBoxChanged(int value);

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
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QGridLayout *m_layout;

    void createBands();
};

#endif // EQUALIZERWIDGET_H