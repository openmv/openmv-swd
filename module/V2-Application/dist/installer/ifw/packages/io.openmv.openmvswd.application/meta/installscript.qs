/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

// constructor
function Component()
{
    installer.installationFinished.connect(this, Component.prototype.installationFinishedPageIsShown);
    installer.finishButtonClicked.connect(this, Component.prototype.installationFinished);
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
}

Component.prototype.createOperationsForArchive = function(archive)
{
    // if there are additional plugin 7zips, these must be extracted in .app/Contents on OS X
    if (systemInfo.productType !== "osx")
        component.addOperation("Extract", archive, "@TargetDir@");
    else
        component.addOperation("Extract", archive, "@TargetDir@/OpenMV SWD.app/Contents");
}

Component.prototype.beginInstallation = function()
{
    component.openMVSWDBinaryPath = installer.value("TargetDir");

    if (installer.value("os") == "win") {
        component.openMVSWDBinaryPath = component.openMVSWDBinaryPath + "\\bin\\openmvswd.exe";
        component.openMVSWDBinaryPath = component.openMVSWDBinaryPath.replace(/\//g, "\\");
    }
    else if (installer.value("os") == "x11")
        component.openMVSWDBinaryPath = component.openMVSWDBinaryPath + "/bin/openmvswd";
    else if (installer.value("os") == "mac")
        component.openMVSWDBinaryPath = component.openMVSWDBinaryPath + "/OpenMV SWD.app/Contents/MacOS/OpenMV SWD";

    if ( installer.value("os") === "win" )
        component.setStopProcessForUpdateRequest(component.openMVSWDBinaryPath, true);
}

Component.prototype.createOperations = function()
{
    // Call the base createOperations and afterwards set some registry settings
    component.createOperations();
    if ( installer.value("os") == "win" )
    {
        component.addOperation( "CreateShortcut",
                                component.openMVSWDBinaryPath,
                                "@StartMenuDir@/OpenMV SWD.lnk",
                                "workingDirectory=@homeDir@" );
        component.addOperation( "CreateShortcut",
                                "@TargetDir@/OpenMVSWDUninst.exe",
                                "@StartMenuDir@/Uninstall.lnk",
                                "workingDirectory=@homeDir@" );
        component.addElevatedOperation("Execute", "{2,512}", "cmd", "/c", "@TargetDir@\\share\\drivers\\ftdi.cmd");
    }
    if ( installer.value("os") == "x11" )
    {
        component.addOperation( "InstallIcons", "@TargetDir@/share/icons" );
        component.addOperation( "CreateDesktopEntry",
                                "OpenMV-openmvswd.desktop",
                                "Type=Application\nExec=" + component.openMVSWDBinaryPath + "\nPath=@TargetDir@\nName=OpenMV SWD\nGenericName=The tool of choice for OpenMV Cam provisioning.\nIcon=OpenMV-openmvswd\nTerminal=false;"
                                );
    }
}

function isRoot()
{
    if (installer.value("os") == "x11" || installer.value("os") == "mac")
    {
        var id = installer.execute("/usr/bin/id", new Array("-u"))[0];
        id = id.replace(/(\r\n|\n|\r)/gm,"");
        if (id === "0")
        {
            return true;
        }
    }
    return false;
}

Component.prototype.installationFinishedPageIsShown = function()
{
    isroot = isRoot();
    try {
        if (component.installed && installer.isInstaller() && installer.status == QInstaller.Success && !isroot) {
            installer.addWizardPageItem( component, "LaunchOpenMVSWDCheckBoxForm", QInstaller.InstallationFinished );
        }
    } catch(e) {
        print(e);
    }
}

Component.prototype.installationFinished = function()
{
    try {
        if (component.installed && installer.isInstaller() && installer.status == QInstaller.Success && !isroot) {
            var isLaunchOpenMVSWDCheckBoxChecked = component.userInterface("LaunchOpenMVSWDCheckBoxForm").launchOpenMVSWDCheckBox.checked;
            if (isLaunchOpenMVSWDCheckBoxChecked)
                installer.executeDetached(component.openMVSWDBinaryPath, new Array(), "@homeDir@");
        }
    } catch(e) {
        print(e);
    }
}
