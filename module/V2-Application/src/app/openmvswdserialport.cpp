#include "openmvswdserialport.h"

#define BAUD_RATE 115200

#define WRITE_TIMEOUT 3000
#define READ_TIMEOUT 5000

#define PROGRAM_SWD_0 0x53574430
#define PROGRAM_SWD_1 0x53574431
#define PROGRAM_SWD_2 0x53574432
#define PROGRAM_SWD_3 0x53574433
#define PROGRAM_SWD_4 0x53574434
#define ACTIVATE_ROW_0 0x53574435
#define ACTIVATE_ROW_1 0x53574436
#define ACTIVATE_ROW_2 0x53574437
#define ACTIVATE_ROW_3 0x53574438
#define PING 0x53574439

QByteArray command(int c)
{
    return QByteArray().append((c >> 24) & 0xFF).append((c >> 16) & 0xFF).append((c >> 8) & 0xFF).append((c >> 0) & 0xFF);
}

OpenMVSWDSerialPort_private::OpenMVSWDSerialPort_private(QObject *parent) : QObject(parent)
{
    m_port = nullptr;
}

void OpenMVSWDSerialPort_private::open(const QString &portName)
{
    if(m_port)
    {
        delete m_port;
    }

    m_port = new QSerialPort(portName, this);

    if((!m_port->setBaudRate(BAUD_RATE))
    || (!m_port->open(QIODevice::ReadWrite)))
    {
        emit openResult(m_port->errorString());

        delete m_port;
        m_port = Q_NULLPTR;
    }

    if(m_port)
    {
        emit openResult(QString());
    }
}

void OpenMVSWDSerialPort_private::write(const QByteArray &data)
{
    m_port->clearError();

    if((m_port->write(data) != data.size()) || (!m_port->flush()))
    {
        delete m_port;
        m_port = Q_NULLPTR;
    }
    else
    {
        QElapsedTimer elaspedTimer;
        elaspedTimer.start();

        while(m_port->bytesToWrite())
        {
            m_port->waitForBytesWritten(1);

            if(m_port->bytesToWrite() && elaspedTimer.hasExpired(WRITE_TIMEOUT))
            {
                break;
            }
        }

        if(m_port->bytesToWrite())
        {
            delete m_port;
            m_port = Q_NULLPTR;
        }
    }
}

QString OpenMVSWDSerialPort_private::read()
{
    if(!m_port)
    {
        return QString();
    }
    else
    {
        QByteArray response;
        QElapsedTimer elaspedTimer;
        elaspedTimer.start();

        do
        {
            m_port->waitForReadyRead(1);

            for(int i = 0, j = m_port->bytesAvailable(); i < j; i++)
            {
                response.append(m_port->read(1));

                if(response.endsWith('\n'))
                {
                    break;
                }
            }
        }
        while((!response.endsWith('\n')) && (!elaspedTimer.hasExpired(READ_TIMEOUT)));

        if(response.endsWith('\n'))
        {
            return QString::fromUtf8(response);
        }
        else
        {
            delete m_port;
            m_port = Q_NULLPTR;

            return QString();
        }
    }
}

void OpenMVSWDSerialPort_private::ping()
{
    if(m_port)
    {
        write(command(PING));
        emit pingResult(read());
    }
    else
    {
        emit pingResult(QString());
    }
}

void OpenMVSWDSerialPort_private::activateRow(int row)
{
    if(m_port)
    {
        write(command(ACTIVATE_ROW_0 + (row % 4)));
        emit activateRowResult(read());
    }
    else
    {
        emit activateRowResult(QString());
    }
}

void OpenMVSWDSerialPort_private::startProgramming(int col)
{
    if(m_port)
    {
        write(command(PROGRAM_SWD_0 + (col % 5)));

        if(!m_port)
        {
            emit startProgrammingResult(false);
        }
        else
        {
            emit startProgrammingResult(true);
        }
    }
    else
    {
        emit startProgrammingResult(false);
    }
}

void OpenMVSWDSerialPort_private::getLine()
{
    emit getLineResult(read());
}

void OpenMVSWDSerialPort_private::close()
{
    if(m_port)
    {
        delete m_port;
    }

    emit closeResult();
}

OpenMVSWDSerialPort::OpenMVSWDSerialPort(QObject *parent) : QObject(parent)
{
    QThread *thread = new QThread;
    OpenMVSWDSerialPort_private *port = new OpenMVSWDSerialPort_private;
    port->moveToThread(thread);

    connect(this, &OpenMVSWDSerialPort::open,
            port, &OpenMVSWDSerialPort_private::open);

    connect(port, &OpenMVSWDSerialPort_private::openResult,
            this, &OpenMVSWDSerialPort::openResult);

    connect(this, &OpenMVSWDSerialPort::ping,
            port, &OpenMVSWDSerialPort_private::ping);

    connect(port, &OpenMVSWDSerialPort_private::pingResult,
            this, &OpenMVSWDSerialPort::pingResult);

    connect(this, &OpenMVSWDSerialPort::activateRow,
            port, &OpenMVSWDSerialPort_private::activateRow);

    connect(port, &OpenMVSWDSerialPort_private::activateRowResult,
            this, &OpenMVSWDSerialPort::activateRowResult);

    connect(this, &OpenMVSWDSerialPort::startProgramming,
            port, &OpenMVSWDSerialPort_private::startProgramming);

    connect(port, &OpenMVSWDSerialPort_private::startProgrammingResult,
            this, &OpenMVSWDSerialPort::startProgrammingResult);

    connect(this, &OpenMVSWDSerialPort::getLine,
            port, &OpenMVSWDSerialPort_private::getLine);

    connect(port, &OpenMVSWDSerialPort_private::getLineResult,
            this, &OpenMVSWDSerialPort::getLineResult);

    connect(this, &OpenMVSWDSerialPort::close,
            port, &OpenMVSWDSerialPort_private::close);

    connect(port, &OpenMVSWDSerialPort_private::closeResult,
            this, &OpenMVSWDSerialPort::closeResult);

    connect(this, &OpenMVSWDSerialPort::destroyed,
            port, &OpenMVSWDSerialPort_private::deleteLater);

    connect(port, &OpenMVSWDSerialPort_private::destroyed,
            thread, &QThread::quit);

    connect(thread, &QThread::finished,
            thread, &QThread::deleteLater);

    thread->start();
}
