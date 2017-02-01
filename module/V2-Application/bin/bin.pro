TEMPLATE = app
TARGET = openmvswd.sh

include(../qt.pri)

OBJECTS_DIR =

PRE_TARGETDEPS = $$PWD/openmvswd.sh

QMAKE_LINK = cp $$PWD/openmvswd.sh $@ && : IGNORE REST OF LINE:
QMAKE_STRIP =
CONFIG -= qt separate_debug_info gdb_dwarf_index

QMAKE_CLEAN = openmvswd.sh

target.path  = $$INSTALL_BIN_PATH
INSTALLS    += target

DISTFILES = $$PWD/openmvswd.sh
