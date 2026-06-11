#include "settingswidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QIntValidator>
#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QDialog>
#include <QDebug>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_darkTheme(false)
    , m_accentColor(42, 130, 218)
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

    QLabel *note = new QLabel("любое значение обозначает 'не ограничено'");
    note->setStyleSheet("color: gray; font-size: 10px;");
    layout->addWidget(note);

    layout->addWidget(new QLabel("этот выбор ничего не значит :"));
    m_heightSlider = new QSlider(Qt::Horizontal);
    m_heightSlider->setRange(150, 400);
    m_heightSlider->setValue(220);
    m_heightSlider->setTickInterval(10);
    m_heightSlider->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(m_heightSlider);

    m_aboutButton = new QPushButton("О программе");
    layout->addWidget(m_aboutButton);

    QHBoxLayout *themeLayout = new QHBoxLayout;
    m_themeButton = new QPushButton("Тёмная тема");
    themeLayout->addWidget(m_themeButton);

    m_colorCombo = new QComboBox;
    m_colorMap["Синий"] = QColor(42, 130, 218);
    m_colorMap["Красный"] = QColor(218, 42, 42);
    m_colorMap["Зелёный"] = QColor(42, 218, 42);
    m_colorMap["Фиолетовый"] = QColor(142, 42, 218);
    m_colorMap["Коричневый"] = QColor(160, 80, 40);
    for (const QString &name : m_colorMap.keys()) {
        m_colorCombo->addItem(name);
    }
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
    connect(m_aboutButton, &QPushButton::clicked, this, &SettingsWidget::showAboutDialog);
    connect(m_exitButton, &QPushButton::clicked, this, &SettingsWidget::exitRequested);

    applyTheme(false);
    applyAccentColor();
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
    applyTheme(m_darkTheme);
    m_themeButton->setText(m_darkTheme ? "Светлая тема" : "Тёмная тема");
    applyAccentColor();
}

void SettingsWidget::onColorChanged(int index)
{
    if (index >= 0 && index < m_colorCombo->count()) {
        QString colorName = m_colorCombo->itemText(index);
        m_accentColor = m_colorMap[colorName];
        applyAccentColor();
    }
}

void SettingsWidget::onHeightSliderChanged(int value)
{
    emit metadataHeightChanged(value);
}

void SettingsWidget::showAboutDialog()
{
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle("info");
    aboutDialog.setModal(true);
    aboutDialog.resize(400, 300);

    QVBoxLayout *dialogLayout = new QVBoxLayout(&aboutDialog);
    QLabel *textLabel = new QLabel(
        "version 0.5.0 (лоботомия)\n"
        "by proximacentav..\n"
        "https://github.com/proximacentav/ZMP\n"
        "MIT license\n"
        "BETA VERSION"
    );
    textLabel->setWordWrap(true);
    dialogLayout->addWidget(textLabel);

    QPushButton *closeButton = new QPushButton("Закрыть");
    dialogLayout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, &aboutDialog, &QDialog::accept);

    aboutDialog.exec();
}

void SettingsWidget::applyTheme(bool dark)
{
    QPalette pal;
    if (dark) {
        pal.setColor(QPalette::Window, QColor(53, 53, 53));
        pal.setColor(QPalette::WindowText, Qt::white);
        pal.setColor(QPalette::Base, QColor(25, 25, 25));
        pal.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        pal.setColor(QPalette::ToolTipBase, Qt::black);
        pal.setColor(QPalette::ToolTipText, Qt::white);
        pal.setColor(QPalette::Text, Qt::white);
        pal.setColor(QPalette::Button, QColor(53, 53, 53));
        pal.setColor(QPalette::ButtonText, Qt::white);
        pal.setColor(QPalette::BrightText, Qt::red);
        pal.setColor(QPalette::Link, QColor(42, 130, 218));
    } else {
        pal = qApp->style()->standardPalette();
    }
    qApp->setPalette(pal);
}

void SettingsWidget::applyAccentColor()
{
    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Highlight, m_accentColor);
    qApp->setPalette(pal);

    QString sliderStyle = QString(
        "QSlider::groove:horizontal {"
        "    height: 4px;"
        "    background: palette(mid);"
        "    border-radius: 2px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background: %1;"
        "    border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "    background: %1;"
        "    width: 16px;"
        "    height: 16px;"
        "    margin: -6px 0;"
        "    border-radius: 8px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "    background: %2;"
        "}"
    ).arg(m_accentColor.name()).arg(m_accentColor.darker(120).name());

    QString menuItemStyle = QString(
        "QListWidget::item:selected {"
        "    background-color: %1;"
        "    color: white;"
        "}"
    ).arg(m_accentColor.name());

    QString combinedStyle = sliderStyle + "\n" + menuItemStyle;
    qApp->setStyleSheet(combinedStyle);
}