#include "SerialPort.h"
#include <winerror.h>
#include <QElapsedTimer>
#include <QSettings>
#include <windows.h>
#include <QThread>

//DECLARE_LOG_SRC("SerialPort", LOGCAT_Common);


using namespace std;

#define OPEN_TIMEOUT_MS		5000

void EXERR(const QString& sCode, const char* pszFormat, ...)
{
	// Process the string formatting
	va_list args;
	va_start(args, pszFormat);
	char szMsg[2048];
#ifdef Q_OS_WIN
	_vsnprintf(szMsg, sizeof(szMsg) / sizeof(szMsg[0]), pszFormat, args);
#else
	vsnprintf(szMsg, sizeof(szMsg) / sizeof(szMsg[0]), pszFormat, args);
#endif
	va_end(args);
	szMsg[sizeof(szMsg) - 1] = 0; // null terminate to be safe


	Q_UNUSED(sCode);
	throw QString(szMsg);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SerialPort::SerialPort(QObject *parent)
	: QIODevice(parent)
{
	m_hFile = 0;
	m_iCommPort = 1;
}


SerialPort::~SerialPort()
{
	close();
}

int SerialPort::CommPort()
{
	return m_iCommPort;
}

void SerialPort::SetCommPort(int iPort)
{
	m_iCommPort = iPort;
}


void SerialPort::Open(int iPort, int iBaud)
{
	SetCommPort(iPort);

	// Validate the baud rate
	switch (iBaud)
	{
	case CBR_110:
	case CBR_300:
	case CBR_600:
	case CBR_1200:
	case CBR_2400:
	case CBR_4800:
	case CBR_9600:
	case CBR_14400:
	case CBR_19200:
	case CBR_38400:
	case CBR_56000:
	case CBR_57600:
	case CBR_115200:
	case CBR_128000:
	case CBR_256000:
		break;
	default:
		EXERR("84FR", "Invalid baud rate %d provided", iBaud);
		break;
	}

	// Setup the DCB
	DCB dcb;
	memset(&dcb, 0, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = iBaud;
	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fAbortOnError = FALSE;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	if (!open(QIODevice::ReadWrite))
		EXERR("PLP4", "Could not open COM port %d", iPort);

	// HACK: Debugging
	DCB dcb2;
	memset(&dcb2, 0, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);
	GetDCB(&dcb2);	

	SetDCB(&dcb);
}

bool SerialPort::open(OpenMode mode)
{
	QString sPort = QString("\\\\.\\COM%1").arg(QString::number(m_iCommPort));
	WCHAR szPort[32];
	memset(szPort, 0, 32);
	sPort.toWCharArray(szPort);

	if (m_iCommPort <= 0)
		EXERR("HGZG", "Comm port %d is not valid", m_iCommPort);

	// When waking up from sleep, it sometimes takes a moment
	// for the serial port to be ready. Poll it.
	QElapsedTimer timer;
	timer.start();

	while(true)
	{
		// Attempt to open the serial port
		m_hFile = CreateFile(szPort, GENERIC_WRITE | GENERIC_READ,
							0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if(INVALID_HANDLE_VALUE != m_hFile)
		{
			// All is well
			//LOGDBG("SerialPort::Open()-Opened '%s'\r\n", qPrintable(sPort));
			return QIODevice::open(mode);
		}

		// Error!
		m_hFile = 0;
		DWORD dwErr = ::GetLastError();
		if(ERROR_DEV_NOT_EXIST != dwErr)
		{
			// A real error occurred
			EXERR("HQ66", "SerialPort::Open- Could not open port '%s', error %d (CreateFile returned 0x%08X)\r\n",
						qPrintable(sPort), dwErr, m_hFile);	
		}

		// If we got here, then it was because the serial port is not yet ready.
		// Try again in a moment.
		EXERR("U4YJ", "SerialPort::Open- Serial port not available '%s', error %d (CreateFile returned 0x%08X)\r\n",
						qPrintable(sPort), dwErr, m_hFile);
		
		// Timed out?
		if(timer.elapsed() > OPEN_TIMEOUT_MS)
		{
			EXERR("QCZZ", "SerialPort::Open- Timed out waiting for serial port '%s' to become available\r\n", qPrintable(sPort));
		}
		
		// Pause a moment
		QThread::msleep(250);
	}

	return true;
}




void SerialPort::close()
{
	if (!m_hFile)
		return;
	
	if(!::PurgeComm(m_hFile, PURGE_TXABORT | PURGE_RXABORT))
	{
		//LOGDBG("SerialPort::Close PurgeComm failed! (err %d)\r\n", ::GetLastError());
		return;
	}

	//TRACE("DBG:  SerialPort::RunThread::Close +::CloseHandle (m_hFile=%08X)\r\n", m_hFile));
	::SetCommMask(m_hFile, 0);
	DWORD dwErrors;
	COMSTAT stat;
	::ClearCommError(m_hFile, &dwErrors, &stat);
	//TRACE("DBG:  SerialPort::RunThread::Close  ::ClearCommError(0x%08X)\r\n", dwErrors));
	::CloseHandle(m_hFile);
	//TRACE("DBG:  SerialPort::RunThread::Close -::CloseHandle\r\n"));
	m_hFile = 0;

	QIODevice::close();
}



void SerialPort::GetDCB(DCB *pDCB)
{
	// We do NOT want this assertion. We want it to fail properly if it 
	// is closed by another thread
	//ASSERT(m_hFile);

	if (!::GetCommState(m_hFile, pDCB))
		EXERR("LZGU", "Failed to fetch DCB");
}

void SerialPort::SetDCB(DCB *pDCB)
{
	// We do NOT want this assertion. We want it to fail properly if it 
	// is closed by another thread
	//ASSERT(m_hFile);

	pDCB->DCBlength = sizeof(DCB);       
	if (!::SetCommState(m_hFile, pDCB))
		EXERR("TAGD", "Failed to set DCB");
}


void SerialPort::GetTimeouts(COMMTIMEOUTS *pTimeouts)
{
	// We do NOT want this assertion. We want it to fail properly if it 
	// is closed by another thread
	//ASSERT(m_hFile);

	if (!::GetCommTimeouts(m_hFile, pTimeouts))
		EXERR("U2Q4", "Failed to get timeouts");
}

void SerialPort::SetTimeouts(COMMTIMEOUTS *pTimeouts)
{
	// We do NOT want this assertion. We want it to fail properly if it 
	// is closed by another thread
	//ASSERT(m_hFile);

	if(!::SetCommTimeouts(m_hFile, pTimeouts))
		EXERR("6WHP", "Failed to set timeouts %d", ::GetLastError());
}



/****************************************************************
Write

	Write data to the serial port
****************************************************************/
qint64 SerialPort::writeData(const char *pData, qint64 qiSize)
{
	Q_ASSERT(m_hFile);
	DWORD dwTotalBytesWritten = 0;
	DWORD dwBytesLeft = qiSize;
	int count = 0;

	while (dwTotalBytesWritten < qiSize)
	{
		DWORD dwBytesWritten;
		if (!::WriteFile(m_hFile, pData, dwBytesLeft, &dwBytesWritten, NULL))
			EXERR("GEAK", "Error %d while writing to serial port", ::GetLastError());

		dwTotalBytesWritten += dwBytesWritten;
		dwBytesLeft -= dwBytesWritten;

		// I saw this issues with serial bus (happens very often when using RAEGuard PID),
		// the Borg freezes in this loop because serial bus is in some bad non-responsive condition.
		// Adding this check to warn the user if this happens.
		if (dwTotalBytesWritten == 0)
			count++;
		else
			count = 0;

		if (count > 100)
		{
			// We tried to write to serial bus 100 times, but it's stuck and not responding
			// For my PC the only thing worked was to shut down the computer and turn it back on.
			EXERR("WUC6", "Looks like serial bus is in bad condition and not responding. Shutdown your PC, then turn it back on. It should fix this issue.");
		}
	}

	return dwTotalBytesWritten;
}



/****************************************************************
Read

	Read data from the serial port.
****************************************************************/
qint64 SerialPort::readData(char *pData, qint64 qiSize)
{
	Q_ASSERT(m_hFile);
	// We do NOT want this assertion. We want it to fail properly if it 
	// is closed by another thread
	//ASSERT(m_hFile);
	DWORD dwBytesRead = 0;
	if (!::ReadFile(m_hFile, pData, qiSize, &dwBytesRead, NULL))
		EXERR("HEZW", "Error %d while reading from serial port", ::GetLastError());

	return dwBytesRead;
}


/***************************************************************
Flush

	
***************************************************************/
void SerialPort::Flush(void)
{
	Q_ASSERT(m_hFile);
	if (!::PurgeComm(m_hFile, PURGE_RXCLEAR | PURGE_TXCLEAR))
		EXERR("AQEW", "Flush Failed");
}




void SerialPort::GetCommMask(ulong *lpEvtMask)
{
	Q_ASSERT(m_hFile);
	if (!::GetCommMask(m_hFile, lpEvtMask))
		EXERR("TJ4K", "Failed to get Comm Mask");
}


void SerialPort::SetCommMask(ulong dwEvtMask)
{
	Q_ASSERT(m_hFile);
	if(!::SetCommMask(m_hFile, dwEvtMask))
		EXERR("GD9W", "Failed to set Comm Mask");
}

QList<int> SerialPort::ListCommPorts()
{
	QList<int> list;
	
	for (int i = 1; i <= 64; i++)
	{
		QString sPort = QString("\\\\.\\COM%1").arg(QString::number(i));
		WCHAR szPort[32];
		memset(szPort, 0, 32);
		sPort.toWCharArray(szPort);

		HANDLE hFile = CreateFile(szPort, GENERIC_WRITE | GENERIC_READ,
			0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			list << i;
			//LOGINFO("PORT %d", i);
			::CloseHandle(hFile);
		}
	}

	return list;
}
