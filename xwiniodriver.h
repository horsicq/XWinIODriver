/* Copyright (c) 2022-2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XWINIODRIVER_H
#define XWINIODRIVER_H

#include "xprocess.h"

#define IOCTL_OPENPROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CLOSEPROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READPROCESSMEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITEPROCESSMEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GETNUMBEROFTHREADS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GETEPROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GETKPCR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GETETHREADS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GETETHREAD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GETKPCRS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80A, METHOD_BUFFERED, FILE_ANY_ACCESS)

class XWinIODriver : public XIODevice {
    Q_OBJECT

    struct PROCESSMEMORY {
        void *pProcessHandle;
        void *pMemoryAddress;
        qint32 nMemorySize;
    };

public:
    XWinIODriver(QObject *pParent = nullptr);
    XWinIODriver(const QString &sServiceName, qint64 nProcessID, quint64 nAddress, quint64 nSize, QObject *pParent = nullptr);

    virtual bool open(OpenMode mode);
    virtual void close();

protected:
    virtual qint64 readData(char *pData, qint64 nMaxSize);
    virtual qint64 writeData(const char *pData, qint64 nMaxSize);

public:
    bool loadDriver(QString sFileName, QString sServiceName = "XWINIODRIVER");
    bool unloadDriver(QString sServiceName = "XWINIODRIVER");

    static HANDLE openDriverDevice(QString sServiceName);
    static void closeDriverDevice(HANDLE hDriverDevice);
    static quint64 getEPROCESSAddress(HANDLE hDriverDevice, qint64 nProcessID);
    static QList<quint64> getKPCRAddresses(HANDLE hDriverDevice, qint64 nProcessID);

    static void *openProcess(HANDLE hDriverDevice, qint64 nProcessID);
    static void closeProcess(HANDLE hDriverDevice, void *hProcess);

    static quint64 read_array(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, char *pData, quint64 nSize);
    static quint8 read_uint8(HANDLE hDriverDevice, void *hProcess, quint64 nAddress);
    static quint16 read_uint16(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, bool bIsBigEndian = false);
    static quint32 read_uint32(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, bool bIsBigEndian = false);
    static quint64 read_uint64(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, bool bIsBigEndian = false);
    static QString read_ansiString(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, quint64 nMaxSize = 256);
    static QString read_unicodeString(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, quint64 nMaxSize = 256);  // TODO endian ??

private:
    bool installDriver(SC_HANDLE hSCManager, QString sServiceName, QString sFileName);
    bool removeDriver(SC_HANDLE hSCManager, QString sServiceName);
    bool startDriver(SC_HANDLE hSCManager, QString sServiceName);
    bool stopDriver(SC_HANDLE hSCManager, QString sServiceName);

signals:
    void infoMessage(QString sText);
    void errorMessage(QString sText);

private:
    const qint64 N_BUFFER_SIZE = 0x1000;
    HANDLE g_hDriver;
    HANDLE g_hProcess;
    QString g_sServiceName;
    qint64 g_nProcessID;
};

#endif  // XWINIODRIVER_H
