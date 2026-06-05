#include "equalizerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QDebug>

EqualizerWidget::EqualizerWidget(AudioManager *audioManager, QWidget *parent)
    : QWidget(parent), m_audioManager(audioManager)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Preamp
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
    connect(m_preampSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &EqualizerWidget::onPreampSpinBoxChanged);
}

void EqualizerWidget::createBands()
{
    QVector<double> freqs = {
        0.5, 20, 40, 75, 150, 300, 800, 1200, 2500,
        4000, 6000, 10000, 13000, 16000, 19000, 22000, 25000
    };
    m_bands.resize(freqs.size());

    for (int i = 0; i < freqs.size(); ++i) {
        double freq = freqs[i];
        QWidget *cell = new QWidget;
        QVBoxLayout *vbox = new QVBoxLayout(cell);
        vbox->setAlignment(Qt::AlignCenter);

        QString freqStr = (freq < 1000) ? QString::number(freq) + " Hz" : QString::number(freq/1000, 'f', 1) + " kHz";
        QLabel *freqLabel = new QLabel(freqStr);
        freqLabel->setAlignment(Qt::AlignCenter);
        vbox->addWidget(freqLabel);

        QSlider *slider = new QSlider(Qt::Vertical);
        slider->setRange(-500, 500);
        slider->setValue(0);
        slider->setTickInterval(50);
        slider->setTickPosition(QSlider::TicksBothSides);
        slider->setFixedHeight(200);
        vbox->addWidget(slider, 0, Qt::AlignHCenter);

        QSpinBox *spinBox = new QSpinBox;
        spinBox->setRange(-500, 500);
        spinBox->setValue(0);
        spinBox->setSuffix(" dB");
        vbox->addWidget(spinBox);

        m_layout->addWidget(cell, 0, i);
        m_bands[i] = {freq, slider, spinBox};

        connect(slider, &QSlider::valueChanged, this, [this, i](int val) {
            onSliderMoved(i, val);
        });
        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, i](int val) {
            onSpinBoxChanged(i, val);
        });
    }
}

void EqualizerWidget::onSliderMoved(int bandIndex, int value)
{
    m_bands[bandIndex].spinBox->blockSignals(true);
    m_bands[bandIndex].spinBox->setValue(value);
    m_bands[bandIndex].spinBox->blockSignals(false);
    emit bandGainChanged(m_bands[bandIndex].freq, value);
    if (m_audioManager)
        m_audioManager->setEqualizerGain(bandIndex, static_cast<float>(value));
}

void EqualizerWidget::onSpinBoxChanged(int bandIndex, int value)
{
    m_bands[bandIndex].slider->blockSignals(true);
    m_bands[bandIndex].slider->setValue(value);
    m_bands[bandIndex].slider->blockSignals(false);
    emit bandGainChanged(m_bands[bandIndex].freq, value);
    if (m_audioManager)
        m_audioManager->setEqualizerGain(bandIndex, static_cast<float>(value));
}

void EqualizerWidget::onPreampSliderMoved(int value)
{
    m_preampSpinBox->blockSignals(true);
    m_preampSpinBox->setValue(value);
    m_preampSpinBox->blockSignals(false);
    emit preampGainChanged(value);
    if (m_audioManager)
        m_audioManager->setPreampGain(static_cast<float>(value));
}

void EqualizerWidget::onPreampSpinBoxChanged(int value)
{
    m_preampSlider->blockSignals(true);
    m_preampSlider->setValue(value);
    m_preampSlider->blockSignals(false);
    emit preampGainChanged(value);
    if (m_audioManager)
        m_audioManager->setPreampGain(static_cast<float>(value));
}

QMap<double, int> EqualizerWidget::getBandGains() const
{
    QMap<double, int> gains;
    for (const Band &band : m_bands)
        gains[band.freq] = band.slider->value();
    return gains;
}

int EqualizerWidget::getPreampGain() const
{
    return m_preampSlider->value();
}