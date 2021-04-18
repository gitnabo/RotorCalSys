#include "pch.h"
#include "Agent.h"
#include <QSerialPortInfo>
#include <QElapsedTimer>
#include <QThread>
#include "Exception.h"
#include <windows.h>
#include <QDebug>

#define TIMEOUT_MS		30000



Agent::Agent()
{
}


Agent::~Agent()
{
	Close();
}


void Agent::Open(const QString& sPort)
{
	int iBaud = 57600;
	/*
	QString sDevice = QString("\\\\.\\%1").arg(sPort);
	WCHAR szPort[32];
	memset(szPort, 0, 32);
	sDevice.toWCharArray(szPort);

	// Attempt to open the serial port
	HANDLE hFilevice = ::CreateFile(szPort, GENERIC_WRITE | GENERIC_READ,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (INVALID_HANDLE_VALUE == hFilevice){
		QString sEx = QString ("Could not open COM port %1").arg(sPort);
		throw Exception(sEx);
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
	
	BOOL bOk = ::SetCommState(hFilevice, &dcb);
	::CloseHandle(hFilevice);
	if (!bOk){
		throw Exception ("Failed to set DCB");
	}
	*/

	QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
	QSerialPortInfo port(sPort);
	m_serial.setPort(port);
	m_serial.setBaudRate(iBaud);
	m_serial.setDataBits(QSerialPort::Data8);
	m_serial.setStopBits(QSerialPort::StopBits::OneStop);
	m_serial.setParity(QSerialPort::Parity::NoParity);
	m_serial.setFlowControl(QSerialPort::NoFlowControl);
	if (!m_serial.open(QIODevice::ReadWrite))
		throw Exception("Could not open serial port %1").arg("x");

	// Read whatever data might still be in the serial port buffer, ya never know!
	m_serial.readAll();

	// Do a sanity check upfront to make sure everything is good
	QString sMsg = "Hello";
	QString s = Req_Echo(sMsg);
	if(s != sMsg)
		throw Exception("Echo test to arduino failed");
}


void Agent::Close()
{
	try{ 
		if (m_serial.isOpen())
			SetMotorSpeedRPM(0);
	}
	catch (const Exception&)
	{

	}
	m_serial.close();
}

QString Agent::GetVersion()
{
	QString sVer = Tx("getversion");
	return sVer;
}

QString Agent::Tx(QString sReq)
{
	QString sCmd = QString("%1: %2\r\n").arg(
				QString::number(++m_uiRequestID), sReq);
	m_serial.write(qPrintable(sCmd));
	m_serial.waitForBytesWritten(1);

	// Read the response from the arduino

	// Are we happy with it?
	QString sResp = ReadLine();

	// Parse out the ID to verify the protocol
	int iColonPos = sResp.indexOf(':');
	if(-1 == iColonPos)
		throw Exception("Invalid Response: No colon");
	QString sID = sResp.left(iColonPos);
	int iID = sID.toInt();
	if (iID != m_uiRequestID) {
		throw Exception("Invalid ID, Protocol out of sync.");
	}
	sResp.remove(0, iColonPos + 1);
	QString sData = sResp.trimmed();

	return sData;
}


QString Agent::ReadLine()
{
	// Spin a bit so that we can hopefully just read a single line
	QElapsedTimer tmr;
	tmr.start();
	while (!m_serial.canReadLine()) // Can Not Read Line Not = True
	{
		m_serial.waitForReadyRead(50); // ###TS was 30 // Delete foo
		if (tmr.elapsed() > TIMEOUT_MS) {
			// Something went wrong
			
			if (m_serial.bytesAvailable() == 0)
				throw Exception("No response from test stand");
			else
				throw Exception("No response from test stand (no full line)")
			;
		}
	}

	// Read the line
	QString sLine = m_serial.readLine();
	return sLine.trimmed();
}


QString Agent::Req_Echo(const QString& sMsg)
{
	QString sReq = QString("echo %1").arg(sMsg);
	return Tx(sReq);
}


