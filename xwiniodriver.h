/* Copyright (c) 2022 hors<horsicq@gmail.com>
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

class XWinIODriver : public XIODevice
{
    Q_OBJECT

public:
    XWinIODriver(QObject *pParent=nullptr);
    XWinIODriver(QString sServiceName,qint64 nProcessID,quint64 nAddress,quint64 nSize,QObject *pParent=nullptr);

    virtual bool open(OpenMode mode);
    virtual void close();

    bool loadDriver(QString sFileName,QString sServiceName="XWINIODRIVER");
    bool unloadDriver(QString sServiceName="XWINIODRIVER");

private:
    bool installDriver(SC_HANDLE hSCManager,QString sServiceName,QString sFileName);
    bool removeDriver(SC_HANDLE hSCManager,QString sServiceName);
    bool startDriver(SC_HANDLE hSCManager,QString sServiceName);
    bool stopDriver(SC_HANDLE hSCManager,QString sServiceName);
    HANDLE openDevice(QString sServiceName);

signals:
    void infoMessage(QString sText);
    void errorMessage(QString sText);

private:
    HANDLE g_hDriver;
    QString g_sServiceName;
    qint64 g_nProcessID;
    quint64 g_nAddress;
    quint64 g_nSize;
};

#endif // XWINIODRIVER_H
