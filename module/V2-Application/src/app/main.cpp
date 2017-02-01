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

#include <app/app_version.h>
#include <qtsingleapplication.h>

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

#include "openmvswd.h"

static const char *setHighDpiEnvironmentVariable()
{
    const char* envVarName = 0;
    static const char ENV_VAR_QT_DEVICE_PIXEL_RATIO[] = "QT_DEVICE_PIXEL_RATIO";
#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
    if (Utils::HostOsInfo().isWindowsHost()
            && !qEnvironmentVariableIsSet(ENV_VAR_QT_DEVICE_PIXEL_RATIO)) {
        envVarName = ENV_VAR_QT_DEVICE_PIXEL_RATIO;
        qputenv(envVarName, "auto");
    }
#else
    #if defined(Q_OS_WIN)
        if(!qEnvironmentVariableIsSet(ENV_VAR_QT_DEVICE_PIXEL_RATIO) // legacy in 5.6, but still functional
                && !qEnvironmentVariableIsSet("QT_AUTO_SCREEN_SCALE_FACTOR")
                && !qEnvironmentVariableIsSet("QT_SCALE_FACTOR")
                && !qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS")) {
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        }
    #endif
#endif // < Qt 5.6
    return envVarName;
}

int main(int argc, char **argv)
{
    const char *highDpiEnvironmentVariable = setHighDpiEnvironmentVariable();

#ifdef Q_OS_MAC
    // increase the number of file that can be opened.
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);

    rl.rlim_cur = qMin((rlim_t)OPEN_MAX, rl.rlim_max);
    setrlimit(RLIMIT_NOFILE, &rl);
#endif

    SharedTools::QtSingleApplication app(QLatin1String("OpenMV SWD"), argc, argv);

    if (app.sendMessage(QLatin1String("Hello World!"))) {
        return 0;
    }

    if (highDpiEnvironmentVariable)
        qunsetenv(highDpiEnvironmentVariable);

#if defined(Q_OS_WIN)
    if(!qFuzzyCompare(qApp->devicePixelRatio(), 1.0)
            && QApplication::style()->objectName().startsWith(
                QLatin1String("windows"), Qt::CaseInsensitive)) {
        QApplication::setStyle(QLatin1String("fusion"));
    }
#endif

    const int threadCount = QThreadPool::globalInstance()->maxThreadCount();
    QThreadPool::globalInstance()->setMaxThreadCount(qMax(4, 2 * threadCount));

    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Make sure we honor the system's proxy settings
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    OpenMVSWD w;
    app.setActivationWindow(&w);

    w.show();
    return app.exec();
}
