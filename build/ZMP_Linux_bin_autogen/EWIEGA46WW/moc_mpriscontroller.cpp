/****************************************************************************
** Meta object code from reading C++ file 'mpriscontroller.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../mpriscontroller.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mpriscontroller.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16MprisRootAdaptorE_t {};
} // unnamed namespace

template <> constexpr inline auto MprisRootAdaptor::qt_create_metaobjectdata<qt_meta_tag_ZN16MprisRootAdaptorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MprisRootAdaptor",
        "D-Bus Interface",
        "org.mpris.MediaPlayer2",
        "Raise",
        "",
        "Quit",
        "CanQuit",
        "CanRaise",
        "CanSetFullscreen",
        "HasTrackList",
        "Identity",
        "DesktopEntry",
        "SupportedUriSchemes",
        "SupportedMimeTypes"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'Raise'
        QtMocHelpers::SlotData<void()>(3, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'Quit'
        QtMocHelpers::SlotData<void()>(5, 4, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'CanQuit'
        QtMocHelpers::PropertyData<bool>(6, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'CanRaise'
        QtMocHelpers::PropertyData<bool>(7, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'CanSetFullscreen'
        QtMocHelpers::PropertyData<bool>(8, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'HasTrackList'
        QtMocHelpers::PropertyData<bool>(9, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'Identity'
        QtMocHelpers::PropertyData<QString>(10, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'DesktopEntry'
        QtMocHelpers::PropertyData<QString>(11, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'SupportedUriSchemes'
        QtMocHelpers::PropertyData<QStringList>(12, QMetaType::QStringList, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'SupportedMimeTypes'
        QtMocHelpers::PropertyData<QStringList>(13, QMetaType::QStringList, QMC::DefaultPropertyFlags | QMC::Constant),
    };
    QtMocHelpers::UintData qt_enums {
    };
    QtMocHelpers::UintData qt_constructors {};
    QtMocHelpers::ClassInfos qt_classinfo({
            {    1,    2 },
    });
    return QtMocHelpers::metaObjectData<MprisRootAdaptor, qt_meta_tag_ZN16MprisRootAdaptorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums, qt_constructors, qt_classinfo);
}
Q_CONSTINIT const QMetaObject MprisRootAdaptor::staticMetaObject = { {
    QMetaObject::SuperData::link<QDBusAbstractAdaptor::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16MprisRootAdaptorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16MprisRootAdaptorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16MprisRootAdaptorE_t>.metaTypes,
    nullptr
} };

void MprisRootAdaptor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MprisRootAdaptor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->Raise(); break;
        case 1: _t->Quit(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->CanQuit(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->CanRaise(); break;
        case 2: *reinterpret_cast<bool*>(_v) = _t->CanSetFullscreen(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->HasTrackList(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->Identity(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->DesktopEntry(); break;
        case 6: *reinterpret_cast<QStringList*>(_v) = _t->SupportedUriSchemes(); break;
        case 7: *reinterpret_cast<QStringList*>(_v) = _t->SupportedMimeTypes(); break;
        default: break;
        }
    }
}

const QMetaObject *MprisRootAdaptor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MprisRootAdaptor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16MprisRootAdaptorE_t>.strings))
        return static_cast<void*>(this);
    return QDBusAbstractAdaptor::qt_metacast(_clname);
}

int MprisRootAdaptor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDBusAbstractAdaptor::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}
namespace {
struct qt_meta_tag_ZN18MprisPlayerAdaptorE_t {};
} // unnamed namespace

template <> constexpr inline auto MprisPlayerAdaptor::qt_create_metaobjectdata<qt_meta_tag_ZN18MprisPlayerAdaptorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MprisPlayerAdaptor",
        "D-Bus Interface",
        "org.mpris.MediaPlayer2.Player",
        "playbackStatusChanged",
        "",
        "loopStatusChanged",
        "rateChanged",
        "shuffleChanged",
        "metadataChanged",
        "volumeChanged",
        "positionChanged",
        "canGoNextChanged",
        "canGoPreviousChanged",
        "canPlayChanged",
        "canPauseChanged",
        "Next",
        "Previous",
        "Pause",
        "PlayPause",
        "Stop",
        "Play",
        "Seek",
        "offset",
        "SetPosition",
        "QDBusObjectPath",
        "trackId",
        "position",
        "OpenUri",
        "uri",
        "PlaybackStatus",
        "LoopStatus",
        "Rate",
        "Shuffle",
        "Metadata",
        "QVariantMap",
        "Volume",
        "Position",
        "MinimumRate",
        "MaximumRate",
        "CanGoNext",
        "CanGoPrevious",
        "CanPlay",
        "CanPause",
        "CanSeek",
        "CanControl"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'playbackStatusChanged'
        QtMocHelpers::SignalData<void()>(3, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'loopStatusChanged'
        QtMocHelpers::SignalData<void()>(5, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'rateChanged'
        QtMocHelpers::SignalData<void()>(6, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'shuffleChanged'
        QtMocHelpers::SignalData<void()>(7, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'metadataChanged'
        QtMocHelpers::SignalData<void()>(8, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'volumeChanged'
        QtMocHelpers::SignalData<void()>(9, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'positionChanged'
        QtMocHelpers::SignalData<void()>(10, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'canGoNextChanged'
        QtMocHelpers::SignalData<void()>(11, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'canGoPreviousChanged'
        QtMocHelpers::SignalData<void()>(12, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'canPlayChanged'
        QtMocHelpers::SignalData<void()>(13, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'canPauseChanged'
        QtMocHelpers::SignalData<void()>(14, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'Next'
        QtMocHelpers::SlotData<void()>(15, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'Previous'
        QtMocHelpers::SlotData<void()>(16, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'Pause'
        QtMocHelpers::SlotData<void()>(17, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'PlayPause'
        QtMocHelpers::SlotData<void()>(18, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'Stop'
        QtMocHelpers::SlotData<void()>(19, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'Play'
        QtMocHelpers::SlotData<void()>(20, 4, QMC::AccessPublic, QMetaType::Void),
        // Slot 'Seek'
        QtMocHelpers::SlotData<void(qint64)>(21, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::LongLong, 22 },
        }}),
        // Slot 'SetPosition'
        QtMocHelpers::SlotData<void(const QDBusObjectPath &, qint64)>(23, 4, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 24, 25 }, { QMetaType::LongLong, 26 },
        }}),
        // Slot 'OpenUri'
        QtMocHelpers::SlotData<void(const QString &)>(27, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 28 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'PlaybackStatus'
        QtMocHelpers::PropertyData<QString>(29, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'LoopStatus'
        QtMocHelpers::PropertyData<QString>(30, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'Rate'
        QtMocHelpers::PropertyData<double>(31, QMetaType::Double, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 2),
        // property 'Shuffle'
        QtMocHelpers::PropertyData<bool>(32, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'Metadata'
        QtMocHelpers::PropertyData<QVariantMap>(33, 0x80000000 | 34, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 4),
        // property 'Volume'
        QtMocHelpers::PropertyData<double>(35, QMetaType::Double, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'Position'
        QtMocHelpers::PropertyData<qint64>(36, QMetaType::LongLong, QMC::DefaultPropertyFlags, 6),
        // property 'MinimumRate'
        QtMocHelpers::PropertyData<double>(37, QMetaType::Double, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'MaximumRate'
        QtMocHelpers::PropertyData<double>(38, QMetaType::Double, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'CanGoNext'
        QtMocHelpers::PropertyData<bool>(39, QMetaType::Bool, QMC::DefaultPropertyFlags, 7),
        // property 'CanGoPrevious'
        QtMocHelpers::PropertyData<bool>(40, QMetaType::Bool, QMC::DefaultPropertyFlags, 8),
        // property 'CanPlay'
        QtMocHelpers::PropertyData<bool>(41, QMetaType::Bool, QMC::DefaultPropertyFlags, 9),
        // property 'CanPause'
        QtMocHelpers::PropertyData<bool>(42, QMetaType::Bool, QMC::DefaultPropertyFlags, 10),
        // property 'CanSeek'
        QtMocHelpers::PropertyData<bool>(43, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'CanControl'
        QtMocHelpers::PropertyData<bool>(44, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Constant),
    };
    QtMocHelpers::UintData qt_enums {
    };
    QtMocHelpers::UintData qt_constructors {};
    QtMocHelpers::ClassInfos qt_classinfo({
            {    1,    2 },
    });
    return QtMocHelpers::metaObjectData<MprisPlayerAdaptor, qt_meta_tag_ZN18MprisPlayerAdaptorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums, qt_constructors, qt_classinfo);
}
Q_CONSTINIT const QMetaObject MprisPlayerAdaptor::staticMetaObject = { {
    QMetaObject::SuperData::link<QDBusAbstractAdaptor::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MprisPlayerAdaptorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MprisPlayerAdaptorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18MprisPlayerAdaptorE_t>.metaTypes,
    nullptr
} };

void MprisPlayerAdaptor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MprisPlayerAdaptor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->playbackStatusChanged(); break;
        case 1: _t->loopStatusChanged(); break;
        case 2: _t->rateChanged(); break;
        case 3: _t->shuffleChanged(); break;
        case 4: _t->metadataChanged(); break;
        case 5: _t->volumeChanged(); break;
        case 6: _t->positionChanged(); break;
        case 7: _t->canGoNextChanged(); break;
        case 8: _t->canGoPreviousChanged(); break;
        case 9: _t->canPlayChanged(); break;
        case 10: _t->canPauseChanged(); break;
        case 11: _t->Next(); break;
        case 12: _t->Previous(); break;
        case 13: _t->Pause(); break;
        case 14: _t->PlayPause(); break;
        case 15: _t->Stop(); break;
        case 16: _t->Play(); break;
        case 17: _t->Seek((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 18: _t->SetPosition((*reinterpret_cast<std::add_pointer_t<QDBusObjectPath>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<qint64>>(_a[2]))); break;
        case 19: _t->OpenUri((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 18:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QDBusObjectPath >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::playbackStatusChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::loopStatusChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::rateChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::shuffleChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::metadataChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::volumeChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::positionChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::canGoNextChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::canGoPreviousChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::canPlayChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (MprisPlayerAdaptor::*)()>(_a, &MprisPlayerAdaptor::canPauseChanged, 10))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->PlaybackStatus(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->LoopStatus(); break;
        case 2: *reinterpret_cast<double*>(_v) = _t->Rate(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->Shuffle(); break;
        case 4: *reinterpret_cast<QVariantMap*>(_v) = _t->Metadata(); break;
        case 5: *reinterpret_cast<double*>(_v) = _t->Volume(); break;
        case 6: *reinterpret_cast<qint64*>(_v) = _t->Position(); break;
        case 7: *reinterpret_cast<double*>(_v) = _t->MinimumRate(); break;
        case 8: *reinterpret_cast<double*>(_v) = _t->MaximumRate(); break;
        case 9: *reinterpret_cast<bool*>(_v) = _t->CanGoNext(); break;
        case 10: *reinterpret_cast<bool*>(_v) = _t->CanGoPrevious(); break;
        case 11: *reinterpret_cast<bool*>(_v) = _t->CanPlay(); break;
        case 12: *reinterpret_cast<bool*>(_v) = _t->CanPause(); break;
        case 13: *reinterpret_cast<bool*>(_v) = _t->CanSeek(); break;
        case 14: *reinterpret_cast<bool*>(_v) = _t->CanControl(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 1: _t->setLoopStatus(*reinterpret_cast<QString*>(_v)); break;
        case 2: _t->setRate(*reinterpret_cast<double*>(_v)); break;
        case 3: _t->setShuffle(*reinterpret_cast<bool*>(_v)); break;
        case 5: _t->setVolume(*reinterpret_cast<double*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *MprisPlayerAdaptor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MprisPlayerAdaptor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18MprisPlayerAdaptorE_t>.strings))
        return static_cast<void*>(this);
    return QDBusAbstractAdaptor::qt_metacast(_clname);
}

int MprisPlayerAdaptor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDBusAbstractAdaptor::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void MprisPlayerAdaptor::playbackStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MprisPlayerAdaptor::loopStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MprisPlayerAdaptor::rateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MprisPlayerAdaptor::shuffleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void MprisPlayerAdaptor::metadataChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void MprisPlayerAdaptor::volumeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void MprisPlayerAdaptor::positionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void MprisPlayerAdaptor::canGoNextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void MprisPlayerAdaptor::canGoPreviousChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void MprisPlayerAdaptor::canPlayChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void MprisPlayerAdaptor::canPauseChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}
namespace {
struct qt_meta_tag_ZN15MprisControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto MprisController::qt_create_metaobjectdata<qt_meta_tag_ZN15MprisControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MprisController"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MprisController, qt_meta_tag_ZN15MprisControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MprisController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15MprisControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15MprisControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15MprisControllerE_t>.metaTypes,
    nullptr
} };

void MprisController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MprisController *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *MprisController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MprisController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15MprisControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MprisController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
