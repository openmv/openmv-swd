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

static QString resourcePath()
{
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String(isMacHost() ? "/../Resources" : "/../share"));
}

QString OpenMVSWD::userResourcePath()
{
    QString path = QFileInfo(m_settings->fileName()).path() + QStringLiteral("openmvswd");

    if(!QDir(path).exists(path))
    {
        QDir().mkpath(path);
    }

    return path;
}

static bool copyRecursively(const QString &srcFilePath, const QString &tgtFilePath)
{
    if (QFileInfo(srcFilePath).isDir())
    {
        QDir targetDir(tgtFilePath);

        if((!targetDir.cdUp())
        || (!targetDir.mkdir(QFileInfo(tgtFilePath).fileName())))
        {
            return false;
        }

        foreach (const QString &fileName, QDir(srcFilePath).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System))
        {
            if (!copyRecursively(srcFilePath + QLatin1Char('/') + fileName, tgtFilePath + QLatin1Char('/') + fileName))
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

OpenMVSWD::OpenMVSWD(QWidget *parent) : QDialog(parent), m_ui(new Ui::OpenMVSWD)
{
    QSplashScreen screen(QPixmap(QStringLiteral(SPLASH_PATH)));
    screen.show();

    QApplication::setApplicationName(QStringLiteral("OpenMV SWD"));
    QApplication::setApplicationDisplayName(QStringLiteral("OpenMV SWD"));
    QApplication::setApplicationVersion(QLatin1String(OMV_SWD_VERSION_LONG));
    QApplication::setOrganizationName(QStringLiteral("OpenMV"));
    QApplication::setOrganizationDomain(QStringLiteral("openmv.io"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(ICON_PATH)));

    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("OpenMV"), QStringLiteral("OpenMVSWD"), this);
    m_ui->setupUi(this);

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

    setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                   (isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
}

OpenMVSWD::~OpenMVSWD()
{
    delete m_ui;
}

void OpenMVSWD::programJig()
{

}

void OpenMVSWD::programSDCard()
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

    }
}

void OpenMVSWD::programOpenMVCams()
{

}
