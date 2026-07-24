#include "settingswidget.h"
#include "translator.h"
#include <QVariant>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QIntValidator>
#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QDebug>
#include <QLineEdit>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QKeyEvent>
#include <QStandardPaths>
#include <QStackedWidget>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent), m_darkTheme(false), m_accentColor(42,130,218)
{
    m_stackedWidget = new QStackedWidget(this);
    
    setupMainSettingsTab();
    setupKeyBindingTab();
    
    m_stackedWidget->addWidget(m_mainSettingsWidget);
    m_stackedWidget->addWidget(m_keyBindingWidget);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stackedWidget);
    
    loadKeyBindingsFromConfig();
    updateKeyBindingButtons();

    applyTheme(true);
    applyAccentColor();

    connect(&Translator::instance(), &Translator::languageChanged,
            this, &SettingsWidget::retranslateUi);
}

void SettingsWidget::retranslateUi() {
    runRetrans(m_retrans);
}

int SettingsWidget::maxBitrate() const {
    int val = m_bitrateEdit->text().toInt();
    return (val <= 0 || val > 1000) ? 0 : val;
}
void SettingsWidget::onSliderChanged(int v) {
    m_bitrateEdit->setText(QString::number(v));
    emit maxBitrateChanged(maxBitrate());
}
void SettingsWidget::onLineEditChanged() {
    int v = m_bitrateEdit->text().toInt();
    if (v < 1) v = 1; if (v > 1000) v = 1000;
    m_bitrateSlider->setValue(v);
    emit maxBitrateChanged(maxBitrate());
}
void SettingsWidget::toggleTheme() {
    m_darkTheme = !m_darkTheme;
    applyTheme(m_darkTheme);
    m_themeButton->setText(m_darkTheme ? ztr("Светлая тема") : ztr("Тёмная тема"));
    applyAccentColor();
}
void SettingsWidget::onColorChanged(int index) {
    if (index >= 0 && index < m_colorCombo->count()) {
        m_accentColor = m_colorCombo->itemData(index).value<QColor>();
        applyAccentColor();
        emit accentColorChanged(m_accentColor);
    }
}
void SettingsWidget::onHeightSliderChanged(int v) { emit metadataHeightChanged(v); }
void SettingsWidget::onIconSizeSliderChanged(int v) { emit iconSizeChanged(v); }
void SettingsWidget::onIconSizeChanged(int v) { m_iconSizeSlider->blockSignals(true); m_iconSizeSlider->setValue(v); m_iconSizeSlider->blockSignals(false); }
void SettingsWidget::showAboutDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle(ztr("О программе"));
    dlg.resize(400,300);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel(ztr("version 1.5.0 (localization)\nby proximacentav..\nhttps://github.com/proximacentav/ZMP\nMIT license\nRELEASE\nтакже был использован projectM")));
    QPushButton *closeBtn = new QPushButton(ztr("Закрыть"));
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

