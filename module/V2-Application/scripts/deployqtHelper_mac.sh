#!/bin/bash

############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

[ $# -lt 5 ] && echo "Usage: $(basename $0) <app folder> <qt translations folder> <qt plugin folder> <qt quick imports folder> <qt quick 2 imports folder>" && exit 2
[ $(uname -s) != "Darwin" ] && echo "Run this script on Mac OS X" && exit 2;

echo "Deploying Qt"

# copy qt creator qt.conf
if [ ! -f "$1/Contents/Resources/qt.conf" ]; then
    echo "- Copying qt.conf"
    cp -f "$(dirname "${BASH_SOURCE[0]}")/../dist/installer/mac/qt.conf" "$1/Contents/Resources/qt.conf" || exit 1
fi

# copy Qt translations
# check for known existing translation to avoid copying multiple times
if [ ! -f "$1/Contents/Resources/translations/qt_de.qm" ]; then
    echo "- Copying Qt translations"
    cp "$2"/*.qm "$1/Contents/Resources/translations/" || exit 1
fi

# copy libclang if needed
if [ $LLVM_INSTALL_DIR ]; then
    if [ "$LLVM_INSTALL_DIR"/lib/libclang.dylib -nt "$1/Contents/PlugIns"/libclang.dylib ]; then
        echo "- Copying libclang"
        mkdir -p "$1/Contents/Frameworks" || exit 1
        # use recursive copy to make it copy symlinks as symlinks
        cp -Rf "$LLVM_INSTALL_DIR"/lib/libclang.*dylib "$1/Contents/Frameworks/" || exit 1
        cp -Rf "$LLVM_INSTALL_DIR"/lib/clang "$1/Contents/Resources/cplusplus/" || exit 1
        clangsource="$LLVM_INSTALL_DIR"/bin/clang
        clanglinktarget="$(readlink "$clangsource")"
        cp -Rf "$clangsource" "$1/Contents/Resources/" || exit 1
        if [ $clanglinktarget ]; then
            cp -Rf "$(dirname "$clangsource")/$clanglinktarget" "$1/Contents/Resources/$clanglinktarget" || exit 1
        fi
    fi
    _CLANG_CODEMODEL_LIB="$1/Contents/PlugIns/libClangCodeModel_debug.dylib"
    if [ ! -f "$_CLANG_CODEMODEL_LIB" ]; then
        _CLANG_CODEMODEL_LIB="$1/Contents/PlugIns/libClangCodeModel.dylib"
    fi
    # this will just fail when run a second time on libClangCodeModel
    xcrun install_name_tool -delete_rpath "$LLVM_INSTALL_DIR/lib" "$_CLANG_CODEMODEL_LIB" || true
    xcrun install_name_tool -add_rpath "@loader_path/../Frameworks" "$_CLANG_CODEMODEL_LIB" || true
    clangbackendArgument="-executable=$1/Contents/Resources/clangbackend"
fi

#### macdeployqt

if [ ! -d "$1/Contents/Frameworks/QtCore.framework" ]; then

    echo "- Running macdeployqt ($(which macdeployqt))"

    macdeployqt "$1" || exit 1

fi
