#include "equalizerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QDebug>

EqualizerWidget::EqualizerWidget(AudioManager *audioManager, QWidget *parent)
    : QWidget(parent), m_audioManager(audioManager)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *preampLayout = new QHBoxLayout;
    preampLayout->addWidget(new QLabel("Preamp (dB):"));
    m_preampSlider = new QSlider(Qt::Horizontal);
    m_preampSlider->setRange(-1000, 1000);
    m_preampSlider->setValue(0);
    m_preampSlider->setTickInterval(100);
    m_preampSlider->setTickPosition(QSlider::TicksBelow);
    preampLayout->addWidget(m_preampSlider);
    m_preampSpinBox = new QSpinBox;
    m_preampSpinBox->setRange(-1000, 1000);
    m_preampSpinBox->setValue(0);
    m_preampSpinBox->setSuffix(" dB");
    preampLayout->addWidget(m_preampSpinBox);
    mainLayout->addLayout(preampLayout);

    // СПИД
    QHBoxLayout *speedLayout = new QHBoxLayout;
    speedLayout->addWidget(new QLabel("Speed (x):"));
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(1, 5000);
    m_speedSlider->setValue(100);
    m_speedSlider->setTickInterval(100);
    m_speedSlider->setTickPosition(QSlider::TicksBelow);
    speedLayout->addWidget(m_speedSlider);
    m_speedSpinBox = new QDoubleSpinBox;
    m_speedSpinBox->setRange(0.01, 50.0);
    m_speedSpinBox->setValue(1.0);
    m_speedSpinBox->setSingleStep(0.01);
    m_speedSpinBox->setDecimals(2);
    m_speedSpinBox->setSuffix("x");
    speedLayout->addWidget(m_speedSpinBox);
    mainLayout->addLayout(speedLayout);

    QHBoxLayout *pitchLayout = new QHBoxLayout;
    pitchLayout->addWidget(new QLabel("Pitch (semitones):"));
    m_pitchSlider = new QSlider(Qt::Horizontal);
    m_pitchSlider->setRange(-150, 150);
    m_pitchSlider->setValue(0);
    m_pitchSlider->setTickInterval(30);
    m_pitchSlider->setTickPosition(QSlider::TicksBelow);
    pitchLayout->addWidget(m_pitchSlider);
    m_pitchSpinBox = new QDoubleSpinBox;
    m_pitchSpinBox->setRange(-15.0, 15.0);
    m_pitchSpinBox->setValue(0.0);
    m_pitchSpinBox->setSingleStep(0.1);
    m_pitchSpinBox->setDecimals(1);
    m_pitchSpinBox->setSuffix(" st");
    pitchLayout->addWidget(m_pitchSpinBox);
    mainLayout->addLayout(pitchLayout);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainLayout->addWidget(m_scrollArea);

    m_scrollContent = new QWidget;
    m_layout = new QGridLayout(m_scrollContent);
    m_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_scrollArea->setWidget(m_scrollContent);
    createBands();

    connect(m_preampSlider, &QSlider::valueChanged, this, &EqualizerWidget::onPreampSliderMoved);
    connect(m_preampSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &EqualizerWidget::onPreampSpinBoxChanged);
    connect(m_speedSlider, &QSlider::valueChanged, this, &EqualizerWidget::onSpeedSliderMoved);
    connect(m_speedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EqualizerWidget::onSpeedSpinBoxChanged);
    connect(m_pitchSlider, &QSlider::valueChanged, this, &EqualizerWidget::onPitchSliderMoved);
    connect(m_pitchSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &EqualizerWidget::onPitchSpinBoxChanged);
}