void SettingsWidget::setupMainSettingsTab() {
    m_mainSettingsWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(m_mainSettingsWidget);

    layout->addStretch(1);

    // Language selector
    QHBoxLayout *langLayout = new QHBoxLayout;
    langLayout->addWidget(ztrLabel(m_retrans, "Язык:"));
    m_languageCombo = new QComboBox;
    m_languageCombo->addItem(Translator::nativeName(Translator::Russian), int(Translator::Russian));
    m_languageCombo->addItem(Translator::nativeName(Translator::English), int(Translator::English));
    m_languageCombo->addItem(Translator::nativeName(Translator::German),  int(Translator::German));
    for (int i = 0; i < m_languageCombo->count(); ++i) {
        if (m_languageCombo->itemData(i).toInt() == int(Translator::instance().language())) {
            m_languageCombo->setCurrentIndex(i);
            break;
        }
    }
    langLayout->addWidget(m_languageCombo, 1);
    layout->addLayout(langLayout);
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx){
        if (idx < 0) return;
        Translator::instance().setLanguage(
            static_cast<Translator::Language>(m_languageCombo->itemData(idx).toInt()));
    });

    layout->addWidget(ztrLabel(m_retrans, "Максимальный битрейт (kbps):"));
    m_bitrateSlider = new QSlider(Qt::Horizontal);
    m_bitrateSlider->setRange(1,1000); m_bitrateSlider->setValue(320);
    layout->addWidget(m_bitrateSlider);
    m_bitrateEdit = new QLineEdit;
    m_bitrateEdit->setValidator(new QIntValidator(1,99999));
    m_bitrateEdit->setText("320");
    layout->addWidget(m_bitrateEdit);
    QLabel *note = ztrLabel(m_retrans, "0 или значение >1000 означает 'не ограничено'");
    note->setStyleSheet("color:gray; font-size:10px;");
    layout->addWidget(note);

    layout->addWidget(ztrLabel(m_retrans, "Высота области метаданных (px):"));
    m_heightSlider = new QSlider(Qt::Horizontal);
    m_heightSlider->setRange(150,400); m_heightSlider->setValue(220);
    m_heightSlider->setTickInterval(10);
    m_heightSlider->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(m_heightSlider);

    layout->addWidget(ztrLabel(m_retrans, "Размер иконок (px):"));
    m_iconSizeSlider = new QSlider(Qt::Horizontal);
    m_iconSizeSlider->setRange(16, 64);
    m_iconSizeSlider->setValue(32);
    m_iconSizeSlider->setTickPosition(QSlider::TicksBelow);
    m_iconSizeSlider->setTickInterval(4);
    layout->addWidget(m_iconSizeSlider);

    layout->addWidget(ztrLabel(m_retrans, "чувствительность спектрограммы:"));
    m_spectrumGainSlider = new QSlider(Qt::Horizontal);
    m_spectrumGainSlider->setRange(10, 1600);
    m_spectrumGainSlider->setValue(800);
    m_spectrumGainSlider->setTickPosition(QSlider::TicksBelow);
    m_spectrumGainSlider->setTickInterval(50);
    layout->addWidget(m_spectrumGainSlider);

    layout->addWidget(ztrLabel(m_retrans, "частота обновления спектраграммы (FPS):"));
    m_spectrumFpsCombo = new QComboBox;
    m_spectrumFpsCombo->addItems({"5", "10", "20", "40", "60", "90", "120", "144", "240", "360", "480", "720", "900", "1600", "1000000"});
    m_spectrumFpsCombo->setCurrentIndex(2);
    layout->addWidget(m_spectrumFpsCombo);

    connect(m_spectrumFpsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        emit spectrumFpsChanged(m_spectrumFpsCombo->itemText(index).toInt());
    });

    layout->addWidget(ztrLabel(m_retrans, "количество частот спектрограммы:"));
    m_spectrumBandsCombo = new QComboBox;
    m_spectrumBandsCombo->addItems({"6", "25", "64", "120", "128", "256", "512", "1024", "2048", "4096", "8192", "16000"});
    m_spectrumBandsCombo->setCurrentIndex(2);
    layout->addWidget(m_spectrumBandsCombo);
    connect(m_spectrumBandsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        emit spectrumBandsChanged(m_spectrumBandsCombo->itemText(index).toInt());
    });

    layout->addWidget(ztrLabel(m_retrans, "projectM пресет:"));
    QHBoxLayout *projectMLayout = new QHBoxLayout;
    m_projectMPresetButton = ztrButton(m_retrans, "Выбрать .milk файл");
    projectMLayout->addWidget(m_projectMPresetButton);
    m_projectMPresetPath = new QLabel("");
    m_projectMPresetPath->setStyleSheet("color: #888; font-size: 10px;");
    m_projectMPresetPath->setWordWrap(true);
    projectMLayout->addWidget(m_projectMPresetPath, 1);
    layout->addLayout(projectMLayout);
    connect(m_projectMPresetButton, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, ztr("Выберите .milk пресет"), QString(), "Milk presets (*.milk);;All files (*)");
        if (!path.isEmpty()) {
            m_projectMPresetPath->setText(path);
            emit projectMPresetSelected(path);
        }
    });

    m_powerModeCheck = new QCheckBox("Powermode");
    layout->addWidget(m_powerModeCheck);
    connect(m_powerModeCheck, &QCheckBox::toggled, this, &SettingsWidget::powerModeChanged);

    m_aboutButton = ztrButton(m_retrans, "О программе");
    layout->addWidget(m_aboutButton);

    QHBoxLayout *themeLayout = new QHBoxLayout;
    m_themeButton = new QPushButton;
    ztrRegister(m_retrans, [this]{
        m_themeButton->setText(m_darkTheme ? ztr("Светлая тема") : ztr("Тёмная тема"));
    });
    themeLayout->addWidget(m_themeButton);
    m_colorCombo = new QComboBox;
    struct ColorEntry { const char *ru; QColor color; };
    const ColorEntry colorEntries[] = {
        {"Синий",       QColor(42,130,218)},
        {"Красный",     QColor(218,42,42)},
        {"Зелёный",     QColor(42,218,42)},
        {"Фиолетовый",  QColor(142,42,218)},
        {"Коричневый",  QColor(160,80,40)}
    };
    for (const ColorEntry &e : colorEntries) {
        const int idx = m_colorCombo->count();
        const QString ruKey = QString::fromUtf8(e.ru);
        m_colorCombo->addItem(ztr(ruKey), QVariant::fromValue(e.color)); // display text + color payload
        ztrRegister(m_retrans, [this, idx, ruKey]{ m_colorCombo->setItemText(idx, ztr(ruKey)); });
    }
    m_colorCombo->setCurrentIndex(0);
    themeLayout->addWidget(m_colorCombo);
    layout->addLayout(themeLayout);

    m_exitButton = ztrButton(m_retrans, "Выйти из программы");
    layout->addWidget(m_exitButton);

    m_keyBindingButton = ztrButton(m_retrans, "Клавиши");
    m_keyBindingButton->setStyleSheet("font-weight: bold; font-size: 14px; padding: 10px;");
    layout->addWidget(m_keyBindingButton);
    connect(m_keyBindingButton, &QPushButton::clicked, this, &SettingsWidget::showKeyBindingTab);

    connect(m_bitrateSlider, &QSlider::valueChanged, this, &SettingsWidget::onSliderChanged);
    connect(m_bitrateEdit, &QLineEdit::editingFinished, this, &SettingsWidget::onLineEditChanged);
    connect(m_themeButton, &QPushButton::clicked, this, &SettingsWidget::toggleTheme);
    connect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::onColorChanged);
    connect(m_heightSlider, &QSlider::valueChanged, this, &SettingsWidget::onHeightSliderChanged);
    connect(m_iconSizeSlider, &QSlider::valueChanged, this, &SettingsWidget::onIconSizeSliderChanged);
    connect(m_aboutButton, &QPushButton::clicked, this, &SettingsWidget::showAboutDialog);
    connect(m_exitButton, &QPushButton::clicked, this, &SettingsWidget::exitRequested);
    connect(m_spectrumGainSlider, &QSlider::valueChanged, this, &SettingsWidget::onSpectrumGainChanged);
    
    layout->addStretch();
}

