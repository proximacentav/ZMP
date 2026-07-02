/****************************************************************************
** Meta object code from reading C++ file 'fileswidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../fileswidget.h"
#include <QtGui/qtextcursor.h>
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fileswidget.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN11FilesWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto FilesWidget::qt_create_metaobjectdata<qt_meta_tag_ZN11FilesWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "FilesWidget",
        "fileSelected",
        "",
        "path",
        "onDoubleClicked",
        "QModelIndex",
        "index",
        "onMenuButtonClicked",
        "onPathChanged",
        "onSearchTextChanged",
        "text",
        "onBrowseHome",
        "onBrowseRoot",
        "onDownloadFile",
        "url",
        "localPath",
        "onDownloadProgress",
        "bytesReceived",
        "bytesTotal",
        "onDownloadFinished",
        "onDownloadError",
        "QNetworkReply::NetworkError",
        "error",
        "onSslErrors",
        "QList<QSslError>",
        "errors",
        "QNetworkReply*",
        "reply",
        "onCertificateDialogFinished",
        "onConnectSMBFinished",
        "onIPValidationError",
        "bool&",
        "continueConnection",
        "startPing",
        "ip",
        "port",
        "onPingSocketConnected",
        "onPingSocketError",
        "QAbstractSocket::SocketError",
        "socketError",
        "onPingSocketTimeout"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'fileSelected'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Slot 'onDoubleClicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 5, 6 },
        }}),
        // Slot 'onMenuButtonClicked'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPathChanged'
        QtMocHelpers::SlotData<void(const QString &)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Slot 'onSearchTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Slot 'onBrowseHome'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onBrowseRoot'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDownloadFile'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 14 }, { QMetaType::QString, 15 },
        }}),
        // Slot 'onDownloadProgress'
        QtMocHelpers::SlotData<void(qint64, qint64)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 17 }, { QMetaType::LongLong, 18 },
        }}),
        // Slot 'onDownloadFinished'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDownloadError'
        QtMocHelpers::SlotData<void(QNetworkReply::NetworkError)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 21, 22 },
        }}),
        // Slot 'onSslErrors'
        QtMocHelpers::SlotData<void(const QList<QSslError> &, QNetworkReply *)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 24, 25 }, { 0x80000000 | 26, 27 },
        }}),
        // Slot 'onCertificateDialogFinished'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onConnectSMBFinished'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onIPValidationError'
        QtMocHelpers::SlotData<void(bool &)>(30, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 31, 32 },
        }}),
        // Slot 'startPing'
        QtMocHelpers::SlotData<void(const QString &, int)>(33, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 34 }, { QMetaType::Int, 35 },
        }}),
        // Slot 'onPingSocketConnected'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPingSocketError'
        QtMocHelpers::SlotData<void(QAbstractSocket::SocketError)>(37, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 38, 39 },
        }}),
        // Slot 'onPingSocketTimeout'
        QtMocHelpers::SlotData<void()>(40, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<FilesWidget, qt_meta_tag_ZN11FilesWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject FilesWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11FilesWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11FilesWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN11FilesWidgetE_t>.metaTypes,
    nullptr
} };

void FilesWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<FilesWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->fileSelected((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->onDoubleClicked((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 2: _t->onMenuButtonClicked(); break;
        case 3: _t->onPathChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->onSearchTextChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->onBrowseHome(); break;
        case 6: _t->onBrowseRoot(); break;
        case 7: _t->onDownloadFile((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 8: _t->onDownloadProgress((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<qint64>>(_a[2]))); break;
        case 9: _t->onDownloadFinished(); break;
        case 10: _t->onDownloadError((*reinterpret_cast<std::add_pointer_t<QNetworkReply::NetworkError>>(_a[1]))); break;
        case 11: _t->onSslErrors((*reinterpret_cast<std::add_pointer_t<QList<QSslError>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QNetworkReply*>>(_a[2]))); break;
        case 12: _t->onCertificateDialogFinished(); break;
        case 13: _t->onConnectSMBFinished(); break;
        case 14: _t->onIPValidationError((*reinterpret_cast<std::add_pointer_t<bool&>>(_a[1]))); break;
        case 15: _t->startPing((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 16: _t->onPingSocketConnected(); break;
        case 17: _t->onPingSocketError((*reinterpret_cast<std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        case 18: _t->onPingSocketTimeout(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 10:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply::NetworkError >(); break;
            }
            break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<QSslError> >(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 17:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (FilesWidget::*)(const QString & )>(_a, &FilesWidget::fileSelected, 0))
            return;
    }
}

const QMetaObject *FilesWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FilesWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11FilesWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int FilesWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void FilesWidget::fileSelected(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}
QT_WARNING_POP
