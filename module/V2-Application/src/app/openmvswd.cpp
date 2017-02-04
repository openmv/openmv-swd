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
        QDir dir = QDir(filePath).canonicalPath();

        if(dir.isRoot())
        {
            return false;
        }

        if(dir.path() == QDir::home().canonicalPath())
        {
            return false;
        }

        foreach(const QString &fileName, dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot))
        {
            if(!removeRecursively(filePath + QLatin1Char('/') + fileName))
            {
                return false;
            }
        }

        if(!QDir::root().rmdir(dir.path()))
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
        QFileInfo tgtFileInfo = QFileInfo(tgtFilePath);

        if(!tgtFileInfo.exists())
        {
            QDir targetDir = QDir(tgtFilePath);

            if(!targetDir.cdUp())
            {
                return false;
            }

            if(!targetDir.mkdir(tgtFileInfo.fileName()))
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

    if(!QFileInfo::exists(path + QLatin1Char('/')))
    {
        if(!QDir().mkpath(path))
        {
            return QFileInfo(m_settings->fileName()).path();
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

    setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                   (isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

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
            QMessageBox::critical(splashScreen,
                QString(),
                tr("Please close any programs that are viewing/editing OpenMV SWD's application data and then restart OpenMV SWD!"));

            QTimer::singleShot(0, this, [this] {QApplication::quit();});
            ok = false;
        }
        else
        {
            QStringList list = QStringList() << QStringLiteral("firmware");

            foreach(const QString &dir, list)
            {
                if(!copyRecursively(resourcePath() + QLatin1Char('/') + dir, userResourcePath() + QLatin1Char('/') + dir))
                {
                    QMessageBox::critical(splashScreen,
                        QString(),
                        tr("Please close any programs that are viewing/editing OpenMV SWD's application data and then restart OpenMV SWD!"));

                    QTimer::singleShot(0, this, [this] {QApplication::quit();});
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
        else
        {
            return;
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

    bool ok;
    QString key = QInputDialog::getText(splashScreen,
        QString(), tr("Please enter a valid form key (provided by OpenMV)"),
        QLineEdit::Normal, m_settings->value(QStringLiteral(LAST_FORM_KEY)).toString(), &ok,
        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
        (isMacHost() ? Qt::WindowType() : Qt::WindowCloseButtonHint));

    if(ok)
    {
        QNetworkAccessManager manager(this);
        QEventLoop loop;

        connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

        QNetworkRequest request = QNetworkRequest(QUrl(QString(QStringLiteral("http://upload.openmv.io/openmv-swd-ids.php?board=NULL&id=NULL&key=%L1")).arg(key)));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
        QNetworkReply *reply = manager.get(request);

        if(reply)
        {
            connect(reply, &QNetworkReply::sslErrors, reply, static_cast<void (QNetworkReply::*)(void)>(&QNetworkReply::ignoreSslErrors));

            loop.exec();

            QByteArray data = reply->readAll();

            if((reply->error() == QNetworkReply::NoError) && (!data.isEmpty()))
            {
                if(QString::fromLatin1(data).contains(QStringLiteral("Done")))
                {
                    m_settings->setValue(QStringLiteral(LAST_FORM_KEY), key);
                }
                else
                {
                    QMessageBox::critical(splashScreen,
                        QString(),
                        tr("Invalid form key!"));

                    QTimer::singleShot(0, this, [this] {QApplication::quit();});
                }
            }
            else if(reply->error() != QNetworkReply::NoError)
            {
                QMessageBox::critical(splashScreen,
                    QString(),
                    tr("Error: %L1!").arg(reply->error()));

                QTimer::singleShot(0, this, [this] {QApplication::quit();});
            }
            else
            {
                QMessageBox::critical(splashScreen,
                    QString(),
                    tr("GET Network error!"));

                QTimer::singleShot(0, this, [this] {QApplication::quit();});
            }

            reply->deleteLater();
        }
        else
        {
            QMessageBox::critical(splashScreen,
                QString(),
                tr("GET network error!"));

            QTimer::singleShot(0, this, [this] {QApplication::quit();});
        }
    }
    else
    {
        QMessageBox::critical(splashScreen,
            QString(),
            tr("A valid form key is required for OpenMV SWD to run."));

        QTimer::singleShot(0, this, [this] {QApplication::quit();});
    }
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

                QApplication::setOverrideCursor(Qt::WaitCursor);
                QApplication::processEvents();

                foreach(const QString &dir, list)
                {
                    if(!removeRecursively(drive + QLatin1Char('/') + dir))
                    {
                        QApplication::restoreOverrideCursor();
                        QApplication::processEvents();

                        QMessageBox::critical(this,
                            tr("Program SD Card"),
                            tr("Please close any programs that are viewing/editing the SD card!"));

                        ok = false;
                        break;
                    }
                }

                if(ok)
                {
                    foreach(const QString &dir, list)
                    {
                        if(!copyRecursively(userResourcePath() + QStringLiteral("/firmware/") + dir, drive + QLatin1Char('/') + dir))
                        {
                            QApplication::restoreOverrideCursor();
                            QApplication::processEvents();

                            QMessageBox::critical(this,
                                tr("Program SD Card"),
                                tr("Please close any programs that are viewing/editing the SD card!"));

                            ok = false;
                            break;
                        }
                    }
                }

                if(ok)
                {
                    QApplication::restoreOverrideCursor();
                    QApplication::processEvents();

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
        m_ui->label_00->setText(tr("SWD 1"));
        m_ui->progressBar_00->setValue(0);
        m_ui->label_01->setText(tr("SWD 2"));
        m_ui->progressBar_01->setValue(0);
        m_ui->label_02->setText(tr("SWD 3"));
        m_ui->progressBar_02->setValue(0);
        m_ui->label_03->setText(tr("SWD 4"));
        m_ui->progressBar_03->setValue(0);
        m_ui->label_04->setText(tr("SWD 5"));
        m_ui->progressBar_04->setValue(0);
        m_ui->label_05->setText(tr("SWD 6"));
        m_ui->progressBar_05->setValue(0);
        m_ui->label_06->setText(tr("SWD 7"));
        m_ui->progressBar_06->setValue(0);
        m_ui->label_07->setText(tr("SWD 8"));
        m_ui->progressBar_07->setValue(0);
        m_ui->label_08->setText(tr("SWD 9"));
        m_ui->progressBar_08->setValue(0);
        m_ui->label_09->setText(tr("SWD 10"));
        m_ui->progressBar_09->setValue(0);
        m_ui->label_10->setText(tr("SWD 11"));
        m_ui->progressBar_10->setValue(0);
        m_ui->label_11->setText(tr("SWD 12"));
        m_ui->progressBar_11->setValue(0);
        m_ui->label_12->setText(tr("SWD 13"));
        m_ui->progressBar_12->setValue(0);
        m_ui->label_13->setText(tr("SWD 14"));
        m_ui->progressBar_13->setValue(0);
        m_ui->label_14->setText(tr("SWD 15"));
        m_ui->progressBar_14->setValue(0);
        m_ui->label_15->setText(tr("SWD 16"));
        m_ui->progressBar_15->setValue(0);
        m_ui->label_16->setText(tr("SWD 17"));
        m_ui->progressBar_16->setValue(0);
        m_ui->label_17->setText(tr("SWD 18"));
        m_ui->progressBar_17->setValue(0);
        m_ui->label_18->setText(tr("SWD 19"));
        m_ui->progressBar_18->setValue(0);
        m_ui->label_19->setText(tr("SWD 20"));
        m_ui->progressBar_19->setValue(0);

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
                    QMessageBox::information(this,
                        tr("Program"),
                        tr("Try doing:\n\nsudo adduser %L1 dialout\n\n...in a terminal and then restart your computer.").arg(isWindowsHost() ? QString::fromLatin1(qgetenv("USERNAME")) : QString::fromLatin1(qgetenv("USER"))));
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

        typedef QPair<QString, QString> board_id_t;
        QList<board_id_t> board_ids;

        for(int i = 0; i < MAX_ROW; i++)
        {
            // Activate Row ///////////////////////////////////////////////////
            {
                bool ok2 = false;
                bool *ok2Ptr = &ok2;

                QMetaObject::Connection conn = connect(&port, &OpenMVSWDSerialPort::activateRowResult,
                    this, [this, ok2Ptr] (const QString &text) {
                    *ok2Ptr = !text.isEmpty();
                });

                QEventLoop loop;

                connect(&port, &OpenMVSWDSerialPort::activateRowResult,
                        &loop, &QEventLoop::quit);

                port.activateRow(i);

                loop.exec();

                disconnect(conn);

                if(!ok2)
                {
                    QMessageBox::critical(this,
                        tr("Program"),
                        tr("Unable to activate row %L1!").arg(i));

                    CLOSE_PROGRAM_END();
                }
            }

            int tries = 3;
            int state[MAX_COL] = {};

            for(int j = 0; j < MAX_COL; j++)
            {
                state[j] = tries;

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

            forever
            {
                QString text2;
                QString *text2Ptr = &text2;

                QMetaObject::Connection conn = connect(&port, &OpenMVSWDSerialPort::getLineResult,
                    this, [this, text2Ptr] (const QString &text) {
                    *text2Ptr = text;
                });

                QEventLoop loop;

                connect(&port, &OpenMVSWDSerialPort::getLineResult,
                        &loop, &QEventLoop::quit);

                port.getLine();

                loop.exec();

                disconnect(conn);

                if(text2.isEmpty())
                {
                    QMessageBox::critical(this,
                        tr("Program"),
                        tr("Jig Timeout!"));

                    CLOSE_PROGRAM_END();
                }

                // Parse Text /////////////////////////////////////////////////
                {
                    QRegularExpressionMatch match = QRegularExpression(QStringLiteral("SWD(\\d+)\\s+\\[(.+?)\\]\\s+(\\d+)%\r\n")).match(text2);
                    int index = match.captured(1).toInt();
                    int swd = (i * MAX_COL) + index;
                    QString text = match.captured(2);
                    int percent = match.captured(3).toInt();

                    if(text.startsWith(QStringLiteral("Not Ready")))
                    {
                        state[index] = 0;
                        text = QStringLiteral("<font color='red'>SWD %L1 Not Ready</font>").arg(swd);
                    }
                    else if(text.startsWith(QStringLiteral("Done")))
                    {
                        state[index] = 0;
                        QRegularExpressionMatch match2 = QRegularExpression(QStringLiteral("Done (.+?):(.+)")).match(text);
                        board_ids.append(board_id_t(match2.captured(1), match2.captured(2)));
                        text = QStringLiteral("<font color='green'>SWD %L1 Done</font>").arg(swd);
                    }
                    else if(text.startsWith(QStringLiteral("Error")))
                    {
                        state[index] -= 1;
                        text = QStringLiteral("<font color='red'>SWD %L1 Error</font>").arg(swd);
                    }
                    else
                    {
                        text.prepend(QStringLiteral("SWD %L1 ").arg(swd));
                    }

                    switch(swd)
                    {
                        case 0:
                        {
                            m_ui->label_00->setText(text);
                            m_ui->progressBar_00->setValue(percent);
                            break;
                        }
                        case 1:
                        {
                            m_ui->label_01->setText(text);
                            m_ui->progressBar_01->setValue(percent);
                            break;
                        }
                        case 2:
                        {
                            m_ui->label_02->setText(text);
                            m_ui->progressBar_02->setValue(percent);
                            break;
                        }
                        case 3: {
                            m_ui->label_03->setText(text);
                            m_ui->progressBar_03->setValue(percent);
                            break;
                        }
                        case 4:
                        {
                            m_ui->label_04->setText(text);
                            m_ui->progressBar_04->setValue(percent);
                            break;
                        }
                        case 5:
                        {
                            m_ui->label_05->setText(text);
                            m_ui->progressBar_05->setValue(percent);
                            break;
                        }
                        case 6: {
                            m_ui->label_06->setText(text);
                            m_ui->progressBar_06->setValue(percent);
                            break;
                        }
                        case 7:
                        {
                            m_ui->label_07->setText(text);
                            m_ui->progressBar_07->setValue(percent);
                            break;
                        }
                        case 8: {
                            m_ui->label_08->setText(text);
                            m_ui->progressBar_08->setValue(percent);
                            break;
                        }
                        case 9:
                        {
                            m_ui->label_09->setText(text);
                            m_ui->progressBar_09->setValue(percent);
                            break;
                        }
                        case 10:
                        {
                            m_ui->label_10->setText(text);
                            m_ui->progressBar_10->setValue(percent);
                            break;
                        }
                        case 11:
                        {
                            m_ui->label_11->setText(text);
                            m_ui->progressBar_11->setValue(percent);
                            break;
                        }
                        case 12:
                        {
                            m_ui->label_12->setText(text);
                            m_ui->progressBar_12->setValue(percent);
                            break;
                        }
                        case 13:
                        {
                            m_ui->label_13->setText(text);
                            m_ui->progressBar_13->setValue(percent);
                            break;
                        }
                        case 14:
                        {
                            m_ui->label_14->setText(text);
                            m_ui->progressBar_14->setValue(percent);
                            break;
                        }
                        case 15:
                        {
                            m_ui->label_15->setText(text);
                            m_ui->progressBar_15->setValue(percent);
                            break;
                        }
                        case 16:
                        {
                            m_ui->label_16->setText(text);
                            m_ui->progressBar_16->setValue(percent);
                            break;
                        }
                        case 17:
                        {
                            m_ui->label_17->setText(text);
                            m_ui->progressBar_17->setValue(percent);
                            break;
                        }
                        case 18:
                        {
                            m_ui->label_18->setText(text);
                            m_ui->progressBar_18->setValue(percent);
                            break;
                        }
                        case 19:
                        {
                            m_ui->label_19->setText(text);
                            m_ui->progressBar_19->setValue(percent);
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }

                bool quit = true;

                for(int j = 0; j < MAX_COL; j++)
                {
                    quit &= state[j] != tries;
                }

                if(quit)
                {
                    QApplication::setOverrideCursor(Qt::WaitCursor);

                    QEventLoop loop;
                    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
                    loop.exec();

                    QApplication::restoreOverrideCursor();

                    bool retry = false;

                    for(int j = 0; j < MAX_COL; j++)
                    {
                        if(state[j])
                        {
                            retry = true;

                            // Start Reprogramming ////////////////////////////
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
                    }

                    if(!retry)
                    {
                        break;
                    }

                    tries -= 1;
                }
            }

            // Deactivate Row /////////////////////////////////////////////////
            {
                bool ok2 = false;
                bool *ok2Ptr = &ok2;

                QMetaObject::Connection conn = connect(&port, &OpenMVSWDSerialPort::activateRowResult,
                    this, [this, ok2Ptr] (const QString &text) {
                    *ok2Ptr = !text.isEmpty();
                });

                QEventLoop loop;

                connect(&port, &OpenMVSWDSerialPort::activateRowResult,
                        &loop, &QEventLoop::quit);

                port.activateRow(i);

                loop.exec();

                disconnect(conn);

                if(!ok2)
                {
                    QMessageBox::critical(this,
                        tr("Program"),
                        tr("Unable to deactivate row %L1!").arg(i));

                    CLOSE_PROGRAM_END();
                }
            }
        }

        // Upload Results /////////////////////////////////////////////////////

        foreach(board_id_t board_id, board_ids)
        {
            QNetworkAccessManager manager(this);
            QEventLoop loop;

            connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

            QNetworkRequest request = QNetworkRequest(QUrl(QString(QStringLiteral("http://upload.openmv.io/openmv-swd-ids.php?board=%L1&id=%L2&key=%L3")).arg(board_id.first).arg(board_id.second).arg(m_settings->value(QStringLiteral(LAST_FORM_KEY)).toString())));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
            request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
            QNetworkReply *reply = manager.get(request);

            if(reply)
            {
                connect(reply, &QNetworkReply::sslErrors, reply, static_cast<void (QNetworkReply::*)(void)>(&QNetworkReply::ignoreSslErrors));

                loop.exec();

                QByteArray data = reply->readAll();

                QTimer::singleShot(0, reply, &QNetworkReply::deleteLater);

                if((reply->error() == QNetworkReply::NoError) && (!data.isEmpty()))
                {
                    if(!QString::fromLatin1(data).contains(QStringLiteral("Done")))
                    {
                        QMessageBox::critical(this,
                            tr("Program"),
                            tr("Data Base Error!\n\nPlease re-program!"));

                        CLOSE_PROGRAM_END();
                    }
                }
                else if(reply->error() != QNetworkReply::NoError)
                {
                    QMessageBox::critical(this,
                        tr("Program"),
                        tr("Error: %L1!\n\nPlease re-program!").arg(reply->error()));

                    CLOSE_PROGRAM_END();
                }
                else
                {
                    QMessageBox::critical(this,
                        tr("Program"),
                        tr("GET Network error!\n\nPlease re-program!"));

                    CLOSE_PROGRAM_END();
                }
            }
            else
            {
                QMessageBox::critical(this,
                    tr("Program"),
                    tr("GET network error!\n\nPlease re-program!"));

                CLOSE_PROGRAM_END();
            }
        }

        QMessageBox::information(this,
            tr("Program"),
            tr("Firmware programming complete!"));

        CLOSE_PROGRAM_END();
    }
}
