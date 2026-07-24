#ifndef TRANSLATOR_H
#define TRANSLATOR_H

// ---------------------------------------------------------------------------
//  ZMP localization layer
//
//  Lightweight in-app i18n. The Russian source string is used as the lookup
//  key, so Russian keeps working even if a translation is missing (the key is
//  returned verbatim as the fallback).
//
//  Usage:
//    - ztr("Русский текст")           -> translated to the current language
//    - ztrLabel(reg, "...")           -> creates a QLabel, sets its text now and
//                                        registers it for live re-translation
//    - runRetrans(reg)                -> re-applies every registered string
//                                        (call from a widget's retranslateUi())
//
//  Language is resolved on startup from ~/zmp_playlists/config.json ("language"
//  key), falling back to the system locale. User changes are persisted back to
//  the same config.json without touching the other keys.
// ---------------------------------------------------------------------------

#include <QObject>
#include <QString>
#include <QHash>
#include <QVector>
#include <functional>

class QWidget;
class QLabel;
class QPushButton;
class QLineEdit;

class Translator : public QObject
{
    Q_OBJECT
public:
    enum Language {
        Russian = 0,
        English = 1,
        German  = 2
    };

    static Translator &instance();

    Language language() const { return m_language; }

    // Change the active language, persist it to config.json and notify the UI.
    void setLanguage(Language lang);

    // Set the language WITHOUT persisting (used once at startup).
    void setLanguageInitial(Language lang) { m_language = lang; }

    // Translate a Russian source string to the active language.
    QString t(const QString &ru) const;

    // Resolve the initial language: config.json override, else the system locale.
    void initFromConfigOrLocale();

    static QString codeFor(Language lang);            // "ru" / "en" / "de"
    static Language fromCode(const QString &code);
    static QString nativeName(Language lang);         // "Русский" / "English" / "Deutsch"

signals:
    void languageChanged();

private:
    explicit Translator(QObject *parent = nullptr);
    void buildTables();
    void persist() const;
    static QString configFilePath();

    Language m_language = English;
    QHash<QString, QString> m_en;   // ru -> en
    QHash<QString, QString> m_de;   // ru -> de
};

// --------------------------- translate helpers -----------------------------

inline QString ztr(const QString &ru) { return Translator::instance().t(ru); }
inline QString ztr(const char *ru)    { return Translator::instance().t(QString::fromUtf8(ru)); }

// ----------------------- live-retranslation registry -----------------------
//
// A persistent widget keeps one RetransList and, on Translator::languageChanged,
// calls runRetrans() to re-apply every registered string in the active language.

using RetransList = QVector<std::function<void()>>;

inline void runRetrans(const RetransList &reg)
{
    for (const auto &fn : reg)
        fn();
}

// Factory helpers: build the widget, set its localized text now, and register a
// closure so the text updates live when the language changes.
QLabel      *ztrLabel(RetransList &reg, const char *ru, QWidget *parent = nullptr);
QPushButton *ztrButton(RetransList &reg, const char *ru, QWidget *parent = nullptr);

// Apply-and-register helpers for widgets that already exist.
void ztrSetText(RetransList &reg, QWidget *w, const char *ru);          // "text" property
void ztrSetTitle(RetransList &reg, QWidget *w, const char *ru);         // "windowTitle"
void ztrSetPlaceholder(RetransList &reg, QLineEdit *e, const char *ru); // "placeholderText"
void ztrSetTooltip(RetransList &reg, QWidget *w, const char *ru);       // "toolTip"

// Register an arbitrary closure (and run it once now). Use for state-dependent
// text such as a toggle button that shows different labels per state.
void ztrRegister(RetransList &reg, std::function<void()> fn);

#endif // TRANSLATOR_H
