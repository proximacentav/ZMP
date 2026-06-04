#include "settingswidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QIntValidator>
#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <QPushButton>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_darkTheme(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Максимальный битрейт (kbps):"));

    m_bitrateSlider = new QSlider(Qt::Horizontal);
    m_bitrateSlider->setRange(1, 1000);
    m_bitrateSlider->setValue(320);
    layout->addWidget(m_bitrateSlider);

    m_bitrateEdit = new QLineEdit;
    m_bitrateEdit->setValidator(new QIntValidator(1, 99999));
    m_bitrateEdit->setText("320");
    layout->addWidget(m_bitrateEdit);

    QLabel *note = new QLabel("0 или значение >1000 азначает 'не ограничено'");
    note->setStyleSheet("color: gray; font-size: 10px;");
    layout->addWidget(note);

    m_themeButton = new QPushButton("Тёмная тема");
    layout->addWidget(m_themeButton);

    m_exitButton = new QPushButton("Выйти из программы");
    layout->addWidget(m_exitButton);

    connect(m_bitrateSlider, &QSlider::valueChanged, this, &SettingsWidget::onSliderChanged);
    connect(m_bitrateEdit, &QLineEdit::editingFinished, this, &SettingsWidget::onLineEditChanged);
    connect(m_themeButton, &QPushButton::clicked, this, &SettingsWidget::toggleTheme);
    connect(m_exitButton, &QPushButton::clicked, this, &SettingsWidget::exitRequested);
}

int SettingsWidget::maxBitrate() const
{
    int val = m_bitrateEdit->text().toInt();
    if (val <= 0 || val > 1000) return 0;
    return val;
}

void SettingsWidget::onSliderChanged(int value)
{
    m_bitrateEdit->setText(QString::number(value));
}

void SettingsWidget::onLineEditChanged()
{
    int val = m_bitrateEdit->text().toInt();
    if (val < 1) val = 1;
    if (val > 1000) val = 1000;
    m_bitrateSlider->setValue(val);
}

void SettingsWidget::toggleTheme()
{
    m_darkTheme = !m_darkTheme;
    if (m_darkTheme) {
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::black);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(darkPalette);
        m_themeButton->setText("Светлая тема");
    } else {
        qApp->setPalette(qApp->style()->standardPalette());
        m_themeButton->setText("Тёмная тема");
    }
}