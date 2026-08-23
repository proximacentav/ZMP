#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QMap>
#include <QColor>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include "translator.h"

class QSlider;
class QComboBox;
class QLineEdit;
class QCheckBox;
class DependencyCheckDialog;

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    int maxBitrate() const;

    enum KeyAction {
        PrevTrack = 0,
        NextTrack = 1,
        PlayPause = 2,
        EqualizerPreset = 3,
        KeyActionCount = 4
    };

    struct KeyBinding {
        Qt::Key key = Qt::Key_unknown;
        Qt::KeyboardModifiers modifiers = Qt::NoModifier;
        QString displayName;
    };

    QMap<KeyAction, KeyBinding> getKeyBindings() const;
    void loadKeyBindings(const QMap<KeyAction, KeyBinding> &bindings);

signals:
    void accentColorChanged(const QColor &color);
    void iconSizeChanged(int size);
    void metadataHeightChanged(int height);
    void exitRequested();
    void spectrumGainChanged(float gain);
    void spectrumFpsChanged(int fps);
    void spectrumBandsChanged(int bands);
    void powerModeChanged(bool enabled);
    void projectMPresetSelected(const QString &filePath);
    void maxBitrateChanged(int bitrate);
    void keyBindingChanged(KeyAction action, const KeyBinding &binding);
    void keyBindingsSaved();
    void jamendoReconfigureRequested();
    void offlineModeChanged(bool enabled);

private slots:
    void onSliderChanged(int v);
    void onLineEditChanged();
    void toggleTheme();
    void onColorChanged(int index);
    void onHeightSliderChanged(int v);
    void onIconSizeSliderChanged(int v);
    void onSpectrumGainChanged(int value);
    void showAboutDialog();
    void onIconSizeChanged(int v);
    
    void showKeyBindingTab();
    void showMainSettingsTab();
    void onKeyCaptureButtonClicked(KeyAction action);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    void setupMainSettingsTab();
    void setupKeyBindingTab();
    void onKeyCaptured(Qt::Key key, Qt::KeyboardModifiers modifiers);
    void updateKeyBindingButton(KeyAction action);
    void updateKeyBindingButtons();
    void updateKeyBindingButtonStyle(int index, bool isWaiting);
    QString keyToString(Qt::Key key, Qt::KeyboardModifiers modifiers);
    KeyBinding stringToKeyBinding(const QString &str);
    void createConfigDir();
    void saveKeyBindingsToConfig();
    void loadKeyBindingsFromConfig();

    QSlider *m_bitrateSlider;
    QLineEdit *m_bitrateEdit;
    QSlider *m_heightSlider;
    QSlider *m_iconSizeSlider;
    QPushButton *m_aboutButton;
    QPushButton *m_themeButton;
    QComboBox *m_colorCombo;
    QComboBox *m_languageCombo;
    QPushButton *m_exitButton;
    QSlider *m_spectrumGainSlider;
    QComboBox *m_spectrumFpsCombo;
    QComboBox *m_spectrumBandsCombo;
    QCheckBox *m_powerModeCheck;
    QPushButton *m_projectMPresetButton;
    QLabel *m_projectMPresetPath;
    QPushButton *m_keyBindingButton;
    QPushButton *m_clearJamendoCacheBtn;
    QPushButton *m_jamendoReconfigureBtn;
    QPushButton *m_checkDepsBtn;
    QPushButton *m_updateLocalBtn;
    QPushButton *m_offlineBtn;
    bool m_offlineMode = false;

    void toggleOfflineMode();
    void applyOfflineMode(bool on);
    void updateOfflineButtonStyle();
    static bool loadOfflineModeFromConfig();
    static void saveOfflineModeToConfig(bool on);
    
    QStackedWidget *m_stackedWidget;
    QWidget *m_mainSettingsWidget;
    QWidget *m_keyBindingWidget;
    QPushButton *m_backButton;
    QLabel *m_keyLabels[KeyActionCount];
    QPushButton *m_keyCaptureButtons[KeyActionCount];
    
    QMap<KeyAction, KeyBinding> m_keyBindings;
    bool m_waitingForKey = false;
    KeyAction m_currentKeyAction = PrevTrack;

    bool m_darkTheme;
    QColor m_accentColor;

    RetransList m_retrans;      // live-retranslation registry
    void retranslateUi();

    void applyTheme(bool dark);
    void applyAccentColor();
};

#endif // SETTINGSWIDGET_H