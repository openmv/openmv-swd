#ifndef OPENMVSWD_H
#define OPENMVSWD_H

#include <QtConcurrent>
#include <QtCore>
#include <QtGui>
#include <QtGui/private/qzipreader_p.h>
#include <QtNetwork>
#include <QtSerialPort>
#include <QtWidgets>

#include "app_version.h"
#include "openmvswdserialport.h"

#define ICON_PATH ":/openmv-media/icons/openmv-icon/openmv.png"
#define SPLASH_PATH ":/openmv-media/splash/openmv-splash-slate/splash-small.png"

#define LAST_FORM_KEY "LastFormKey"
#define LAST_PROGRAM_SD_CARD "LastProgramSDCard"
#define LAST_SERIAL_PORT "LastSerialPort"
#define RESOURCES_MAJOR "ResourcesMajor"
#define RESOURCES_MINOR "ResourcesMinor"
#define RESOURCES_PATCH "ResourcesPatch"

#define FTDI_VID 0x0403

namespace Ui
{
    class OpenMVSWD;
}

class OpenMVSWD : public QDialog
{
    Q_OBJECT

public:

    explicit OpenMVSWD(QWidget *parent = 0);
    ~OpenMVSWD();

public slots:

    void packageUpdate();
    void programJig() { programJig2(false); }
    bool programJig2(bool noMessage);
    void programSDCard() { programSDCard2(false); }
    bool programSDCard2(bool noMessage);
    void programOpenMVCams();

signals:

    void opened();

protected:

    void showEvent(QShowEvent *event)
    {
        QTimer::singleShot(0, this, &OpenMVSWD::opened);
        QWidget::showEvent(event);
    }

private:

    QString resourcePath();
    QString userResourcePath();

    QSettings *m_settings;
    Ui::OpenMVSWD *m_ui;
};

#endif // OPENMVSWD_H
