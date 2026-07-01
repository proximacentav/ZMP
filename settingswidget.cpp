#include "settingswidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QIntValidator>
#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <QPushButton>
#include <QComboBox>
#include <QDialog>
#include <QDebug>
#include <QLineEdit>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent), m_darkTheme(false), m_accentColor(42,130,218)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Максимальный битрейт (kbps):"));
    m_bitrateSlider = new QSlider(Qt::Horizontal);
    m_bitrateSlider->setRange(1,1000); m_bitrateSlider->setValue(320);
    layout->addWidget(m_bitrateSlider);
    m_bitrateEdit = new QLineEdit;
    m_bitrateEdit->setValidator(new QIntValidator(1,99999));
    m_bitrateEdit->setText("320");
    layout->addWidget(m_bitrateEdit);
    QLabel *note = new QLabel("0 или значение >1000 означает 'не ограничено'");
    note->setStyleSheet("color:gray; font-size:10px;");
    layout->addWidget(note);

    layout->addWidget(new QLabel("Высота области метаданных (px):"));
    m_heightSlider = new QSlider(Qt::Horizontal);
    m_heightSlider->setRange(150,400); m_heightSlider->setValue(220);
    m_heightSlider->setTickInterval(10);
    m_heightSlider->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(m_heightSlider);

    layout->addWidget(new QLabel("Размер иконок (px):"));
    m_iconSizeSlider = new QSlider(Qt::Horizontal);
    m_iconSizeSlider->setRange(10,120); m_iconSizeSlider->setValue(40);
    m_iconSizeSlider->setTickInterval(5);
    m_iconSizeSlider->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(m_iconSizeSlider);

    layout->addWidget(new QLabel("чувствительность спектрограммы:"));
    m_spectrumGainSlider = new QSlider(Qt::Horizontal);
    m_spectrumGainSlider->setRange(10, 1600); 
    m_spectrumGainSlider->setValue(800);
    m_spectrumGainSlider->setTickPosition(QSlider::TicksBelow);
    m_spectrumGainSlider->setTickInterval(50);
    layout->addWidget(m_spectrumGainSlider);

    layout->addWidget(new QLabel("частота обновления спектраграммы (FPS):"));
    m_spectrumFpsCombo = new QComboBox;
    m_spectrumFpsCombo->addItems({"5", "10", "20", "40", "60", "90", "120", "144", "240", "360", "480", "720", "900", "1600", "1000000"});
    m_spectrumFpsCombo->setCurrentIndex(2);
    layout->addWidget(m_spectrumFpsCombo);
    
    connect(m_spectrumFpsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        emit spectrumFpsChanged(m_spectrumFpsCombo->itemText(index).toInt());
    });

    m_aboutButton = new QPushButton("О программе");
    layout->addWidget(m_aboutButton);

    QHBoxLayout *themeLayout = new QHBoxLayout;
    m_themeButton = new QPushButton("Тёмная тема");
    themeLayout->addWidget(m_themeButton);
    m_colorCombo = new QComboBox;
    m_colorMap["Синий"] = QColor(42,130,218);
    m_colorMap["Красный"] = QColor(218,42,42);
    m_colorMap["Зелёный"] = QColor(42,218,42);
    m_colorMap["Фиолетовый"] = QColor(142,42,218);
    m_colorMap["Коричневый"] = QColor(160,80,40);
    for (const QString &name : m_colorMap.keys()) m_colorCombo->addItem(name);
    m_colorCombo->setCurrentIndex(0);
    themeLayout->addWidget(m_colorCombo);
    layout->addLayout(themeLayout);

    m_exitButton = new QPushButton("Выйти из программы");
    layout->addWidget(m_exitButton);

    connect(m_bitrateSlider, &QSlider::valueChanged, this, &SettingsWidget::onSliderChanged);
    connect(m_bitrateEdit, &QLineEdit::editingFinished, this, &SettingsWidget::onLineEditChanged);
    connect(m_themeButton, &QPushButton::clicked, this, &SettingsWidget::toggleTheme);
    connect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::onColorChanged);
    connect(m_heightSlider, &QSlider::valueChanged, this, &SettingsWidget::onHeightSliderChanged);
    connect(m_iconSizeSlider, &QSlider::valueChanged, this, &SettingsWidget::onIconSizeSliderChanged);
    connect(m_aboutButton, &QPushButton::clicked, this, &SettingsWidget::showAboutDialog);
    connect(m_exitButton, &QPushButton::clicked, this, &SettingsWidget::exitRequested);
    connect(m_spectrumGainSlider, &QSlider::valueChanged, this, &SettingsWidget::onSpectrumGainChanged);

    applyTheme(true);
    applyAccentColor();
}

