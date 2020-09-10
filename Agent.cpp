#include "pch.h"
#include "Agent.h"
#include <QSerialPortInfo>
#include <QElapsedTimer>
#include <QThread>
#include "Exception.h"

#define TIMEOUT_MS		4000



Agent::Agent()
{
}

Agent::~Agent()
{
	Close();
}


void Agent::Open(const QString& sPort)
{
	QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
	QSerialPortInfo port(sPort);
	m_serial.setPort(port);
	m_serial.setBaudRate(57600);
	m_serial.setDataBits(QSerialPort::Data8);
	m_serial.setStopBits(QSerialPort::StopBits::OneStop);
	m_serial.setParity(QSerialPort::Parity::NoParity);
	m_serial.setFlowControl(QSerialPort::NoFlowControl);
	if (!m_serial.open(QIODevice::ReadWrite))
		throw Exception("Could not open serial port %1").arg("x");

	// Read whatever data might still be in the serial port buffer, ya never know!
	m_serial.readAll();
}

void Agent::Close()
{
	if (m_serial.isOpen()) {
		SetMotorSpeedRPM(0);
	} // !!!! Make sure this Stops
	m_serial.close();
}


QString Agent::ReadLine()
{
	// Spin a bit so that we can hopefully just read a single line
	QElapsedTimer tmr;
	tmr.start();
	while (!m_serial.canReadLine())
	{
		m_serial.waitForReadyRead(30);
		if (tmr.elapsed() > TIMEOUT_MS) {
			// Something went wrong
			if (m_serial.bytesAvailable() == 0)
				throw Exception("No response from test stand");
			else
				throw Exception("No response from test stand (no full line)");
		}
	}

	// Read the line
	QString sLine = m_serial.readLine();
	return sLine;
}

Agent::Data Agent::GetData()
{
	// Send the command to request the data
	/// Read whatever data might still be in the serial port buffer, ya never know!
	m_serial.readAll();
	m_serial.write("printVals\r\n");
	m_serial.waitForBytesWritten(1);

	QString sLine = ReadLine();

	// Parse out the data
	int iDataPkgSize = 9;
	QStringList slTokens = sLine.split(',');
	if(iDataPkgSize != slTokens.count())
		throw Exception("Invalid printContinuous response size");

	// Parse the line tokens "Read:,85435976,-0.34,0.00,0.04,0.17,0.00,50,50"
	Data data;
	memset(&data, 0, sizeof(data)); /// Clears data from any previous data
	bool bOk;   //	
	
	data.iSampleMs = slTokens.at(1).toFloat(&bOk); 
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fTime");
	}

	// LoadCell gains is adjusted here
	data.fLoadCellKg = ((slTokens.at(2).toFloat(&bOk)) * m_fLoadCellGainSlope + m_fLoadCellGainIntc) / 2.205; 
	//  2.205 is constant to convert from Lb to kg
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fLoadCell");
	}

	data.fServoCurrent = slTokens.at(3).toFloat(&bOk);	
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fServoCurrent");
	}

	data.fServoVoltage = slTokens.at(4).toFloat(&bOk);
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fServoVoltage");
	}

	data.fMotorControllerCurrent = slTokens.at(5).toFloat(&bOk);
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fMotorControllerCurrent");
	}

	data.fMotorControllerVoltage = slTokens.at(6).toFloat(&bOk);
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fMotorControllerVoltage");
	}

	//Servo
	data.fServoPosPwm = slTokens.at(7).toFloat(&bOk);
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fServoPostion");
	}

	data.fServoPosDegEstimate = ConvPwmToAoaDegree(data.fServoPosPwm); /// Estimate of Degree based on previous Calc


	// Motor
	data.fMotorSpeedPwm = slTokens.at(8).toFloat(&bOk);
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fMotorRpmSetting");
	}

	data.fMotorSpeedRpmData = data.fMotorSpeedPwm * m_fMotorConstSlope + m_fMotorConstInct;
	


	return data; 
}