void EqualizerWidget::createBands() {
    QVector<double> freqs = {0.5,20,40,75,150,300,800,1200,2500,4000,6000,10000,13000,16000,19000,22000,25000};
    m_bands.resize(freqs.size());
    for (int i=0; i<freqs.size(); ++i) {
        double f = freqs[i];
        QWidget *cell = new QWidget;
        QVBoxLayout *vbox = new QVBoxLayout(cell);
        vbox->setAlignment(Qt::AlignCenter);
        QString label = (f<1000) ? QString::number(f)+" Hz" : QString::number(f/1000,'f',1)+" kHz";
        QLabel *fl = new QLabel(label);
        fl->setAlignment(Qt::AlignCenter);
        vbox->addWidget(fl);
        QSlider *sl = new QSlider(Qt::Vertical);
        sl->setRange(-500, 500);
        sl->setValue(0);
        sl->setTickInterval(50);
        sl->setTickPosition(QSlider::TicksBothSides);
        sl->setFixedHeight(200);
        vbox->addWidget(sl, 0, Qt::AlignHCenter);
        QSpinBox *sb = new QSpinBox;
        sb->setRange(-500, 500);
        sb->setValue(0);
        sb->setSuffix(" dB");
        vbox->addWidget(sb);
        m_layout->addWidget(cell, 0, i);
        m_bands[i] = {f, sl, sb};
        connect(sl, &QSlider::valueChanged, this, [this,i](int v){ onSliderMoved(i,v); });
        connect(sb, QOverload<int>::of(&QSpinBox::valueChanged), this, [this,i](int v){ onSpinBoxChanged(i,v); });
    }
}

void EqualizerWidget::onSliderMoved(int idx, int val) {
    m_bands[idx].spinBox->blockSignals(true);
    m_bands[idx].spinBox->setValue(val);
    m_bands[idx].spinBox->blockSignals(false);
    emit bandGainChanged(m_bands[idx].freq, val);
    if (m_audioManager) m_audioManager->setEqualizerGain(idx, (float)val);
}
void EqualizerWidget::onSpinBoxChanged(int idx, int val) {
    m_bands[idx].slider->blockSignals(true);
    m_bands[idx].slider->setValue(val);
    m_bands[idx].slider->blockSignals(false);
    emit bandGainChanged(m_bands[idx].freq, val);
    if (m_audioManager) m_audioManager->setEqualizerGain(idx, (float)val);
}
void EqualizerWidget::onPreampSliderMoved(int v) {
    m_preampSpinBox->blockSignals(true);
    m_preampSpinBox->setValue(v);
    m_preampSpinBox->blockSignals(false);
    emit preampGainChanged(v);
    if (m_audioManager) m_audioManager->setPreampGain((float)v);
}
void EqualizerWidget::onPreampSpinBoxChanged(int v) {
    m_preampSlider->blockSignals(true);
    m_preampSlider->setValue(v);
    m_preampSlider->blockSignals(false);
    emit preampGainChanged(v);
    if (m_audioManager) m_audioManager->setPreampGain((float)v);
}
void EqualizerWidget::onSpeedSliderMoved(int v) {
    double speed = v / 100.0;
    m_speedSpinBox->blockSignals(true);
    m_speedSpinBox->setValue(speed);
    m_speedSpinBox->blockSignals(false);
    emit speedChanged(speed);
    if (m_audioManager) m_audioManager->setPlaybackSpeed(speed);
}
void EqualizerWidget::onSpeedSpinBoxChanged(double speed) {
    int sliderVal = (int)(speed * 100);
    m_speedSlider->blockSignals(true);
    m_speedSlider->setValue(sliderVal);
    m_speedSlider->blockSignals(false);
    emit speedChanged(speed);
    if (m_audioManager) m_audioManager->setPlaybackSpeed(speed);
}
void EqualizerWidget::onPitchSliderMoved(int v) {
    double pitch = v / 10.0;
    m_pitchSpinBox->blockSignals(true);
    m_pitchSpinBox->setValue(pitch);
    m_pitchSpinBox->blockSignals(false);
    emit pitchChanged(pitch);
    if (m_audioManager) m_audioManager->setPitchShift(pitch);
}
void EqualizerWidget::onPitchSpinBoxChanged(double pitch) {
    int sliderVal = (int)(pitch * 10);
    m_pitchSlider->blockSignals(true);
    m_pitchSlider->setValue(sliderVal);
    m_pitchSlider->blockSignals(false);
    emit pitchChanged(pitch);
    if (m_audioManager) m_audioManager->setPitchShift(pitch);
}
QMap<double,int> EqualizerWidget::getBandGains() const {
    QMap<double,int> g;
    for (const Band &b : m_bands) g[b.freq] = b.slider->value();
    return g;
}
int EqualizerWidget::getPreampGain() const { return m_preampSlider->value(); }