void SettingsWidget::setupKeyBindingTab() {
    m_keyBindingWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(m_keyBindingWidget);
    
    layout->addStretch(1);
    
    QHBoxLayout *headerLayout = new QHBoxLayout;
    m_backButton = ztrButton(m_retrans, "Назад");
    m_backButton->setFixedWidth(80);
    headerLayout->addWidget(m_backButton);
    headerLayout->addStretch();
    QLabel *titleLabel = ztrLabel(m_retrans, "Назначение клавиш");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: palette(highlight);");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    connect(m_backButton, &QPushButton::clicked, this, &SettingsWidget::showMainSettingsTab);

    static const char *actionNames[KeyActionCount] = {
        "Предыдущий трек",
        "Следующий трек",
        "Снять с паузы/Запустить трек",
        "Пресет эквалайзера"
    };

    for (int i = 0; i < KeyActionCount; ++i) {
        KeyAction action = static_cast<KeyAction>(i);
        QHBoxLayout *actionLayout = new QHBoxLayout;

        QLabel *actionLabel = ztrLabel(m_retrans, actionNames[i]);
        actionLabel->setMinimumWidth(200);
        actionLabel->setStyleSheet("font-size: 14px;");
        actionLayout->addWidget(actionLabel);

        actionLayout->addStretch();

        m_keyLabels[i] = new QLabel();
        m_keyLabels[i]->setMinimumWidth(120);
        m_keyLabels[i]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_keyLabels[i]->setStyleSheet("font-size: 14px; color: palette(highlight); font-weight: bold;");
        actionLayout->addWidget(m_keyLabels[i]);

        m_keyCaptureButtons[i] = new QPushButton;
        m_keyCaptureButtons[i]->setFixedSize(120, 40);
        updateKeyBindingButtonStyle(i, false);
        m_keyCaptureButtons[i]->setProperty("keyAction", i);
        actionLayout->addWidget(m_keyCaptureButtons[i]);

        connect(m_keyCaptureButtons[i], &QPushButton::clicked, this, [this, action]() {
            onKeyCaptureButtonClicked(action);
        });

        layout->addLayout(actionLayout);
    }

    // Key labels/buttons carry translatable state text ("КЛАВИША" / "не назначено"),
    // so refresh them whenever the language changes.
    ztrRegister(m_retrans, [this]{ updateKeyBindingButtons(); });

    layout->addStretch();
}