int SettingsWidget::maxBitrate() const {
    int val = m_bitrateEdit->text().toInt();
    return (val <= 0 || val > 1000) ? 0 : val;
}
void SettingsWidget::onSliderChanged(int v) { m_bitrateEdit->setText(QString::number(v)); }
void SettingsWidget::onLineEditChanged() {
    int v = m_bitrateEdit->text().toInt();
    if (v < 1) v = 1; if (v > 1000) v = 1000;
    m_bitrateSlider->setValue(v);
}
void SettingsWidget::toggleTheme() {
    m_darkTheme = !m_darkTheme;
    applyTheme(m_darkTheme);
    m_themeButton->setText(m_darkTheme ? "Светлая тема" : "Тёмная тема");
    applyAccentColor();
}
void SettingsWidget::onColorChanged(int index) {
    if (index >= 0 && index < m_colorCombo->count()) {
        m_accentColor = m_colorMap[m_colorCombo->itemText(index)];
        applyAccentColor();
        emit accentColorChanged(m_accentColor);
    }
}
void SettingsWidget::onHeightSliderChanged(int v) { emit metadataHeightChanged(v); }
void SettingsWidget::onIconSizeSliderChanged(int v) { emit iconSizeChanged(v); }
void SettingsWidget::onIconSizeChanged(int v) { m_iconSizeSlider->blockSignals(true); m_iconSizeSlider->setValue(v); m_iconSizeSlider->blockSignals(false); }
void SettingsWidget::showAboutDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle("О программе");
    dlg.resize(400,300);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel("version 0.9.0 (Zavtra)\nby proximacentav..\nhttps://github.com/proximacentav/ZMP\nMIT license\nBETA VERSION"));
    QPushButton *closeBtn = new QPushButton("Закрыть");
    l->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}
void SettingsWidget::applyTheme(bool dark) {
    QPalette pal;
    if (dark) {
        pal.setColor(QPalette::Window, QColor(53,53,53));
        pal.setColor(QPalette::WindowText, Qt::white);
        pal.setColor(QPalette::Base, QColor(25,25,25));
        pal.setColor(QPalette::AlternateBase, QColor(53,53,53));
        pal.setColor(QPalette::ToolTipBase, Qt::black);
        pal.setColor(QPalette::ToolTipText, Qt::white);
        pal.setColor(QPalette::Text, Qt::white);
        pal.setColor(QPalette::Button, QColor(53,53,53));
        pal.setColor(QPalette::ButtonText, Qt::white);
        pal.setColor(QPalette::BrightText, Qt::red);
        pal.setColor(QPalette::Link, QColor(42,130,218));
    } else {
        pal = qApp->style()->standardPalette();
    }
    qApp->setPalette(pal);
}
void SettingsWidget::applyAccentColor() {
    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Highlight, m_accentColor);
    qApp->setPalette(pal);

    QString sliderStyle = QString(
        "QSlider::groove:horizontal { height:4px; background:palette(mid); border-radius:2px; }"
        "QSlider::sub-page:horizontal { background:%1; border-radius:2px; }"
        "QSlider::handle:horizontal { background:%1; width:16px; height:16px; margin:-6px 0; border-radius:8px; }"
        "QSlider::handle:horizontal:hover { background:%2; }"
    ).arg(m_accentColor.name()).arg(m_accentColor.darker(120).name());

    QString menuItemStyle = QString(
        "QListWidget::item:selected { background-color:%1; color:white; }"
    ).arg(m_accentColor.name());

    qApp->setStyleSheet(sliderStyle + "\n" + menuItemStyle);
}
void SettingsWidget::onSpectrumGainChanged(int value) {
    emit spectrumGainChanged(value / 100.0f);
}
