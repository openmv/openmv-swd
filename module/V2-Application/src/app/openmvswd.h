#ifndef OPENMVSWD_H
#define OPENMVSWD_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtNetwork>

#include "app_version.h"

#define ICON_PATH ":/openmv-media/icons/openmv-icon/openmv.png"
#define SPLASH_PATH ":/openmv-media/splash/openmv-splash-slate/splash-small.png"

#define LAST_PROGRAM_SD_CARD "LastProgramSDCard"

namespace Ui
{
    class OpenMVSWD;
}

class OpenMVSWD : public QDialog
{
    Q_OBJECT

public:

    explicit OpenMVSWD(QWidget *parent = nullptr);
    ~OpenMVSWD();

public slots:

    void programJig();
    void programSDCard();
    void programOpenMVCams();

private:

    QString userResourcePath();
    QSettings *m_settings;
    Ui::OpenMVSWD *m_ui;
};

#endif // OPENMVSWD_H