///  Servo Pos = setServoOnePos
void Agent::SetPitch(float fDegree)
{
	float fAnglePwm = ConvDegreeToPwm(fDegree);

	QString sServoPos = QString::number(fAnglePwm);
	QByteArray baServoPos = sServoPos.toLocal8Bit();
	const char *ccServoPos = baServoPos.data(); /// Creates a pointer. 
												/// This is not the data from Agent::Data Agent::GetData()	
	m_serial.write("setServoOnePos");
	m_serial.write(ccServoPos);
	m_serial.write("\r\n");
	m_serial.waitForBytesWritten(1);
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
	if (fMotorSpeedRpm == 0) {
		fMotorSpeedPwmCdm = 1000;
	}
	else {
		fMotorSpeedPwmCdm = (fMotorSpeedRpm - m_fMotorConstInct) / m_fMotorConstSlope;
	}
	

	QString sMotorSpeed = QString::number(fMotorSpeedPwmCdm);
	QByteArray baMotorSpeed = sMotorSpeed.toLocal8Bit();
	const char *ccMotorSpeed = baMotorSpeed.data();

	m_serial.write("setServoTwoPos");
	m_serial.write(ccMotorSpeed);
	m_serial.write("\r\n");
	m_serial.waitForBytesWritten(1);
}
void Agent::ZeroScale() {
	m_serial.write("zeroScale");
	m_serial.write("\r\n");
	m_serial.waitForBytesWritten(1);
}

// ------ OLD ROTOR ------

/// Angle of attack: Convert from PWM to Degrees 

float Agent::ConvPwmToAoaDegree(float fPwmAOA)
{
	float fDegree;
	fDegree = fPwmAOA * m_fRotorConstSlope + m_fRotorConstIntc;
	return fDegree;
}

/// Angle of attack: Convert from AoA Degrees to PWM
float Agent::ConvDegreeToPwm(float fDegreeAOA)
{
	float fPwm;
	fPwm = (fDegreeAOA - m_fRotorConstIntc) / m_fRotorConstSlope;
	return fPwm;
}

// Servo: Convert PWM to Servo Degree 
float Agent::ConvPwmToServoDeg(float fPwm)
{
	float fServoDeg;
	fServoDeg = m_fOLDRotorPwmToServoDegSlope * fPwm + m_fOLDRotorPwmToServoDegInt;
	return fServoDeg;
}


// ------ NEW ROTOR -------
/*
///  Servo Pos = setServoOnePos
void Agent::SetServoAnglePwm(float fServoAnglePwm)
{
	QString sServoAnglePwm = QString::number(fServoAnglePwm);
	QByteArray baServoPos = sServoAnglePwm.toLocal8Bit();
	const char *ccServoPos = baServoPos.data(); /// Creates a pointer. 
												/// This is not the data from Agent::Data Agent::GetData()	
	m_serial.write("setServoOnePos");
	m_serial.write(ccServoPos);
	m_serial.write("\r\n");
	m_serial.waitForBytesWritten(1);
}

/// Angle of attack: Convert from PWM to AoA PWM 
float Agent::ConvPwmToAoaDegree(float fPwmAOA)
{
	float fAoADegree;
	fAoADegree = m_fNEWRotorPwmToDegAoaSlope * log(fPwmAOA) + m_fNEWRotorPwmToDegAoaIntc;
	return fAoADegree;
}


/// Angle of attack: Convert from AoA Degree to PWM 
float Agent::ConvDegreeToPwm(float fAoaDeg)
{
	float fServoPwm;
	fServoPwm = exp(((fAoaDeg - m_fNEWRotorPwmToDegAoaIntc) / m_fNEWRotorPwmToDegAoaSlope));
	return fServoPwm;
}


// Servo: Convert PWM to Servo Degree 
float Agent::ConvPwmToServoDeg(float fPwm)
{
	float fServoDeg;
	fServoDeg = m_fPwmToServoDegSlope * fPwm + m_fPwmToServoDegInt;
	return fServoDeg;
}
*/

