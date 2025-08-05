/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "on_actionNewFile_triggered",
        "",
        "handleSaveFileTriggered",
        "handleSaveFileAsTriggered",
        "on_actionCloseTab_triggered",
        "handleOpenFileTriggered",
        "on_tabWidgetEditor_tabCloseRequested",
        "index",
        "handleTabModificationChanged",
        "modified",
        "handleTabTitleChanged",
        "title",
        "buildProject",
        "runProject",
        "cleanProject",
        "debugProject",
        "showAboutDialog",
        "updateLineNumberAreaWidth",
        "newBlockCount",
        "showModeSelectionDialog",
        "activateMode",
        "ChoiceMode::DevelopmentMode",
        "mode",
        "handleDownloadDialogFinished",
        "updateTabTitle",
        "updateTabTitleOnRename",
        "newTitle",
        "handlePathLineEditReturnPressed",
        "handleListViewDoubleClicked",
        "QModelIndex",
        "saveProjectAs",
        "on_tabWidgetEditor_currentChanged",
        "onTabClosed",
        "obj"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'on_actionNewFile_triggered'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleSaveFileTriggered'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleSaveFileAsTriggered'
        QtMocHelpers::SlotData<bool()>(4, 2, QMC::AccessPrivate, QMetaType::Bool),
        // Slot 'on_actionCloseTab_triggered'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleOpenFileTriggered'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_tabWidgetEditor_tabCloseRequested'
        QtMocHelpers::SlotData<void(int)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Slot 'handleTabModificationChanged'
        QtMocHelpers::SlotData<void(bool)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 10 },
        }}),
        // Slot 'handleTabTitleChanged'
        QtMocHelpers::SlotData<void(const QString &)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
        // Slot 'buildProject'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'runProject'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'cleanProject'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'debugProject'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'showAboutDialog'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateLineNumberAreaWidth'
        QtMocHelpers::SlotData<void(int)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 19 },
        }}),
        // Slot 'showModeSelectionDialog'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'activateMode'
        QtMocHelpers::SlotData<void(ChoiceMode::DevelopmentMode)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 22, 23 },
        }}),
        // Slot 'handleDownloadDialogFinished'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateTabTitle'
        QtMocHelpers::SlotData<void(bool)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 10 },
        }}),
        // Slot 'updateTabTitleOnRename'
        QtMocHelpers::SlotData<void(const QString &)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 27 },
        }}),
        // Slot 'handlePathLineEditReturnPressed'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleListViewDoubleClicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 30, 8 },
        }}),
        // Slot 'saveProjectAs'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_tabWidgetEditor_currentChanged'
        QtMocHelpers::SlotData<void(int)>(32, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Slot 'onTabClosed'
        QtMocHelpers::SlotData<void(QObject *)>(33, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QObjectStar, 34 },
        }}),
        // Slot 'onTabClosed'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate | QMC::MethodCloned, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->on_actionNewFile_triggered(); break;
        case 1: _t->handleSaveFileTriggered(); break;
        case 2: { bool _r = _t->handleSaveFileAsTriggered();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->on_actionCloseTab_triggered(); break;
        case 4: _t->handleOpenFileTriggered(); break;
        case 5: _t->on_tabWidgetEditor_tabCloseRequested((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->handleTabModificationChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->handleTabTitleChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->buildProject(); break;
        case 9: _t->runProject(); break;
        case 10: _t->cleanProject(); break;
        case 11: _t->debugProject(); break;
        case 12: _t->showAboutDialog(); break;
        case 13: _t->updateLineNumberAreaWidth((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 14: _t->showModeSelectionDialog(); break;
        case 15: _t->activateMode((*reinterpret_cast< std::add_pointer_t<ChoiceMode::DevelopmentMode>>(_a[1]))); break;
        case 16: _t->handleDownloadDialogFinished(); break;
        case 17: _t->updateTabTitle((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 18: _t->updateTabTitleOnRename((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 19: _t->handlePathLineEditReturnPressed(); break;
        case 20: _t->handleListViewDoubleClicked((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 21: _t->saveProjectAs(); break;
        case 22: _t->on_tabWidgetEditor_currentChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 23: _t->onTabClosed((*reinterpret_cast< std::add_pointer_t<QObject*>>(_a[1]))); break;
        case 24: _t->onTabClosed(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 25)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 25;
    }
    return _id;
}
QT_WARNING_POP
