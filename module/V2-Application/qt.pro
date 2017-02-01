include(qt.pri)

# version check qt
!minQtVersion(5, 5, 0) {
    message("Cannot build with Qt version $${QT_VERSION}.")
    error("Use at least Qt 5.5.0.")
}

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src share
unix:!macx:!isEmpty(copydata):SUBDIRS += bin

DISTFILES += \
    $$files(dist/installer/ifw/config/config-*) \
    dist/installer/ifw/packages/io.openmv.openmvswd/meta/package.xml.in \
    dist/installer/ifw/packages/io.openmv.openmvswd.application/meta/installscript.qs \
    dist/installer/ifw/packages/io.openmv.openmvswd.application/meta/package.xml.in \
    dist/installer/ifw/packages/io.openmv.openmvswd.application/meta/license.txt \
    $$files(scripts/*.py) \
    $$files(scripts/*.sh)

contains(QT_ARCH, i386): ARCHITECTURE = x86
else: ARCHITECTURE = $$QT_ARCH

macx: PLATFORM = "mac"
else:win32: PLATFORM = "windows"
else:linux-*: PLATFORM = "linux-$${ARCHITECTURE}"
else: PLATFORM = "unknown"

BASENAME = $$(INSTALL_BASENAME)
isEmpty(BASENAME): BASENAME = openmv-swd-$${PLATFORM}$(INSTALL_EDITION)-$${OPENMVSWD_VERSION}$(INSTALL_POSTFIX)

macx:INSTALLER_NAME = "openmv-swd-$${OPENMVSWD_VERSION}"
else:INSTALLER_NAME = "$${BASENAME}"

macx {
    APPBUNDLE = "$$OUT_PWD/bin/OpenMV SWD.app"
    BINDIST_SOURCE = "$$OUT_PWD/bin/OpenMV SWD.app"
    BINDIST_INSTALLER_SOURCE = $$BINDIST_SOURCE
    deployqt.commands = $$PWD/scripts/deployqtHelper_mac.sh \"$${APPBUNDLE}\" \"$$[QT_INSTALL_TRANSLATIONS]\" \"$$[QT_INSTALL_PLUGINS]\" \"$$[QT_INSTALL_IMPORTS]\" \"$$[QT_INSTALL_QML]\"
    codesign.commands = codesign --deep -s \"$(SIGNING_IDENTITY)\" $(SIGNING_FLAGS) \"$${APPBUNDLE}\"
    dmg.commands = $$PWD/scripts/makedmg.sh $$OUT_PWD/bin $${BASENAME}.dmg
    QMAKE_EXTRA_TARGETS += codesign dmg
} else {
    BINDIST_SOURCE = "$(INSTALL_ROOT)$$QTC_PREFIX"
    BINDIST_INSTALLER_SOURCE = "$$BINDIST_SOURCE/*"
    deployqt.commands = python -u $$PWD/scripts/deployqt.py -i \"$(INSTALL_ROOT)$$QTC_PREFIX\" \"$(QMAKE)\"
    deployqt.depends = install
}

INSTALLER_ARCHIVE_FROM_ENV = $$(INSTALLER_ARCHIVE)
isEmpty(INSTALLER_ARCHIVE_FROM_ENV) {
    INSTALLER_ARCHIVE = $$OUT_PWD/$${BASENAME}-installer-archive.7z
} else {
    INSTALLER_ARCHIVE = $$OUT_PWD/$$(INSTALLER_ARCHIVE)
}

bindist.commands = 7z a -mx9 $$OUT_PWD/$${BASENAME}.7z \"$$BINDIST_SOURCE\"
bindist_installer.depends = deployqt
bindist_installer.commands = python -u $$PWD/scripts/sign.py \"$$BINDIST_INSTALLER_SOURCE\" && 7z a -mx9 $${INSTALLER_ARCHIVE} \"$$BINDIST_INSTALLER_SOURCE\"
installer.depends = bindist_installer
macx {
    installer.commands = python -u $$PWD/scripts/packageIfw.py -i \"$(IFW_PATH)\" -v $${OPENMVSWD_VERSION} -a \"$${INSTALLER_ARCHIVE}\" "$$INSTALLER_NAME" && python -u $$PWD/scripts/sign.py \"$${INSTALLER_NAME}.app\"
} else:win32 {
    installer.commands = python -u $$PWD/scripts/packageIfw.py -i \"$(IFW_PATH)\" -v $${OPENMVSWD_VERSION} -a \"$${INSTALLER_ARCHIVE}\" "$$INSTALLER_NAME" && python -u $$PWD/scripts/sign.py \"$${INSTALLER_NAME}.exe\"
} else:linux-*: {
    installer.commands = python -u $$PWD/scripts/packageIfw.py -i \"$(IFW_PATH)\" -v $${OPENMVSWD_VERSION} -a \"$${INSTALLER_ARCHIVE}\" "$$INSTALLER_NAME" && python -u $$PWD/scripts/sign.py \"$${INSTALLER_NAME}.run\"
} else {
    installer.commands = python -u $$PWD/scripts/packageIfw.py -i \"$(IFW_PATH)\" -v $${OPENMVSWD_VERSION} -a \"$${INSTALLER_ARCHIVE}\" "$$INSTALLER_NAME"
}

win32 {
    deployqt.commands ~= s,/,\\\\,g
    bindist.commands ~= s,/,\\\\,g
    bindist_installer.commands ~= s,/,\\\\,g
    installer.commands ~= s,/,\\\\,g
}

QMAKE_EXTRA_TARGETS += deployqt bindist bindist_installer installer
