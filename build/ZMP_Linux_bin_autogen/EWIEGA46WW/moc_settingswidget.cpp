/****************************************************************************
** Meta object code from reading C++ file 'settingswidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../settingswidget.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'settingswidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14SettingsWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto SettingsWidget::qt_create_metaobjectdata<qt_meta_tag_ZN14SettingsWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SettingsWidget",
        "accentColorChanged",
        "",
        "QColor",
        "color",
        "iconSizeChanged",
        "size",
        "metadataHeightChanged",
        "height",
        "exitRequested",
        "spectrumGainChanged",
        "gain",
        "spectrumFpsChanged",
        "fps",
        "spectrumBandsChanged",
        "bands",
        "powerModeChanged",
        "enabled",
        "projectMPresetSelected",
        "filePath",
        "maxBitrateChanged",
        "bitrate",
        "onSliderChanged",
        "v",
        "onLineEditChanged",
        "toggleTheme",
        "onColorChanged",
        "index",
        "onHeightSliderChanged",
        "onIconSizeSliderChanged",
        "onSpectrumGainChanged",
        "value",
        "showAboutDialog",
        "onIconSizeChanged"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'accentColorChanged'
        QtMocHelpers::SignalData<void(const QColor &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'iconSizeChanged'
        QtMocHelpers::SignalData<void(int)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 6 },
        }}),
        // Signal 'metadataHeightChanged'
        QtMocHelpers::SignalData<void(int)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Signal 'exitRequested'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'spectrumGainChanged'
        QtMocHelpers::SignalData<void(float)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Float, 11 },
        }}),
        // Signal 'spectrumFpsChanged'
        QtMocHelpers::SignalData<void(int)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 },
        }}),
        // Signal 'spectrumBandsChanged'
        QtMocHelpers::SignalData<void(int)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 15 },
        }}),
        // Signal 'powerModeChanged'
        QtMocHelpers::SignalData<void(bool)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 17 },
        }}),
        // Signal 'projectMPresetSelected'
        QtMocHelpers::SignalData<void(const QString &)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 19 },
        }}),
        // Signal 'maxBitrateChanged'
        QtMocHelpers::SignalData<void(int)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 21 },
        }}),
        // Slot 'onSliderChanged'
        QtMocHelpers::SlotData<void(int)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 23 },
        }}),
        // Slot 'onLineEditChanged'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'toggleTheme'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onColorChanged'
        QtMocHelpers::SlotData<void(int)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 27 },
        }}),
        // Slot 'onHeightSliderChanged'
        QtMocHelpers::SlotData<void(int)>(28, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 23 },
        }}),
        // Slot 'onIconSizeSliderChanged'
        QtMocHelpers::SlotData<void(int)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 23 },
        }}),
        // Slot 'onSpectrumGainChanged'
        QtMocHelpers::SlotData<void(int)>(30, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 31 },
        }}),
        // Slot 'showAboutDialog'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onIconSizeChanged'
        QtMocHelpers::SlotData<void(int)>(33, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 23 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SettingsWidget, qt_meta_tag_ZN14SettingsWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SettingsWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SettingsWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SettingsWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14SettingsWidgetE_t>.metaTypes,
    nullptr
} };

void SettingsWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SettingsWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->accentColorChanged((*reinterpret_cast<std::add_pointer_t<QColor>>(_a[1]))); break;
        case 1: _t->iconSizeChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->metadataHeightChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->exitRequested(); break;
        case 4: _t->spectrumGainChanged((*reinterpret_cast<std::add_pointer_t<float>>(_a[1]))); break;
        case 5: _t->spectrumFpsChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->spectrumBandsChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->powerModeChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 8: _t->projectMPresetSelected((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->maxBitrateChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->onSliderChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->onLineEditChanged(); break;
        case 12: _t->toggleTheme(); break;
        case 13: _t->onColorChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 14: _t->onHeightSliderChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 15: _t->onIconSizeSliderChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 16: _t->onSpectrumGainChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 17: _t->showAboutDialog(); break;
        case 18: _t->onIconSizeChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SettingsWidget::*)(const QColor & )>(_a, &SettingsWidget::accentColorChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWidget::*)(int )>(_a, &SettingsWidget::iconSizeChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWidget::*)(int )>(_a, &SettingsWidget::metadataHeightChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWidget::*)()>(_a, &SettingsWidget::exitRequested, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWidget::*)(float )>(_a, &SettingsWidget::spectrumGainChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWidget::*)(int )>(_a, &SettingsWidget::spectrumFpsChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWidget::*)(int )>(_a, &SettingsWidget::spectrumBandsChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWidget::*)(bool )>(_a, &SettingsWidget::powerModeChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWidget::*)(const QString & )>(_a, &SettingsWidget::projectMPresetSelected, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWidget::*)(int )>(_a, &SettingsWidget::maxBitrateChanged, 9))
            return;
    }
}

const QMetaObject *SettingsWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SettingsWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SettingsWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SettingsWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void SettingsWidget::accentColorChanged(const QColor & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void SettingsWidget::iconSizeChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void SettingsWidget::metadataHeightChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void SettingsWidget::exitRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SettingsWidget::spectrumGainChanged(float _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void SettingsWidget::spectrumFpsChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void SettingsWidget::spectrumBandsChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void SettingsWidget::powerModeChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void SettingsWidget::projectMPresetSelected(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void SettingsWidget::maxBitrateChanged(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}
QT_WARNING_POP