Agent::Data Agent::GetData()
{
	QString sResp = Tx("getdata");

	QStringList slTokens = sResp.split(',');

	QMap<QString, QString> mapData;
	for (const QString& s : slTokens)
	{
		// Parse out the key/value
		QStringList sl = s.split('=');
		if(sl.count() != 2)
			throw Exception("Invalid getdata response");
		mapData[sl.at(0)] = sl.at(1);
	}

	Data data;
	memset(&data, 0, sizeof(data)); /// Clears data from any previous data
	bool bOk;   //	
	
	data.iSampleMs = mapData.value("ms").toInt(&bOk);
	if (!bOk)
		throw Exception("Bad value received from Arduino: fTime");

	// LoadCell gains is adjusted here
	float fLoadCellRawKg = mapData.value("kg").toFloat(&bOk);
	if (!bOk)
		throw Exception("Bad value received from Arduino: fLoadCell");
	data.fLoadCellKg = (fLoadCellRawKg * m_fLoadCellGainSlope + m_fLoadCellGainIntc) / 2.205;
	//  2.205 is constant to convert from Lb to kg

	data.fServoCurrent = mapData.value("servo_amp").toFloat(&bOk);
	if (!bOk)
		throw Exception("Bad value received from Arduino: fServoCurrent");	

	data.fServoVoltage = mapData.value("servo_volt").toFloat(&bOk);
	if (!bOk)
		throw Exception("Bad value received from Arduino: fServoVoltage");	

	data.fMotorControllerCurrent = mapData.value("motor_amp").toFloat(&bOk);
	if (!bOk)
		throw Exception("Bad value received from Arduino: fMotorControllerCurrent");	

	data.fMotorControllerVoltage = mapData.value("motor_volt").toFloat(&bOk);
	if (!bOk)
		throw Exception("Bad value received from Arduino: fMotorControllerVoltage");	

	data.fServoPosPwm = mapData.value("servo_pitch").toFloat(&bOk);
	if (!bOk)
		throw Exception("Bad value received from Arduino: fServoPostion");

	data.fServoPosDegEstimate = ConvPwmToAoaDegree(data.fServoPosPwm); /// Estimate of Degree based on previous Calc

	// Motor
	data.fMotorSpeedPwm = mapData.value("motor_rpm").toFloat(&bOk);
	if (!bOk)
		throw Exception("Bad value received from Arduino: fMotorRpmSetting");	

	data.fMotorSpeedRpmData = data.fMotorSpeedPwm * m_fMotorConstSlope + m_fMotorConstInct;
	
	return data; 
}

float Agent::GetScale()
{
	QString sResp = Tx("getscale");

	// Parse out the scale, it just returns one float
	bool bOk;
	float fScale = sResp.toFloat(&bOk);
	if (!bOk) 
		throw Exception("Bad scale value received from Arduino");

	return fScale;
}

///  Servo Pos = setServoOnePos
void Agent::SetPitch(float fDegree)
{
	float fAnglePwm = ConvDegreeToPwm(fDegree);
	int iPitchPwm = qRound(fAnglePwm);

	QString sRequest = QString("setpitchpwm %1").arg(iPitchPwm);
	Tx(sRequest);
}


// ! Engine RPM = setServoTwoPos !
void Agent::SetMotorSpeedRPM(float fMotorSpeedRpm)
{
	// Convert fMotorSpeedRpm to PWM signal
	/// Equation based on Lenny's measurements 
	/// with a tach in early 2020
	/// https://1drv.ms/x/s!AtnJKVikx6OogcJpH6lJq_ltK2r20Q?e=FZvUaX
	float fMotorSpeedPwmCdm;

	// To Initiate and stop the motor an PWM signal of 1000 is expected
	if (fMotorSpeedRpm == 0)
		fMotorSpeedPwmCdm = 1000;
	else
		fMotorSpeedPwmCdm = (fMotorSpeedRpm - m_fMotorConstInct) / m_fMotorConstSlope;
	   
	QString sRequest = QString("setrpmpwm %1").arg(qRound(fMotorSpeedPwmCdm));
	Tx(sRequest);
}


/// Angle of attack: Convert from PWM to AoA PWM 
float Agent::ConvPwmToAoaDegree(float fPwmAOA)
{
	float fAoADegree;
	fAoADegree = m_fRotorPwmToDegAoaSlope * fPwmAOA + m_fRotorPwmToDegAoaIntc;
	return fAoADegree;
}

/// Angle of attack: Convert from AoA Degree to PWM 
float Agent::ConvDegreeToPwm(float fAoaDeg)
{
	float fServoPwm;
	fServoPwm = (fAoaDeg - m_fRotorPwmToDegAoaIntc) / m_fRotorPwmToDegAoaSlope;
	return fServoPwm;
}

/// Servo: Convert PWM to Servo Degree 
float Agent::ConvPwmToServoDeg(float fPwm)
{
	float fServoDeg;
	fServoDeg = m_fRotorPwmToServoDegSlope * fPwm + m_fRotorPwmToServoDegIntc;
	return fServoDeg;
}

