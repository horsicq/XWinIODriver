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
#include "xwiniodriver.h"

XWinIODriver::XWinIODriver(QObject *pParent) : XIODevice(pParent)
{
    g_hDriver=0;
    g_hProcess=0;
    g_nProcessID=0;
}

XWinIODriver::XWinIODriver(QString sServiceName,qint64 nProcessID,quint64 nAddress,quint64 nSize,QObject *pParent) : XWinIODriver(pParent)
{
    g_sServiceName=sServiceName;
    g_nProcessID=nProcessID;
    setInitOffset(nAddress);
    setSize(nSize);
}

bool XWinIODriver::open(OpenMode mode)
{
    bool bResult=false;

    if(g_nProcessID&&size()) // TODO more checks
    {
        quint32 nFlag=0;

        if(mode==ReadOnly)
        {
            nFlag=GENERIC_READ;
        }
        else if(mode==WriteOnly)
        {
            nFlag=GENERIC_WRITE;
        }
        else if(mode==ReadWrite)
        {
            nFlag=GENERIC_READ|GENERIC_WRITE;
        }

        QString sCompleteDeviceName=QString("\\\\.\\%1").arg(g_sServiceName);

        HANDLE hDevice=CreateFileW((LPCWSTR)(sCompleteDeviceName.utf16()),
                                    nFlag,
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

        bResult=(hDevice!=INVALID_HANDLE_VALUE);

        if(bResult)
        {
            g_hDriver=hDevice;

            g_hProcess=openProcess(g_hDriver,g_nProcessID);

            bResult=(g_hProcess!=nullptr);
        }
    }

    if(bResult)
    {
        setOpenMode(mode);
    }

    return bResult;
}

void XWinIODriver::close()
{
    bool bSuccess=false;

    if(g_hProcess)
    {
        closeProcess(g_hDriver,g_hProcess);
    }

    if(g_hDriver)
    {
        bSuccess=CloseHandle(g_hDriver);
    }

    if(bSuccess)
    {
        setOpenMode(NotOpen);
    }
}

qint64 XWinIODriver::readData(char *pData, qint64 nMaxSize)
{
    qint64 nResult=0;

    qint64 _nPos=pos();

    nMaxSize=qMin(nMaxSize,(qint64)(size()-_nPos));

    for(qint64 i=0;i<nMaxSize;)
    {
        qint64 nDelta=S_ALIGN_UP(_nPos,N_BUFFER_SIZE)-_nPos;

        if(nDelta==0)
        {
            nDelta=N_BUFFER_SIZE;
        }

        nDelta=qMin(nDelta,(qint64)(nMaxSize-i));

        if(nDelta==0)
        {
            break;
        }

        if(read_array(g_hDriver,g_hProcess,getInitOffset()+_nPos,pData,nDelta)!=nDelta)
        {
            break;
        }

        _nPos+=nDelta;
        pData+=nDelta;
        nResult+=nDelta;
        i+=nDelta;
    }

    return nResult;
}

qint64 XWinIODriver::writeData(const char *pData, qint64 nMaxSize)
{
    Q_UNUSED(pData)
    Q_UNUSED(nMaxSize)

    return 0;
}

bool XWinIODriver::loadDriver(QString sFileName, QString sServiceName)
{
    bool bResult=0;

    if(XBinary::isFileExists(sFileName))
    {
        SC_HANDLE hSCManager=OpenSCManagerW(NULL,NULL,SC_MANAGER_ALL_ACCESS);

        if(hSCManager)
        {
            removeDriver(hSCManager,sServiceName);
            installDriver(hSCManager,sServiceName,sFileName);

            if(startDriver(hSCManager,sServiceName))
            {
                bResult=true;
//                hResult=openDevice(sServiceName);
            }

            CloseServiceHandle(hSCManager);
        }
        else
        {
        #ifdef QT_DEBUG
            qDebug("%s",XProcess::getLastErrorAsString().toUtf8().data());
        #endif
            emit errorMessage(QString("XWinIODriver::loadDriver: %1").arg(XProcess::getLastErrorAsString()));
        }
    }
    else
    {
        emit errorMessage(QString("XWinIODriver::loadDriver: %1: %2").arg(tr("Cannot open file"),sFileName));
    }

    return bResult;
}

bool XWinIODriver::unloadDriver(QString sServiceName)
{
    bool bResult=false;

    SC_HANDLE hSCManager=OpenSCManagerW(NULL,NULL,SC_MANAGER_ALL_ACCESS);

    if(hSCManager)
    {
        stopDriver(hSCManager,sServiceName);
        bResult=removeDriver(hSCManager,sServiceName);
        CloseServiceHandle(hSCManager);
    }
    else
    {
    #ifdef QT_DEBUG
        qDebug("%s",XProcess::getLastErrorAsString().toUtf8().data());
    #endif
        emit errorMessage(QString("XWinIODriver::unloadDriver: %1").arg(XProcess::getLastErrorAsString()));
    }

    return bResult;
}

bool XWinIODriver::installDriver(SC_HANDLE hSCManager, QString sServiceName, QString sFileName)
{
    bool bResult=false;

    SC_HANDLE hSCService=CreateServiceW(hSCManager,                         // SCManager database
                                        (LPCWSTR)(sServiceName.utf16()),    // name of service
                                        (LPCWSTR)(sServiceName.utf16()),    // name to display
                                        SERVICE_ALL_ACCESS,                 // desired access
                                        SERVICE_KERNEL_DRIVER,              // service type
                                        SERVICE_DEMAND_START,               // start type
                                        SERVICE_ERROR_NORMAL,               // error control type
                                        (LPCWSTR)(sFileName.utf16()),       // service's binary
                                        NULL,                               // no load ordering group
                                        NULL,                               // no tag identifier
                                        NULL,                               // no dependencies
                                        NULL,                               // LocalSystem account
                                        NULL);                              // no password

    if(hSCService)
    {
        bResult=true;

        CloseServiceHandle(hSCService);
    }
    else
    {
    #ifdef QT_DEBUG
        qDebug("%s",XProcess::getLastErrorAsString().toUtf8().data());
    #endif
        emit errorMessage(QString("XWinIODriver::installDriver: %1").arg(XProcess::getLastErrorAsString()));
    }

    return bResult;
}

bool XWinIODriver::removeDriver(SC_HANDLE hSCManager,QString sServiceName)
{
    bool bResult=false;

    SC_HANDLE hSCService=OpenServiceW(hSCManager,(LPCWSTR)(sServiceName.utf16()),SERVICE_ALL_ACCESS);

    if(hSCService)
    {
        bResult=(DeleteService(hSCService)==TRUE);

        if(!bResult)
        {
        #ifdef QT_DEBUG
            qDebug("%s",XProcess::getLastErrorAsString().toUtf8().data());
        #endif
            emit errorMessage(QString("XWinIODriver::removeDriver: %1").arg(XProcess::getLastErrorAsString()));
        }

        CloseServiceHandle(hSCService);
    }
    else
    {
    #ifdef QT_DEBUG
        qDebug("%s",XProcess::getLastErrorAsString().toUtf8().data());
    #endif
        emit errorMessage(QString("XWinIODriver::removeDriver: %1").arg(XProcess::getLastErrorAsString()));
    }

    return bResult;
}

bool XWinIODriver::startDriver(SC_HANDLE hSCManager,QString sServiceName)
{
    bool bResult=false;

    SC_HANDLE hSCService=OpenService(hSCManager,(LPCWSTR)(sServiceName.utf16()),SERVICE_ALL_ACCESS);

    if(hSCService)
    {
        bResult=(StartServiceW(hSCService,0,NULL)==TRUE);

        if(!bResult)
        {
        #ifdef QT_DEBUG
            qDebug("%s",XProcess::getLastErrorAsString().toUtf8().data());
        #endif
            emit errorMessage(QString("XWinIODriver::startDriver: %1").arg(XProcess::getLastErrorAsString()));

            if(GetLastError()==ERROR_SERVICE_ALREADY_RUNNING)
            {
                bResult=true;
            }
        }

        CloseServiceHandle(hSCService);
    }
    else
    {
    #ifdef QT_DEBUG
        qDebug("%s",XProcess::getLastErrorAsString().toUtf8().data());
    #endif
        emit errorMessage(QString("XWinIODriver::startDriver: %1").arg(XProcess::getLastErrorAsString()));
    }

    return bResult;
}

bool XWinIODriver::stopDriver(SC_HANDLE hSCManager, QString sServiceName)
{
    bool bResult=false;

    SC_HANDLE hSCService=OpenService(hSCManager,(LPCWSTR)(sServiceName.utf16()),SERVICE_ALL_ACCESS);

    if(hSCService)
    {
        for(qint32 i=0;i<5;i++)
        {
            SERVICE_STATUS serviceStatus={0};

            SetLastError(ERROR_SUCCESS);

            if(ControlService(hSCService,SERVICE_CONTROL_STOP,&serviceStatus)==TRUE)
            {
                bResult=true;

                break;
            }

            if(GetLastError()!=ERROR_DEPENDENT_SERVICES_RUNNING)
            {
                break;
            }

            Sleep(1000);
        }

        CloseServiceHandle(hSCService);
    }

    if(!bResult)
    {
    #ifdef QT_DEBUG
        qDebug("%s",XProcess::getLastErrorAsString().toUtf8().data());
    #endif
        emit errorMessage(QString("XWinIODriver::stopDriver: %1").arg(XProcess::getLastErrorAsString()));
    }

    return bResult;
}

HANDLE XWinIODriver::openDriverDevice(QString sServiceName)
{
    HANDLE hResult=0;

    QString sCompleteDeviceName=QString("\\\\.\\%1").arg(sServiceName);

    HANDLE hDevice=CreateFileW((LPCWSTR)(sCompleteDeviceName.utf16()),
                GENERIC_READ|GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

    if(hDevice!=INVALID_HANDLE_VALUE)
    {
        hResult=hDevice;
    }
    else
    {
    #ifdef QT_DEBUG
        qDebug("%s",XProcess::getLastErrorAsString().toUtf8().data());
    #endif
    }

    return hResult;
}

void XWinIODriver::closeDriverDevice(HANDLE hDriverDevice)
{
    CloseHandle(hDriverDevice);
}

quint64 XWinIODriver::getEPROCESSAddress(HANDLE hDriverDevice, qint64 nProcessID)
{
    quint64 nResult=0;

    long long nTemp=0;
    HANDLE pEPROCESS=0;
    HANDLE hProcessID=(HANDLE)nProcessID;

    if(DeviceIoControl(hDriverDevice,IOCTL_GETEPROCESS,&hProcessID,sizeof(HANDLE),&pEPROCESS,sizeof(HANDLE),(LPDWORD)&nTemp,0))
    {
        if(nTemp)
        {
            nResult=(quint64)pEPROCESS;
        }
    }

    return nResult;
}

QList<quint64> XWinIODriver::getKPCRAddresses(HANDLE hDriverDevice, qint64 nProcessID)
{
    QList<quint64> listResult;

    long long nTemp=0;

    char buffer[sizeof(void *)*256];

    DeviceIoControl(hDriverDevice,IOCTL_GETKPCRS,0,0,buffer,sizeof(void *)*256,(LPDWORD)&nTemp,0);

    for(qint32 i=0;i<nTemp/(sizeof(void *));i++)
    {
        quint64 nAddress=(quint64)(*(HANDLE *)&(buffer[sizeof(void *)*i]));

        listResult.append(nAddress);
    }

    return listResult;
}

void *XWinIODriver::openProcess(HANDLE hDriverDevice, qint64 nProcessID)
{
    void *pResult=0;

    long long nTemp=0;
    HANDLE hProcessHandle=0;
    HANDLE hProcessID=(HANDLE)nProcessID;

    if(DeviceIoControl(hDriverDevice,IOCTL_OPENPROCESS,&hProcessID,sizeof(HANDLE),&hProcessHandle,sizeof(HANDLE),(LPDWORD)&nTemp,0))
    {
        if(nTemp)
        {
            pResult=(void *)hProcessHandle;
        }
    }

    return pResult;
}

void XWinIODriver::closeProcess(HANDLE hDriverDevice,void *hProcess)
{
    long long nTemp=0;
    HANDLE hProcessHandle=hProcess;

    DeviceIoControl(hDriverDevice,IOCTL_CLOSEPROCESS,&hProcessHandle,sizeof(HANDLE),0,0,(LPDWORD)&nTemp,0);
}

quint64 XWinIODriver::read_array(HANDLE hDriverDevice,void *hProcess,quint64 nAddress,char *pData,quint64 nSize)
{
    quint64 nResult=0;

    long long nTemp=0;
    PROCESSMEMORY pm={};

    pm.pProcessHandle=hProcess;
    pm.pMemoryAddress=(void *)nAddress;
    pm.nMemorySize=nSize;

    if(DeviceIoControl(hDriverDevice,IOCTL_READPROCESSMEMORY,&pm,sizeof(pm),pData,nSize,(LPDWORD)&nTemp,0))
    {
        nResult=nTemp;
    }

    return nResult;
}

quint8 XWinIODriver::read_uint8(HANDLE hDriverDevice, void *hProcess, quint64 nAddress)
{
    quint8 nResult=0;

    read_array(hDriverDevice,hProcess,nAddress,(char *)&nResult,1);

    return nResult;
}

quint16 XWinIODriver::read_uint16(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, bool bIsBigEndian)
{
    quint16 nResult=0;

    read_array(hDriverDevice,hProcess,nAddress,(char *)&nResult,2);

    if(bIsBigEndian)
    {
        nResult=qFromBigEndian(nResult);
    }
    else
    {
        nResult=qFromLittleEndian(nResult);
    }

    return nResult;
}

quint32 XWinIODriver::read_uint32(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, bool bIsBigEndian)
{
    quint32 nResult=0;

    read_array(hDriverDevice,hProcess,nAddress,(char *)&nResult,4);

    if(bIsBigEndian)
    {
        nResult=qFromBigEndian(nResult);
    }
    else
    {
        nResult=qFromLittleEndian(nResult);
    }

    return nResult;
}

quint64 XWinIODriver::read_uint64(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, bool bIsBigEndian)
{
    quint64 nResult=0;

    read_array(hDriverDevice,hProcess,nAddress,(char *)&nResult,8);

    if(bIsBigEndian)
    {
        nResult=qFromBigEndian(nResult);
    }
    else
    {
        nResult=qFromLittleEndian(nResult);
    }

    return nResult;
}

QString XWinIODriver::read_ansiString(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, quint64 nMaxSize)
{
    char *pBuffer=new char[nMaxSize+1];
    QString sResult;
    quint32 i=0;

    for(;i<nMaxSize;i++)
    {
        if(!read_array(hDriverDevice,hProcess,nAddress+i,&(pBuffer[i]),1))
        {
            break;
        }

        if(pBuffer[i]==0)
        {
            break;
        }
    }

    pBuffer[i]=0;
    sResult.append(pBuffer);

    delete [] pBuffer;

    return sResult;
}

QString XWinIODriver::read_unicodeString(HANDLE hDriverDevice, void *hProcess, quint64 nAddress, quint64 nMaxSize)
{
    QString sResult;

    if(nMaxSize)
    {
        quint16 *pBuffer=new quint16[nMaxSize+1];

        for(qint32 i=0;i<nMaxSize;i++)
        {
            pBuffer[i]=read_uint16(hDriverDevice,hProcess,nAddress+2*i);

            if(pBuffer[i]==0)
            {
                break;
            }

            if(i==nMaxSize-1)
            {
                pBuffer[nMaxSize]=0;
            }
        }

        sResult=QString::fromUtf16(pBuffer);

        delete [] pBuffer;
    }

    return sResult;
}
