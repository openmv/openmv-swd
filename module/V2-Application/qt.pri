!isEmpty(QT_PRI_INCLUDED):error("qt.pri already included")
QT_PRI_INCLUDED = 1

OPENMVSWD_VERSION = 1.3.0
OPENMVSWD_COMPAT_VERSION = 1.3.0

# enable c++11
CONFIG += c++11

defineTest(minQtVersion) {
    maj = $$1
    min = $$2
    patch = $$3
    isEqual(QT_MAJOR_VERSION, $$maj) {
        isEqual(QT_MINOR_VERSION, $$min) {
            isEqual(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
            greaterThan(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
        }
        greaterThan(QT_MINOR_VERSION, $$min) {
            return(true)
        }
    }
    greaterThan(QT_MAJOR_VERSION, $$maj) {
        return(true)
    }
    return(false)
}

# For use in custom compilers which just copy files
defineReplace(stripSrcDir) {
    return($$relative_path($$absolute_path($$1, $$OUT_PWD), $$_PRO_FILE_PWD_))
}

EXE_SOURCE_TREE = $$PWD
isEmpty(EXE_BUILD_TREE) {
    sub_dir = $$_PRO_FILE_PWD_
    sub_dir ~= s,^$$re_escape($$PWD),,
    EXE_BUILD_TREE = $$clean_path($$OUT_PWD)
    EXE_BUILD_TREE ~= s,$$re_escape($$sub_dir)$,,
}

EXE_APP_PATH = $$EXE_BUILD_TREE/bin
osx {
    EXE_APP_TARGET = "OpenMV SWD"
    EXE_APP_BUNDLE = $$EXE_APP_PATH/$${EXE_APP_TARGET}.app

    # set output path if not set manually
    isEmpty(EXE_OUTPUT_PATH): EXE_OUTPUT_PATH = $$EXE_APP_BUNDLE/Contents

    EXE_DATA_PATH    = $$EXE_OUTPUT_PATH/Resources
    EXE_BIN_PATH     = $$EXE_OUTPUT_PATH/MacOS
    copydata = 1

} else {
    contains(TEMPLATE, vc.*):vcproj = 1
    EXE_APP_TARGET = openmvswd

    # target output path if not set manually
    isEmpty(EXE_OUTPUT_PATH): EXE_OUTPUT_PATH = $$EXE_BUILD_TREE

    EXE_DATA_PATH    = $$EXE_OUTPUT_PATH/share
    EXE_BIN_PATH     = $$EXE_OUTPUT_PATH/bin
    !isEqual(EXE_SOURCE_TREE, $$EXE_OUTPUT_PATH):copydata = 1

    INSTALL_DATA_PATH    = $$QTC_PREFIX/share
    INSTALL_BIN_PATH     = $$QTC_PREFIX/bin
}

INCLUDEPATH += \
    $$EXE_BUILD_TREE/src \ # for <app/app_version.h>

CONFIG += \
    depend_includepath \
    no_include_pwd

DEFINES += QT_CREATOR QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
!macx:DEFINES += QT_USE_FAST_OPERATOR_PLUS QT_USE_FAST_CONCATENATION

unix {
    CONFIG(debug, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
    CONFIG(release, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared

    CONFIG(debug, debug|release):MOC_DIR = $${OUT_PWD}/.moc/debug-shared
    CONFIG(release, debug|release):MOC_DIR = $${OUT_PWD}/.moc/release-shared

    RCC_DIR = $${OUT_PWD}/.rcc
    UI_DIR = $${OUT_PWD}/.uic
}

msvc {
    # Don't warn about sprintf, fopen etc being 'unsafe'
    DEFINES += _CRT_SECURE_NO_WARNINGS
    QMAKE_CXXFLAGS_WARN_ON *= -w44996
    # Speed up startup time when debugging with cdb
    QMAKE_LFLAGS_DEBUG += /INCREMENTAL:NO
}

qt {
    contains(QT, core): QT += concurrent
    contains(QT, gui): QT += widgets
}
