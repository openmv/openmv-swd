INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
HEADERS += $$PWD/qtsingleapplication.h $$PWD/qtlocalpeer.h
SOURCES += $$PWD/qtsingleapplication.cpp $$PWD/qtlocalpeer.cpp

QT *= network widgets

gotqtlockedfile = $$find(HEADERS, .*qtlockedfile.h)
isEmpty(gotqtlockedfile):include(../qtlockedfile/qtlockedfile.pri)
