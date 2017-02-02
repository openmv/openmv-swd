#include "openmvswd.h"
#include "ui_openmvswd.h"

static bool isWindowsHost()
{
#ifdef Q_OS_WIN
        return true;
#else
        return false;
#endif
}

static bool isMacHost()
{
#ifdef Q_OS_MAC
        return true;
#else
        return false;
#endif
}

static bool isLinuxHost()
{
#ifdef Q_OS_LINUX
        return true;
#else
        return false;
#endif
}

static bool removeRecursively(const QString &filePath)
{
    QFileInfo fileInfo = QFileInfo(filePath);

    if((!fileInfo.exists()) && (!fileInfo.isSymLink()))
    {
        return true;
    }

    QFile::setPermissions(filePath, fileInfo.permissions() | QFile::WriteUser);

    if(fileInfo.isDir())
    {
        if(QDir(filePath).isRoot())
        {
            return false;
        }

        if(QDir(filePath).canonicalPath() == QDir::home().canonicalPath())
        {
            return false;
        }

        foreach(const QString &fileName, QDir(filePath).entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot))
        {
            if(!removeRecursively(filePath + QLatin1Char('/') + fileName))
            {
                return false;
            }
        }

        if(!QDir().rmpath(filePath))
        {
            return false;
        }
    }
    else
    {
        if(!QFile::remove(filePath))
        {
            return false;
        }
    }

    return true;
}

