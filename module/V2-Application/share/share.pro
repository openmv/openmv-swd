TEMPLATE = aux

include(../qt.pri)

STATIC_BASE = $$PWD
STATIC_OUTPUT_BASE = $$EXE_DATA_PATH
STATIC_INSTALL_BASE = $$INSTALL_DATA_PATH

DATA_DIRS = \
    firmware
win32: DATA_DIRS += drivers proploader-windows
macx: DATA_DIRS += proploader-mac
linux-* {
    contains(QMAKE_TARGET.arch, x86_64) {
        DATA_DIRS += proploader-linux-x86_64
    } else {
        DATA_DIRS += proploader-linux-x86
    }
}

for(data_dir, DATA_DIRS) {
    files = $$files($$PWD/$$data_dir/*, true)
    # Info.plist.in are handled below
    for(file, files):!contains(file, ".*/Info\\.plist\\.in$"):!exists($$file/*): \
        STATIC_FILES += $$file
}

include(../qtdata.pri)
