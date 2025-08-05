/****************************************************************************
** Meta object code from reading C++ file 'downloadprogressdialog.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/downloadprogressdialog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'downloadprogressdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.0. It"
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
struct qt_meta_tag_ZN22DownloadProgressDialogE_t {};
} // unnamed namespace

template <> constexpr inline auto DownloadProgressDialog::qt_create_metaobjectdata<qt_meta_tag_ZN22DownloadProgressDialogE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DownloadProgressDialog",
        "processCompleted",
        "",
        "processAborted",
        "handleSingleRepoDownloadFinished",
        "exitCode",
        "QProcess::ExitStatus",
        "exitStatus",
        "handleCancelButtonClicked",
        "handleProcessStarted",
        "handleProcessFinished",
        "handleProcessReadyReadStandardOutput",
        "handleProcessReadyReadStandardError",
        "handleProcessErrorOccurred",
        "QProcess::ProcessError",
        "error",
        "handleXcodeToolsCheckFinished",
        "handleBrewCheckFinished",
        "handleBrewInstallFinished",
        "handleToolCheckFinished",
        "handleToolInstallFinished",
        "ProcessStage",
        "InitialStage",
        "XcodeToolsCheckStage",
        "BrewCheckStage",
        "BrewInstallStage",
        "ToolCheckStage",
        "ToolInstallStage",
        "DownloadStage"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'processCompleted'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'processAborted'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'handleSingleRepoDownloadFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { 0x80000000 | 6, 7 },
        }}),
        // Slot 'handleCancelButtonClicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleProcessStarted'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleProcessFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(10, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { 0x80000000 | 6, 7 },
        }}),
        // Slot 'handleProcessReadyReadStandardOutput'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleProcessReadyReadStandardError'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleProcessErrorOccurred'
        QtMocHelpers::SlotData<void(QProcess::ProcessError)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
        // Slot 'handleXcodeToolsCheckFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { 0x80000000 | 6, 7 },
        }}),
        // Slot 'handleBrewCheckFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { 0x80000000 | 6, 7 },
        }}),
        // Slot 'handleBrewInstallFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { 0x80000000 | 6, 7 },
        }}),
        // Slot 'handleToolCheckFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { 0x80000000 | 6, 7 },
        }}),
        // Slot 'handleToolInstallFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 }, { 0x80000000 | 6, 7 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'ProcessStage'
        QtMocHelpers::EnumData<ProcessStage>(21, 21, QMC::EnumFlags{}).add({
            {   22, ProcessStage::InitialStage },
            {   23, ProcessStage::XcodeToolsCheckStage },
            {   24, ProcessStage::BrewCheckStage },
            {   25, ProcessStage::BrewInstallStage },
            {   26, ProcessStage::ToolCheckStage },
            {   27, ProcessStage::ToolInstallStage },
            {   28, ProcessStage::DownloadStage },
        }),
    };
    return QtMocHelpers::metaObjectData<DownloadProgressDialog, qt_meta_tag_ZN22DownloadProgressDialogE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DownloadProgressDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22DownloadProgressDialogE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22DownloadProgressDialogE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN22DownloadProgressDialogE_t>.metaTypes,
    nullptr
} };

void DownloadProgressDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DownloadProgressDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->processCompleted(); break;
        case 1: _t->processAborted(); break;
        case 2: _t->handleSingleRepoDownloadFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 3: _t->handleCancelButtonClicked(); break;
        case 4: _t->handleProcessStarted(); break;
        case 5: _t->handleProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 6: _t->handleProcessReadyReadStandardOutput(); break;
        case 7: _t->handleProcessReadyReadStandardError(); break;
        case 8: _t->handleProcessErrorOccurred((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 9: _t->handleXcodeToolsCheckFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 10: _t->handleBrewCheckFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 11: _t->handleBrewInstallFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 12: _t->handleToolCheckFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 13: _t->handleToolInstallFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DownloadProgressDialog::*)()>(_a, &DownloadProgressDialog::processCompleted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DownloadProgressDialog::*)()>(_a, &DownloadProgressDialog::processAborted, 1))
            return;
    }
}

const QMetaObject *DownloadProgressDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DownloadProgressDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN22DownloadProgressDialogE_t>.strings))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int DownloadProgressDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void DownloadProgressDialog::processCompleted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void DownloadProgressDialog::processAborted()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
