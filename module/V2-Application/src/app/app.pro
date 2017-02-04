include(../../qt.pri)
include(../shared/qtsingleapplication/qtsingleapplication.pri)

TEMPLATE = app
CONFIG += qtc_runnable
TARGET = $$EXE_APP_TARGET
DESTDIR = $$EXE_APP_PATH
VERSION = $$OPENMVSWD_VERSION
QT -= testlib
QT += concurrent gui-private serialport

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
SOURCES += \
    main.cpp \
    openmvswd.cpp \
    openmvswdserialport.cpp
HEADERS += \
    openmvswd.h \
    openmvswdserialport.h
FORMS += \
    openmvswd.ui
RESOURCES += \
    openmvswd.qrc

include(../rpath.pri)

win32 {
    RC_FILE = openmvswd.rc
    target.path = $$INSTALL_BIN_PATH
    INSTALLS += target
} else:macx {
    LIBS += -framework CoreFoundation
    ICON = ../../../../openmv-media/icons/openmv-icon/openmv.icns
    QMAKE_INFO_PLIST = Info.plist
} else {
    target.path  = $$INSTALL_BIN_PATH
    INSTALLS    += target
}

DISTFILES += openmvswd.rc \
    Info.plist \
    $$PWD/app_version.h.in

QMAKE_SUBSTITUTES += $$PWD/app_version.h.in

CONFIG += no_batch
