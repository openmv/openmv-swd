#ifndef OPENMVSWDSERIALPORT_H
#define OPENMVSWDSERIALPORT_H

#include <QtCore>
#include <QtSerialPort>

#define MAX_ROW 4
#define MAX_COL 5

class OpenMVSWDSerialPort_private : public QObject
{
    Q_OBJECT

public:

    explicit OpenMVSWDSerialPort_private(QObject *parent = 0);

public slots:

    void open(const QString &portName);
    void ping();
    void activateRow(int row);
    void startProgramming(int col);
    void getLine();
    void close();

signals:

    void openResult(const QString &errorMessage);
    void pingResult(const QString &message);
    void activateRowResult(const QString &text);
    void startProgrammingResult(bool ok);
    void getLineResult(const QString &text);
    void closeResult();

private:

    void write(const QByteArray &data);
    QString read();

    QSerialPort *m_port;
};

class OpenMVSWDSerialPort : public QObject
{
    Q_OBJECT

public:

    explicit OpenMVSWDSerialPort(QObject *parent = 0);

signals:

    void open(const QString &portName);
    void ping();
    void activateRow(int row);
    void startProgramming(int col);
    void getLine();
    void close();

    void openResult(const QString &errorMessage);
    void pingResult(const QString &message);
    void activateRowResult(const QString &text);
    void startProgrammingResult(bool ok);
    void getLineResult(const QString &text);
    void closeResult();
};

#endif // OPENMVSWDSERIALPORT_H