static bool copyRecursively(const QString &srcFilePath, const QString &tgtFilePath)
{
    if(QFileInfo(srcFilePath).isDir())
    {
        if(!QFileInfo::exists(tgtFilePath))
        {
            if(!QDir().mkpath(tgtFilePath))
            {
                return false;
            }
        }

        foreach(const QString &fileName, QDir(srcFilePath).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
        {
            if(!copyRecursively(srcFilePath + QLatin1Char('/') + fileName, tgtFilePath + QLatin1Char('/') + fileName))
            {
                return false;
            }
        }
    }
    else
    {
        if(QFile::exists(tgtFilePath))
        {
            if(!QFile::remove(tgtFilePath))
            {
                return false;
            }
        }

        if(!QFile::copy(srcFilePath, tgtFilePath))
        {
            return false;
        }
    }

    return true;
}

static bool removeRecursivelyWrapper(const QString &path)
{
    QEventLoop loop;
    QFutureWatcher<bool> watcher;
    QObject::connect(&watcher, &QFutureWatcher<bool>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(QtConcurrent::run(removeRecursively, path));
    loop.exec();
    return watcher.result();
}

static bool extractAll(QByteArray *data, const QString &path)
{
    QBuffer buffer(data);
    QZipReader reader(&buffer);
    return reader.extractAll(path);
}

static bool extractAllWrapper(QByteArray *data, const QString &path)
{
    QEventLoop loop;
    QFutureWatcher<bool> watcher;
    QObject::connect(&watcher, &QFutureWatcher<bool>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(QtConcurrent::run(extractAll, data, path));
    loop.exec();
    return watcher.result();
}

QString OpenMVSWD::resourcePath()
{
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String(isMacHost() ? "/../Resources" : "/../share"));
}

QString OpenMVSWD::userResourcePath()
{
    QString path = QFileInfo(m_settings->fileName()).path() + QStringLiteral("/openmvswd");

    if(!QFileInfo::exists(path))
    {
        if(!QDir().mkpath(path))
        {
            return path;
        }
    }

    return path;
}

OpenMVSWD::OpenMVSWD(QWidget *parent) : QDialog(parent), m_ui(new Ui::OpenMVSWD)
{
    QSplashScreen *splashScreen = new QSplashScreen(QPixmap(QStringLiteral(SPLASH_PATH)));

    connect(this, &OpenMVSWD::opened,
            splashScreen, &QSplashScreen::deleteLater);

    splashScreen->show();

    ///////////////////////////////////////////////////////////////////////////

    QApplication::setApplicationName(QStringLiteral("OpenMV SWD"));
    QApplication::setApplicationDisplayName(QStringLiteral("OpenMV SWD"));
    QApplication::setApplicationVersion(QLatin1String(OMV_SWD_VERSION_LONG));
    QApplication::setOrganizationName(QStringLiteral("OpenMV"));
    QApplication::setOrganizationDomain(QStringLiteral("openmv.io"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(ICON_PATH)));

    ///////////////////////////////////////////////////////////////////////////

    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("OpenMV"), QStringLiteral("OpenMVSWD"), this);
    m_ui->setupUi(this);

    connect(m_ui->programJigButton, &QPushButton::clicked, this, &OpenMVSWD::programJig);
    connect(m_ui->programSDCardButton, &QPushButton::clicked, this, &OpenMVSWD::programSDCard);
    connect(m_ui->programButton, &QPushButton::clicked, this, &OpenMVSWD::programOpenMVCams);
    connect(m_ui->aboutButton, &QPushButton::clicked, this, [this] {
        QMessageBox::about(this, tr("About OpenMV SWD"), tr(
        "<p><b>About OpenMV SWD %L1</b></p>"
        "<p>By: Ibrahim Abdelkader & Kwabena W. Agyeman</p>"
        "<p><b>GNU GENERAL PUBLIC LICENSE</b></p>"
        "<p>Copyright (C) %L2 %L3</p>"
        "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the <a href=\"http://github.com/openmv/openmv-swd/raw/master/module/V2-Application/LICENSE.GPL3-EXCEPT\">GNU General Public License</a> for more details.</p>"
        "<p><b>Questions or Comments?</b></p>"
        "<p>Contact us at <a href=\"mailto:openmv@openmv.io\">openmv@openmv.io</a>.</p>"
        ).arg(QLatin1String(OMV_SWD_VERSION_LONG)).arg(QLatin1String(OMV_SWD_YEAR)).arg(QLatin1String(OMV_SWD_AUTHOR)));
    });

    ///////////////////////////////////////////////////////////////////////////

    int major = m_settings->value(QStringLiteral(RESOURCES_MAJOR), 0).toInt();
    int minor = m_settings->value(QStringLiteral(RESOURCES_MINOR), 0).toInt();
    int patch = m_settings->value(QStringLiteral(RESOURCES_PATCH), 0).toInt();

    if((major < OMV_SWD_VERSION_MAJOR)
    || ((major == OMV_SWD_VERSION_MAJOR) && (minor < OMV_SWD_VERSION_MINOR))
    || ((major == OMV_SWD_VERSION_MAJOR) && (minor == OMV_SWD_VERSION_MINOR) && (patch < OMV_SWD_VERSION_RELEASE)))
    {
        m_settings->setValue(QStringLiteral(RESOURCES_MAJOR), 0);
        m_settings->setValue(QStringLiteral(RESOURCES_MINOR), 0);
        m_settings->setValue(QStringLiteral(RESOURCES_PATCH), 0);
        m_settings->sync();

        bool ok = true;

        if(!removeRecursively(userResourcePath()))
        {
            QMessageBox::critical(this,
                QString(),
                tr("Please close any programs that are viewing/editing OpenMV SWD's application data and then restart OpenMV SWD!"));

            QApplication::quit();
            ok = false;
        }
        else
        {
            QStringList list = QStringList() << QStringLiteral("firmware");

            foreach(const QString &dir, list)
            {
                if(!copyRecursively(resourcePath() + QLatin1Char('/') + dir, userResourcePath() + QLatin1Char('/') + dir))
                {
                    QMessageBox::critical(this,
                        QString(),
                        tr("Please close any programs that are viewing/editing OpenMV SWD's application data and then restart OpenMV SWD!"));

                    QApplication::quit();
                    ok = false;
                    break;
                }
            }
        }

        if(ok)
        {
            m_settings->setValue(QStringLiteral(RESOURCES_MAJOR), OMV_SWD_VERSION_MAJOR);
            m_settings->setValue(QStringLiteral(RESOURCES_MINOR), OMV_SWD_VERSION_MINOR);
            m_settings->setValue(QStringLiteral(RESOURCES_PATCH), OMV_SWD_VERSION_RELEASE);
            m_settings->sync();
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    QLoggingCategory::setFilterRules(QStringLiteral("qt.network.ssl.warning=false")); // http://stackoverflow.com/questions/26361145/qsslsocket-error-when-ssl-is-not-used

    connect(this, &OpenMVSWD::opened, this, [this] {

        QNetworkAccessManager *manager = new QNetworkAccessManager(this);

        connect(manager, &QNetworkAccessManager::finished, this, [this] (QNetworkReply *reply) {

            QByteArray data = reply->readAll();

            if((reply->error() == QNetworkReply::NoError) && (!data.isEmpty()))
            {
                QRegularExpressionMatch match = QRegularExpression(QStringLiteral("(\\d+)\\.(\\d+)\\.(\\d+)")).match(QString::fromLatin1(data));

                int major = match.captured(1).toInt();
                int minor = match.captured(2).toInt();
                int patch = match.captured(3).toInt();

                if((OMV_SWD_VERSION_MAJOR < major)
                || ((OMV_SWD_VERSION_MAJOR == major) && (OMV_SWD_VERSION_MINOR < minor))
                || ((OMV_SWD_VERSION_MAJOR == major) && (OMV_SWD_VERSION_MINOR == minor) && (OMV_SWD_VERSION_RELEASE < patch)))
                {
                    QMessageBox box(QMessageBox::Information, tr("Update Available"), tr("A new version of OpenMV SWD (%L1.%L2.%L3) is available for download.").arg(major).arg(minor).arg(patch), QMessageBox::Cancel, this,
                        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                        (isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
                    QPushButton *button = box.addButton(tr("Download"), QMessageBox::AcceptRole);
                    box.setDefaultButton(button);
                    box.setEscapeButton(QMessageBox::Cancel);
                    box.exec();

                    if(box.clickedButton() == button)
                    {
                        QUrl url = QUrl(QStringLiteral("http://github.com/openmv/openmv-swd/releases"));

                        if(!QDesktopServices::openUrl(url))
                        {
                            QMessageBox::critical(this,
                                                  QString(),
                                                  tr("Failed to open: \"%L1\"").arg(url.toString()));
                        }
                    }
                    else
                    {
                        QTimer::singleShot(0, this, &OpenMVSWD::packageUpdate);
                    }
                }
                else
                {
                    QTimer::singleShot(0, this, &OpenMVSWD::packageUpdate);
                }
            }
            else
            {
                QTimer::singleShot(0, this, &OpenMVSWD::packageUpdate);
            }

            reply->deleteLater();
        });

        QNetworkRequest request = QNetworkRequest(QUrl(QStringLiteral("http://upload.openmv.io/openmv-swd-version.txt")));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
        QNetworkReply *reply = manager->get(request);

        if(reply)
        {
            connect(reply, &QNetworkReply::sslErrors, reply, static_cast<void (QNetworkReply::*)(void)>(&QNetworkReply::ignoreSslErrors));
        }
        else
        {
            QTimer::singleShot(0, this, &OpenMVSWD::packageUpdate);
        }
    });

    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////

    setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                   (isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
}

OpenMVSWD::~OpenMVSWD()
{
    delete m_settings;
    delete m_ui;
}

void OpenMVSWD::packageUpdate()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    connect(manager, &QNetworkAccessManager::finished, this, [this] (QNetworkReply *reply) {

        QByteArray data = reply->readAll();

        if((reply->error() == QNetworkReply::NoError) && (!data.isEmpty()))
        {
            QRegularExpressionMatch match = QRegularExpression(QStringLiteral("(\\d+)\\.(\\d+)\\.(\\d+)")).match(QString::fromLatin1(data));

            int new_major = match.captured(1).toInt();
            int new_minor = match.captured(2).toInt();
            int new_patch = match.captured(3).toInt();

            int old_major = m_settings->value(QStringLiteral(RESOURCES_MAJOR)).toInt();
            int old_minor = m_settings->value(QStringLiteral(RESOURCES_MINOR)).toInt();
            int old_patch = m_settings->value(QStringLiteral(RESOURCES_PATCH)).toInt();

            if((old_major < new_major)
            || ((old_major == new_major) && (old_minor < new_minor))
            || ((old_major == new_major) && (old_minor == new_minor) && (old_patch < new_patch)))
            {
                QMessageBox box(QMessageBox::Information, tr("Update Available"), tr("New OpenMV SWD reources are available (e.g. firmware)."), QMessageBox::Cancel, this,
                    Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                    (isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
                QPushButton *button = box.addButton(tr("Install"), QMessageBox::AcceptRole);
                box.setDefaultButton(button);
                box.setEscapeButton(QMessageBox::Cancel);
                box.exec();

                if(box.clickedButton() == button)
                {
                    QProgressDialog *dialog = new QProgressDialog(tr("Installing..."), tr("Cancel"), 0, 0, this,
                        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                        (isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                    dialog->setWindowModality(Qt::ApplicationModal);
                    dialog->setAttribute(Qt::WA_ShowWithoutActivating);
                    dialog->setCancelButton(Q_NULLPTR);

                    QNetworkAccessManager *manager2 = new QNetworkAccessManager(this);

                    connect(manager2, &QNetworkAccessManager::finished, this, [this, new_major, new_minor, new_patch, dialog] (QNetworkReply *reply2) {

                        QByteArray data2 = reply2->readAll();

                        if((reply2->error() == QNetworkReply::NoError) && (!data2.isEmpty()))
                        {
                            m_settings->setValue(QStringLiteral(RESOURCES_MAJOR), 0);
                            m_settings->setValue(QStringLiteral(RESOURCES_MINOR), 0);
                            m_settings->setValue(QStringLiteral(RESOURCES_PATCH), 0);
                            m_settings->sync();

                            bool ok = true;

                            if(!removeRecursivelyWrapper(userResourcePath()))
                            {
                                QMessageBox::critical(this,
                                    QString(),
                                    tr("Please close any programs that are viewing/editing OpenMV SWD's application data and then restart OpenMV SWD!"));

                                QApplication::quit();
                                ok = false;
                            }
                            else
                            {
                                if(!extractAllWrapper(&data2, userResourcePath()))
                                {
                                    QMessageBox::critical(this,
                                        QString(),
                                        tr("Please close any programs that are viewing/editing OpenMV SWD's application data and then restart OpenMV SWD!"));

                                    QApplication::quit();
                                    ok = false;
                                }
                            }

                            if(ok)
                            {
                                m_settings->setValue(QStringLiteral(RESOURCES_MAJOR), new_major);
                                m_settings->setValue(QStringLiteral(RESOURCES_MINOR), new_minor);
                                m_settings->setValue(QStringLiteral(RESOURCES_PATCH), new_patch);
                                m_settings->sync();

                                QMessageBox::information(this,
                                    QString(),
                                    tr("Installation Sucessful! Please restart OpenMV SWD."));

                                QApplication::quit();
                            }
                        }
                        else if(reply2->error() != QNetworkReply::NoError)
                        {
                            QMessageBox::critical(this,
                                tr("Package Update"),
                                tr("Error: %L1!").arg(reply2->errorString()));
                        }
                        else
                        {
                            QMessageBox::critical(this,
                                tr("Package Update"),
                                tr("Cannot open the resources file \"%L1\"!").arg(reply2->request().url().toString()));
                        }

                        reply2->deleteLater();

                        delete dialog;
                    });

                    QNetworkRequest request2 = QNetworkRequest(QUrl(QStringLiteral("http://upload.openmv.io/openmv-swd-resources-%L1.%L2.%L3/openmv-swd-resources-%L1.%L2.%L3.zip").arg(new_major).arg(new_minor).arg(new_patch)));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
                    request2.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
                    QNetworkReply *reply2 = manager2->get(request2);

                    if(reply2)
                    {
                        connect(reply2, &QNetworkReply::sslErrors, reply2, static_cast<void (QNetworkReply::*)(void)>(&QNetworkReply::ignoreSslErrors));
                        dialog->show();
                    }
                    else
                    {
                        QMessageBox::critical(this,
                            tr("Package Update"),
                            tr("Network request failed \"%L1\"!").arg(request2.url().toString()));
                    }
                }
            }
        }

        reply->deleteLater();
    });

    QNetworkRequest request = QNetworkRequest(QUrl(QStringLiteral("http://upload.openmv.io/openmv-swd-resources-version.txt")));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
    QNetworkReply *reply = manager->get(request);

    if(reply)
    {
        connect(reply, &QNetworkReply::sslErrors, reply, static_cast<void (QNetworkReply::*)(void)>(&QNetworkReply::ignoreSslErrors));
    }
}

bool OpenMVSWD::programJig2(bool noMessage)
{
    if(noMessage || QMessageBox::warning(this,
        tr("Program Jig"),
        tr("This will program the jig's firmware. Continue?"),
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
    == QMessageBox::Ok)
    {
        if(QMessageBox::information(this,
            tr("Program Jig"),
            tr("Make sure the jig has power and is connected to the computer."),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
        == QMessageBox::Ok)
        {
            QProgressDialog dialog(tr("Programming..."), tr("Cancel"), 0, 0, this,
                Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                (isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
            dialog.setWindowModality(Qt::ApplicationModal);
            dialog.setAttribute(Qt::WA_ShowWithoutActivating);
            dialog.setCancelButton(Q_NULLPTR);
            dialog.show();

            QProcess process;
            process.setProcessChannelMode(QProcess::MergedChannels);

            QTimer timer;
            connect(&timer, &QTimer::timeout, &process, &QProcess::kill);

            QString command;

            if(isWindowsHost())
            {
                command = QDir::cleanPath(QDir::toNativeSeparators(resourcePath() + QStringLiteral("/proploader-windows/proploader.exe")));
            }
            else if(isMacHost())
            {
                command = QDir::cleanPath(QDir::toNativeSeparators(resourcePath() + QStringLiteral("/proploader-mac/proploader")));
            }
            else if(isLinuxHost())
            {
#ifdef Q_PROCESSOR_X86_64
                command = QDir::cleanPath(QDir::toNativeSeparators(resourcePath() + QStringLiteral("/proploader-linux-x86_64/proploader")));
#else
                command = QDir::cleanPath(QDir::toNativeSeparators(resourcePath() + QStringLiteral("/proploader-linux-x86/proploader")));
#endif
            }

            QEventLoop loop;
            connect(&process, static_cast<void(QProcess::*)(int)>(&QProcess::finished), &loop, &QEventLoop::quit);

            process.start(command, QStringList()
                << QStringLiteral("-s")
                << QStringLiteral("-r")
                << QStringLiteral("-e")
                << QDir::cleanPath(QDir::toNativeSeparators(userResourcePath() + QStringLiteral("/firmware/firmware.bin"))));

            timer.start(60000); // 1 minute
            loop.exec();

            if((process.exitStatus() == QProcess::NormalExit) && (!process.exitCode()))
            {
                QMessageBox::information(this,
                    tr("Program Jig"),
                    tr("Firmware update complete!"));

                return true;
            }
            else
            {
                QMessageBox box(QMessageBox::Critical, tr("Program Jig"), tr("Firmware update failed!"), QMessageBox::Ok, this,
                    Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                    (isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
                box.setDetailedText(QString::fromLatin1(process.readAllStandardOutput()));
                box.setDefaultButton(QMessageBox::Ok);
                box.setEscapeButton(QMessageBox::Cancel);
                box.exec();
            }
        }
    }

    return false;
}

bool OpenMVSWD::programSDCard2(bool noMessage)
{
    if(noMessage || QMessageBox::warning(this,
        tr("Program SD Card"),
        tr("This will program the jig's SD card. Continue?"),
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
    == QMessageBox::Ok)
    {
        if(QMessageBox::information(this,
            tr("Program SD Card"),
            tr("Insert the jig's SD card into the computer."),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
        == QMessageBox::Ok)
        {
            QStringList drives;

            foreach(const QStorageInfo &info, QStorageInfo::mountedVolumes())
            {
                if(info.isValid()
                && info.isReady()
                && (!info.isRoot())
                && (!info.isReadOnly())
                && (QString::fromLatin1(info.fileSystemType()).contains(QStringLiteral("fat"), Qt::CaseInsensitive) || QString::fromLatin1(info.fileSystemType()).contains(QStringLiteral("msdos"), Qt::CaseInsensitive))
                && ((!isMacHost()) || info.rootPath().startsWith(QStringLiteral("/volumes/"), Qt::CaseInsensitive))
                && ((!isLinuxHost()) || info.rootPath().startsWith(QStringLiteral("/media/"), Qt::CaseInsensitive) || info.rootPath().startsWith(QStringLiteral("/mnt/"), Qt::CaseInsensitive) || info.rootPath().startsWith(QStringLiteral("/run/"), Qt::CaseInsensitive)))
                {
                    drives.append(info.rootPath());
                }
            }

            QString drive;

            if(drives.isEmpty())
            {
                QMessageBox::critical(this,
                    tr("Program SD Card"),
                    tr("No SD Cards found!"));
            }
            else if(drives.size() == 1)
            {
                drive = drives.first();
                m_settings->setValue(QStringLiteral(LAST_PROGRAM_SD_CARD), drive);
            }
            else
            {
                int index = drives.indexOf(m_settings->value(QStringLiteral(LAST_PROGRAM_SD_CARD)).toString());

                bool ok;
                QString temp = QInputDialog::getItem(this,
                    tr("Program SD Card"), tr("Please select an SD Card"),
                    drives, (index != -1) ? index : 0, false, &ok,
                    Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                    (isMacHost() ? Qt::WindowType() : Qt::WindowCloseButtonHint));

                if(ok)
                {
                    drive = temp;
                    m_settings->setValue(QStringLiteral(LAST_PROGRAM_SD_CARD), drive);
                }
            }

            if(!drive.isEmpty())
            {
                bool ok = true;
                QStringList list = QDir(userResourcePath() + QStringLiteral("/firmware")).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

                foreach(const QString &dir, list)
                {
                    if(!copyRecursively(userResourcePath() + QStringLiteral("/firmware/") + dir, drive + QLatin1Char('/') + dir))
                    {
                        QMessageBox::critical(this,
                            tr("Program SD Card"),
                            tr("Please close any programs that are viewing/editing the SD card!"));

                        ok = false;
                        break;
                    }
                }

                if(ok)
                {
                    QMessageBox::information(this,
                        tr("Program SD Card"),
                        tr("SD card update complete!"));

                    return true;
                }
            }
        }
    }

    return false;
}

#define PROGRAM_END() \
do { \
    m_ui->programJigButton->setEnabled(true); \
    m_ui->programSDCardButton->setEnabled(true); \
    m_ui->aboutButton->setEnabled(true); \
    m_ui->programButton->setEnabled(true); \
    return; \
} while(0)

#define REPROGRAM_END() \
do { \
    QTimer::singleShot(0, this, [this] {programOpenMVCams();}); \
    return; \
} while(0)

#define CLOSE_PROGRAM_END() \
do { \
    QEventLoop m_loop; \
    connect(&port, &OpenMVSWDSerialPort::closeResult, &m_loop, &QEventLoop::quit); \
    port.close(); \
    m_loop.exec(); \
    m_ui->programJigButton->setEnabled(true); \
    m_ui->programSDCardButton->setEnabled(true); \
    m_ui->aboutButton->setEnabled(true); \
    m_ui->programButton->setEnabled(true); \
    return; \
} while(0)

#define CLOSE_REPROGRAM_END() \
do { \
    QEventLoop m_loop; \
    connect(&port, &OpenMVSWDSerialPort::closeResult, &m_loop, &QEventLoop::quit); \
    port.close(); \
    m_loop.exec(); \
    QTimer::singleShot(0, this, [this] {programOpenMVCams();}); \
    return; \
} while(0)

void OpenMVSWD::programOpenMVCams()
{
    QStringList stringList;

    foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts())
    {
        if(port.hasVendorIdentifier() && (port.vendorIdentifier() == FTDI_VID))
        {
            stringList.append(port.portName());
        }
    }

    if(isMacHost())
    {
        stringList = stringList.filter(QStringLiteral("cu"), Qt::CaseInsensitive);
    }

    QString selectedPort;

    if(stringList.isEmpty())
    {
        QMessageBox::critical(this,
            tr("Program"),
            tr("No serial ports found!"));
    }
    else if(stringList.size() == 1)
    {
        selectedPort = stringList.first();
        m_settings->setValue(QStringLiteral(LAST_SERIAL_PORT), selectedPort);
    }
    else
    {
        int index = stringList.indexOf(m_settings->value(QStringLiteral(LAST_SERIAL_PORT)).toString());

        bool ok;
        QString temp = QInputDialog::getItem(this,
            tr("Program"), tr("Select a serial port"),
            stringList, (index != -1) ? index : 0, false, &ok,
            Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
            (isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

        if(ok)
        {
            selectedPort = temp;
            m_settings->setValue(QStringLiteral(LAST_SERIAL_PORT), selectedPort);
        }
    }

    if(!selectedPort.isEmpty())
    {
        m_ui->programJigButton->setEnabled(false);
        m_ui->programSDCardButton->setEnabled(false);
        m_ui->aboutButton->setEnabled(false);
        m_ui->programButton->setEnabled(false);

        OpenMVSWDSerialPort port;

        int major2 = int();
        int minor2 = int();
        int patch2 = int();

        // Open Port //////////////////////////////////////////////////////////
        {
            QString errorMessage2 = QString();
            QString *errorMessage2Ptr = &errorMessage2;

            QMetaObject::Connection conn = connect(&port, &OpenMVSWDSerialPort::openResult,
                this, [this, errorMessage2Ptr] (const QString &errorMessage) {
                *errorMessage2Ptr = errorMessage;
            });

            QProgressDialog dialog(tr("Connecting..."), tr("Cancel"), 0, 0, this,
               Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
               (isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
            dialog.setWindowModality(Qt::ApplicationModal);
            dialog.setAttribute(Qt::WA_ShowWithoutActivating);

            forever
            {
                QEventLoop loop;

                connect(&port, &OpenMVSWDSerialPort::openResult,
                        &loop, &QEventLoop::quit);

                port.open(selectedPort);

                loop.exec();

                if(errorMessage2.isEmpty() || (isLinuxHost() && errorMessage2.contains(QStringLiteral("Permission Denied"), Qt::CaseInsensitive)))
                {
                    break;
                }

                dialog.show();

                QApplication::processEvents();

                if(dialog.wasCanceled())
                {
                    break;
                }
            }

            dialog.close();

            disconnect(conn);

            if(!errorMessage2.isEmpty())
            {
                QMessageBox::critical(this,
                    tr("Program"),
                    tr("Error: %L1!").arg(errorMessage2));

                if(isLinuxHost() && errorMessage2.contains(QStringLiteral("Permission Denied"), Qt::CaseInsensitive))
                {
                    QString name = QString::fromLatin1(qgetenv("USER"));

                    if(name.isEmpty())
                    {
                        name = QString::fromLatin1(qgetenv("USERNAME"));
                    }

                    QMessageBox::information(this,
                        tr("Program"),
                        tr("Try doing:\n\nsudo adduser %L1 dialout\n\n...in a terminal and then restart your computer.").arg(name));
                }

                PROGRAM_END();
            }
        }

        // Get Version ////////////////////////////////////////////////////////
        {
            int *major2Ptr = &major2;
            int *minor2Ptr = &minor2;
            int *patch2Ptr = &patch2;

            QMetaObject::Connection conn = connect(&port, &OpenMVSWDSerialPort::pingResult,
                this, [this, major2Ptr, minor2Ptr, patch2Ptr] (const QString &message) {
                QRegularExpressionMatch match = QRegularExpression(QStringLiteral("Hello World - v(\\d+)\\.(\\d+)\\.(\\d+)")).match(message);
                *major2Ptr = match.captured(1).toInt();
                *minor2Ptr = match.captured(2).toInt();
                *patch2Ptr = match.captured(3).toInt();
            });

            QEventLoop loop;

            connect(&port, &OpenMVSWDSerialPort::pingResult,
                    &loop, &QEventLoop::quit);

            port.ping();

            loop.exec();

            disconnect(conn);

            if((!major2) && (!minor2) && (!patch2))
            {
                QMessageBox::critical(this,
                    tr("Program"),
                    tr("Timeout error while getting jig firmware version!"));

                if(QMessageBox::question(this,
                    tr("Program"),
                    tr("Try to connect again?"),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)
                == QMessageBox::Yes)
                {
                    CLOSE_REPROGRAM_END();
                }

                CLOSE_PROGRAM_END();
            }
            else if((major2 < 0) || (100 < major2) || (minor2 < 0) || (100 < minor2) || (patch2 < 0) || (100 < patch2))
            {
                CLOSE_REPROGRAM_END();
            }
        }

        // Check Version //////////////////////////////////////////////////////
        {
            QFile file(userResourcePath() + QStringLiteral("/firmware/firmware.txt"));

            if(file.open(QIODevice::ReadOnly))
            {
                QByteArray data = file.readAll();

                if((file.error() == QFile::NoError) && (!data.isEmpty()))
                {
                    file.close();

                    QRegularExpressionMatch match = QRegularExpression(QStringLiteral("(\\d+)\\.(\\d+)\\.(\\d+)")).match(QString::fromLatin1(data));

                    if((major2 < match.captured(1).toInt())
                    || ((major2 == match.captured(1).toInt()) && (minor2 < match.captured(2).toInt()))
                    || ((major2 == match.captured(1).toInt()) && (minor2 == match.captured(2).toInt()) && (patch2 < match.captured(3).toInt())))
                    {
                        QEventLoop loop;

                        connect(&port, &OpenMVSWDSerialPort::closeResult,
                                &loop, &QEventLoop::quit);

                        port.close();

                        loop.exec();

                        if(QMessageBox::information(this,
                            tr("Program"),
                            tr("OpenMV SWD needs to update the jig's SD card."),
                            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
                        != QMessageBox::Ok)
                        {
                            PROGRAM_END();
                        }

                        if(!programSDCard2(true))
                        {
                            PROGRAM_END();
                        }

                        QMessageBox::information(this,
                            tr("Program"),
                            tr("Please re-insert the SD card into the jig before continuing."));

                        if(QMessageBox::information(this,
                            tr("Program"),
                            tr("OpenMV SWD needs to update the jig's firmware."),
                            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
                        != QMessageBox::Ok)
                        {
                            PROGRAM_END();
                        }

                        if(!programJig2(true))
                        {
                            PROGRAM_END();
                        }

                        QMessageBox::information(this,
                            tr("Program"),
                            tr("Please wait for the heart-beat LED to start before continuing."));

                        REPROGRAM_END();
                    }
                }
            }
        }

        for(int i = 0; i < MAX_ROW; i++)
        {
//            // Activate Row ///////////////////////////////////////////////////
//            {
//                bool ok2 = false;
//                bool *ok2Ptr = &ok2;

//                QMetaObject::Connection conn = connect(&port, &OpenMVSWDSerialPort::activateRowResult,
//                    this, [this, ok2Ptr] (const QString &text) {
//                    *ok2Ptr = !text.isEmpty();
//                });

//                QEventLoop loop;

//                connect(&port, &OpenMVSWDSerialPort::activateRowResult,
//                        &loop, &QEventLoop::quit);

//                port.activateRow(i);

//                loop.exec();

//                disconnect(conn);

//                if(!ok2)
//                {
//                    QMessageBox::critical(this,
//                        tr("Program"),
//                        tr("Unable to activate row %L1!").arg(i));

//                    CLOSE_PROGRAM_END();
//                }
//            }

            for(int j = 0; j < MAX_COL; j++)
            {
                // Start Programming //////////////////////////////////////////
                {
                    bool ok2 = false;
                    bool *ok2Ptr = &ok2;

                    QMetaObject::Connection conn = connect(&port, &OpenMVSWDSerialPort::startProgrammingResult,
                        this, [this, ok2Ptr] (bool ok) {
                        *ok2Ptr = ok;
                    });

                    QEventLoop loop;

                    connect(&port, &OpenMVSWDSerialPort::startProgrammingResult,
                            &loop, &QEventLoop::quit);

                    port.startProgramming(j);

                    loop.exec();

                    disconnect(conn);

                    if(!ok2)
                    {
                        QMessageBox::critical(this,
                            tr("Program"),
                            tr("Unable to start programming on swd %L1!").arg((i * MAX_COL) + j + 1));

                        CLOSE_PROGRAM_END();
                    }
                }
            }



//            // Deactivate Row /////////////////////////////////////////////////
//            {
//                bool ok2 = false;
//                bool *ok2Ptr = &ok2;

//                QMetaObject::Connection conn = connect(&port, &OpenMVSWDSerialPort::activateRowResult,
//                    this, [this, ok2Ptr] (const QString &text) {
//                    *ok2Ptr = !text.isEmpty();
//                });

//                QEventLoop loop;

//                connect(&port, &OpenMVSWDSerialPort::activateRowResult,
//                        &loop, &QEventLoop::quit);

//                port.activateRow(i);

//                loop.exec();

//                disconnect(conn);

//                if(!ok2)
//                {
//                    QMessageBox::critical(this,
//                        tr("Program"),
//                        tr("Unable to deactivate row %L1!").arg(i));

//                    CLOSE_PROGRAM_END();
//                }
//            }
        }

        CLOSE_PROGRAM_END();
    }
}