void SettingsWidget::showKeyBindingTab() {
    m_stackedWidget->setCurrentWidget(m_keyBindingWidget);
}

void SettingsWidget::showMainSettingsTab() {
    m_stackedWidget->setCurrentWidget(m_mainSettingsWidget);
    saveKeyBindingsToConfig();
    emit keyBindingsSaved();
}

void SettingsWidget::onKeyCaptureButtonClicked(KeyAction action) {
    if (m_waitingForKey) return;
    
    m_waitingForKey = true;
    m_currentKeyAction = action;
    
    m_keyCaptureButtons[action]->setText(ztr("Нажмите клавишу..."));
    updateKeyBindingButtonStyle(action, true);
    m_keyCaptureButtons[action]->setEnabled(false);
    
    for (int i = 0; i < KeyActionCount; ++i) {
        if (i != action) {
            m_keyCaptureButtons[i]->setEnabled(false);
        }
    }
    m_backButton->setEnabled(false);
    
    QApplication::instance()->installEventFilter(this);
}

bool SettingsWidget::eventFilter(QObject *watched, QEvent *event) {
    if (m_waitingForKey && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        Qt::Key key = static_cast<Qt::Key>(keyEvent->key());
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
        
        if (key == Qt::Key_unknown) {
            key = static_cast<Qt::Key>(keyEvent->nativeVirtualKey());
        }
        
        onKeyCaptured(key, modifiers);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void SettingsWidget::onKeyCaptured(Qt::Key key, Qt::KeyboardModifiers modifiers) {
    m_waitingForKey = false;
    QApplication::instance()->removeEventFilter(this);
    
    KeyBinding binding;
    binding.key = key;
    binding.modifiers = modifiers;
    binding.displayName = keyToString(key, modifiers);
    
    m_keyBindings[m_currentKeyAction] = binding;
    
    updateKeyBindingButton(m_currentKeyAction);
    emit keyBindingChanged(m_currentKeyAction, binding);
    
    for (int i = 0; i < KeyActionCount; ++i) {
        m_keyCaptureButtons[i]->setEnabled(true);
    }
    m_backButton->setEnabled(true);
}

void SettingsWidget::updateKeyBindingButtonStyle(int index, bool isWaiting) {
    QPushButton *btn = m_keyCaptureButtons[index];
    if (isWaiting) {
        btn->setStyleSheet("font-weight: bold; font-size: 14px; background-color: #FF9800; color: white; border-radius: 4px;");
    } else {
        btn->setStyleSheet("font-weight: bold; font-size: 14px; background-color: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 4px;");
    }
}

void SettingsWidget::updateKeyBindingButton(KeyAction action) {
    const KeyBinding &binding = m_keyBindings[action];
    if (binding.key != Qt::Key_unknown) {
        m_keyCaptureButtons[action]->setText(binding.displayName);
        m_keyLabels[action]->setText("");
    } else {
        m_keyCaptureButtons[action]->setText(ztr("КЛАВИША"));
        m_keyLabels[action]->setText(ztr("не назначено"));
    }
    updateKeyBindingButtonStyle(action, false);
}

void SettingsWidget::updateKeyBindingButtons() {
    for (int i = 0; i < KeyActionCount; ++i) {
        updateKeyBindingButton(static_cast<KeyAction>(i));
    }
}

QString SettingsWidget::keyToString(Qt::Key key, Qt::KeyboardModifiers modifiers) {
    QStringList parts;
    
    if (modifiers & Qt::ControlModifier) parts << "Ctrl";
    if (modifiers & Qt::AltModifier) parts << "Alt";
    if (modifiers & Qt::ShiftModifier) parts << "Shift";
    if (modifiers & Qt::MetaModifier) parts << "Meta";
    
    QString keyName;
    switch (key) {
        case Qt::Key_Escape: keyName = "Esc"; break;
        case Qt::Key_Tab: keyName = "Tab"; break;
        case Qt::Key_Backspace: keyName = "Backspace"; break;
        case Qt::Key_Return: keyName = "Enter"; break;
        case Qt::Key_Enter: keyName = "Enter"; break;
        case Qt::Key_Insert: keyName = "Insert"; break;
        case Qt::Key_Delete: keyName = "Del"; break;
        case Qt::Key_Pause: keyName = "Pause"; break;
        case Qt::Key_Print: keyName = "Print"; break;
        case Qt::Key_SysReq: keyName = "SysReq"; break;
        case Qt::Key_Clear: keyName = "Clear"; break;
        case Qt::Key_Home: keyName = "Home"; break;
        case Qt::Key_End: keyName = "End"; break;
        case Qt::Key_Left: keyName = "Left"; break;
        case Qt::Key_Up: keyName = "Up"; break;
        case Qt::Key_Right: keyName = "Right"; break;
        case Qt::Key_Down: keyName = "Down"; break;
        case Qt::Key_PageUp: keyName = "PgUp"; break;
        case Qt::Key_PageDown: keyName = "PgDown"; break;
        case Qt::Key_CapsLock: keyName = "CapsLock"; break;
        case Qt::Key_NumLock: keyName = "NumLock"; break;
        case Qt::Key_ScrollLock: keyName = "ScrollLock"; break;
        case Qt::Key_F1: keyName = "F1"; break;
        case Qt::Key_F2: keyName = "F2"; break;
        case Qt::Key_F3: keyName = "F3"; break;
        case Qt::Key_F4: keyName = "F4"; break;
        case Qt::Key_F5: keyName = "F5"; break;
        case Qt::Key_F6: keyName = "F6"; break;
        case Qt::Key_F7: keyName = "F7"; break;
        case Qt::Key_F8: keyName = "F8"; break;
        case Qt::Key_F9: keyName = "F9"; break;
        case Qt::Key_F10: keyName = "F10"; break;
        case Qt::Key_F11: keyName = "F11"; break;
        case Qt::Key_F12: keyName = "F12"; break;
        case Qt::Key_F13: keyName = "F13"; break;
        case Qt::Key_F14: keyName = "F14"; break;
        case Qt::Key_F15: keyName = "F15"; break;
        case Qt::Key_F16: keyName = "F16"; break;
        case Qt::Key_F17: keyName = "F17"; break;
        case Qt::Key_F18: keyName = "F18"; break;
        case Qt::Key_F19: keyName = "F19"; break;
        case Qt::Key_F20: keyName = "F20"; break;
        case Qt::Key_F21: keyName = "F21"; break;
        case Qt::Key_F22: keyName = "F22"; break;
        case Qt::Key_F23: keyName = "F23"; break;
        case Qt::Key_F24: keyName = "F24"; break;
        case Qt::Key_Space: keyName = "Space"; break;
        case Qt::Key_Apostrophe: keyName = "'"; break;
        case Qt::Key_Comma: keyName = ","; break;
        case Qt::Key_Minus: keyName = "-"; break;
        case Qt::Key_Period: keyName = "."; break;
        case Qt::Key_Slash: keyName = "/"; break;
        case Qt::Key_0: keyName = "0"; break;
        case Qt::Key_1: keyName = "1"; break;
        case Qt::Key_2: keyName = "2"; break;
        case Qt::Key_3: keyName = "3"; break;
        case Qt::Key_4: keyName = "4"; break;
        case Qt::Key_5: keyName = "5"; break;
        case Qt::Key_6: keyName = "6"; break;
        case Qt::Key_7: keyName = "7"; break;
        case Qt::Key_8: keyName = "8"; break;
        case Qt::Key_9: keyName = "9"; break;
        case Qt::Key_Semicolon: keyName = ";"; break;
        case Qt::Key_Equal: keyName = "="; break;
        case Qt::Key_A: keyName = "A"; break;
        case Qt::Key_B: keyName = "B"; break;
        case Qt::Key_C: keyName = "C"; break;
        case Qt::Key_D: keyName = "D"; break;
        case Qt::Key_E: keyName = "E"; break;
        case Qt::Key_F: keyName = "F"; break;
        case Qt::Key_G: keyName = "G"; break;
        case Qt::Key_H: keyName = "H"; break;
        case Qt::Key_I: keyName = "I"; break;
        case Qt::Key_J: keyName = "J"; break;
        case Qt::Key_K: keyName = "K"; break;
        case Qt::Key_L: keyName = "L"; break;
        case Qt::Key_M: keyName = "M"; break;
        case Qt::Key_N: keyName = "N"; break;
        case Qt::Key_O: keyName = "O"; break;
        case Qt::Key_P: keyName = "P"; break;
        case Qt::Key_Q: keyName = "Q"; break;
        case Qt::Key_R: keyName = "R"; break;
        case Qt::Key_S: keyName = "S"; break;
        case Qt::Key_T: keyName = "T"; break;
        case Qt::Key_U: keyName = "U"; break;
        case Qt::Key_V: keyName = "V"; break;
        case Qt::Key_W: keyName = "W"; break;
        case Qt::Key_X: keyName = "X"; break;
        case Qt::Key_Y: keyName = "Y"; break;
        case Qt::Key_Z: keyName = "Z"; break;
        case Qt::Key_BracketLeft: keyName = "["; break;
        case Qt::Key_Backslash: keyName = "\\"; break;
        case Qt::Key_BracketRight: keyName = "]"; break;
        case Qt::Key_QuoteDbl: keyName = "\""; break;
        default:
            if (key >= Qt::Key_0 && key <= Qt::Key_9) {
                keyName = QString(QChar('0' + (key - Qt::Key_0)));
            } else if (key >= Qt::Key_A && key <= Qt::Key_Z) {
                keyName = QString(QChar('A' + (key - Qt::Key_A)));
            } else if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
                keyName = QString("F%1").arg(key - Qt::Key_F1 + 1);
            } else {
                keyName = QString("Key(%1)").arg(static_cast<int>(key));
            }
            break;
    }
    
    if (!keyName.isEmpty()) {
        parts << keyName;
    }
    
    return parts.join(" + ");
}

SettingsWidget::KeyBinding SettingsWidget::stringToKeyBinding(const QString &str) {
    KeyBinding binding;
    binding.displayName = str;
    
    QStringList parts = str.split(" + ", Qt::SkipEmptyParts);
    if (parts.isEmpty()) return binding;
    
    QString keyStr = parts.last();
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    
    for (int i = 0; i < parts.size() - 1; ++i) {
        QString mod = parts[i].toLower();
        if (mod == "ctrl" || mod == "control") modifiers |= Qt::ControlModifier;
        else if (mod == "alt") modifiers |= Qt::AltModifier;
        else if (mod == "shift") modifiers |= Qt::ShiftModifier;
        else if (mod == "meta" || mod == "win" || mod == "super") modifiers |= Qt::MetaModifier;
    }
    
    Qt::Key key = Qt::Key_unknown;
    if (keyStr.length() == 1) {
        QChar c = keyStr.at(0).toUpper();
        if (c >= 'A' && c <= 'Z') {
            key = static_cast<Qt::Key>(Qt::Key_A + (c.unicode() - 'A'));
        } else if (c >= '0' && c <= '9') {
            key = static_cast<Qt::Key>(Qt::Key_0 + (c.unicode() - '0'));
        }
    } else {
        if (keyStr == "Esc" || keyStr == "Escape") key = Qt::Key_Escape;
        else if (keyStr == "Tab") key = Qt::Key_Tab;
        else if (keyStr == "Backspace") key = Qt::Key_Backspace;
        else if (keyStr == "Enter" || keyStr == "Return") key = Qt::Key_Return;
        else if (keyStr == "Insert") key = Qt::Key_Insert;
        else if (keyStr == "Del" || keyStr == "Delete") key = Qt::Key_Delete;
        else if (keyStr == "Pause") key = Qt::Key_Pause;
        else if (keyStr == "Print") key = Qt::Key_Print;
        else if (keyStr == "SysReq") key = Qt::Key_SysReq;
        else if (keyStr == "Clear") key = Qt::Key_Clear;
        else if (keyStr == "Home") key = Qt::Key_Home;
        else if (keyStr == "End") key = Qt::Key_End;
        else if (keyStr == "Left") key = Qt::Key_Left;
        else if (keyStr == "Up") key = Qt::Key_Up;
        else if (keyStr == "Right") key = Qt::Key_Right;
        else if (keyStr == "Down") key = Qt::Key_Down;
        else if (keyStr == "PgUp" || keyStr == "PageUp") key = Qt::Key_PageUp;
        else if (keyStr == "PgDown" || keyStr == "PageDown") key = Qt::Key_PageDown;
        else if (keyStr == "CapsLock") key = Qt::Key_CapsLock;
        else if (keyStr == "NumLock") key = Qt::Key_NumLock;
        else if (keyStr == "ScrollLock") key = Qt::Key_ScrollLock;
        else if (keyStr == "Space") key = Qt::Key_Space;
        else if (keyStr == "'") key = Qt::Key_Apostrophe;
        else if (keyStr == ",") key = Qt::Key_Comma;
        else if (keyStr == "-") key = Qt::Key_Minus;
        else if (keyStr == ".") key = Qt::Key_Period;
        else if (keyStr == "/") key = Qt::Key_Slash;
        else if (keyStr == ";") key = Qt::Key_Semicolon;
        else if (keyStr == "=") key = Qt::Key_Equal;
        else if (keyStr == "[") key = Qt::Key_BracketLeft;
        else if (keyStr == "\\") key = Qt::Key_Backslash;
        else if (keyStr == "]") key = Qt::Key_BracketRight;
        else if (keyStr == "\"") key = Qt::Key_QuoteDbl;
        else if (keyStr.startsWith("F") && keyStr.length() > 1) {
            bool ok;
            int fn = keyStr.mid(1).toInt(&ok);
            if (ok && fn >= 1 && fn <= 24) {
                key = static_cast<Qt::Key>(Qt::Key_F1 + fn - 1);
            }
} else if (keyStr.startsWith("Num")) {
            QString numPart = keyStr.mid(3);
            if (numPart == "0") key = Qt::Key_0;
            else if (numPart == "1") key = Qt::Key_1;
            else if (numPart == "2") key = Qt::Key_2;
            else if (numPart == "3") key = Qt::Key_3;
            else if (numPart == "4") key = Qt::Key_4;
            else if (numPart == "5") key = Qt::Key_5;
            else if (numPart == "6") key = Qt::Key_6;
            else if (numPart == "7") key = Qt::Key_7;
            else if (numPart == "8") key = Qt::Key_8;
            else if (numPart == "9") key = Qt::Key_9;
            else if (numPart == "*") key = Qt::Key_Asterisk;
            else if (numPart == "+") key = Qt::Key_Plus;
            else if (numPart == "-") key = Qt::Key_Minus;
            else if (numPart == ".") key = Qt::Key_Period;
            else if (numPart == "/") key = Qt::Key_Slash;
            else if (numPart == "Enter") key = Qt::Key_Return;
            else if (numPart == "=") key = Qt::Key_Equal;
        }
    }
    
    binding.key = key;
    binding.modifiers = modifiers;
    return binding;
}

void SettingsWidget::loadKeyBindings(const QMap<KeyAction, KeyBinding> &bindings) {
    m_keyBindings = bindings;
    updateKeyBindingButtons();
}

QMap<SettingsWidget::KeyAction, SettingsWidget::KeyBinding> SettingsWidget::getKeyBindings() const {
    return m_keyBindings;
}

void SettingsWidget::createConfigDir() {
    QString configDir = QDir::homePath() + "/zmp_playlists";
    QDir dir;
    if (!dir.exists(configDir)) {
        dir.mkpath(configDir);
    }
}

void SettingsWidget::saveKeyBindingsToConfig() {
    createConfigDir();
    QString configPath = QDir::homePath() + "/zmp_playlists/config.json";
    
    QJsonObject config;
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            config = doc.object();
        }
    }
    
    QJsonObject keyBindingsObj;
    for (auto it = m_keyBindings.begin(); it != m_keyBindings.end(); ++it) {
        QJsonObject bindingObj;
        bindingObj["key"] = static_cast<int>(it.value().key);
        bindingObj["modifiers"] = static_cast<int>(it.value().modifiers);
        bindingObj["displayName"] = it.value().displayName;
        keyBindingsObj[QString::number(static_cast<int>(it.key()))] = bindingObj;
    }
    config["keyBindings"] = keyBindingsObj;
    
    QJsonDocument doc(config);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void SettingsWidget::loadKeyBindingsFromConfig() {
    QString configPath = QDir::homePath() + "/zmp_playlists/config.json";
    QFile file(configPath);
    
    if (!file.exists()) {
        return;
    }
    
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject config = doc.object();
            if (config.contains("keyBindings")) {
                QJsonObject keyBindingsObj = config["keyBindings"].toObject();
                for (auto it = keyBindingsObj.begin(); it != keyBindingsObj.end(); ++it) {
                    bool ok;
                    int action = it.key().toInt(&ok);
                    if (ok && action >= 0 && action < KeyActionCount) {
                        QJsonObject bindingObj = it.value().toObject();
                        KeyBinding binding;
                        binding.key = static_cast<Qt::Key>(bindingObj["key"].toInt());
                        binding.modifiers = static_cast<Qt::KeyboardModifiers>(bindingObj["modifiers"].toInt());
                        binding.displayName = bindingObj["displayName"].toString();
                        m_keyBindings[static_cast<KeyAction>(action)] = binding;
                    }
                }
            }
        }
    }
}
