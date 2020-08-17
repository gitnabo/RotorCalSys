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
	m_serial.close();
}


QString Agent::ReadLine()
{
	// Wait for some data to arrive to the serial port internal buffer
	if (!m_serial.waitForReadyRead(TIMEOUT_MS))
		throw Exception("No response from test stand");

	// Spin a bit so that we can hopefully just read a single line
	QElapsedTimer tmr;
	tmr.start();
	while (!m_serial.canReadLine())
	{
		m_serial.waitForReadyRead(30);
		if (tmr.elapsed() > 1000)
			throw Exception("No response from test stand (no full line)");
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
	m_serial.write("printContinuous\r\n");
	m_serial.waitForBytesWritten(1);

	QString sLine = ReadLine();

	// Parse out the data
	int iDataPkgSize = 8;
	QStringList slTokens = sLine.split(',');
	if(iDataPkgSize != slTokens.count())
		throw Exception("Invalid printContinuous response size");

	// Parse the line tokens "Read:,85435976,-0.34,0.00,0.04,0.17,0.00,50,50"
	Data data;
	memset(&data, 0, sizeof(data)); /// Clears data from any previous data
	bool bOk;   //	
	
	data.fTime = slTokens.at(1).toFloat(&bOk); 
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fTime");
	}

	data.fLoadCell = slTokens.at(2).toFloat(&bOk);
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

	data.iServoPos = slTokens.at(7).toFloat(&bOk);
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fServoPostion");
	}

	data.iMotorSpeed = slTokens.at(8).toFloat(&bOk);
	if (!bOk) {
		throw Exception("Bad value received from Arduino: fMotorRpmSetting");
	}

	return data; 
}


//  Servo Pos = setServoOnePos
void Agent::SetPitch(int iServoPos)
{
	QString sServoPos = QString::number(iServoPos);
	QByteArray baServoPos = sServoPos.toLocal8Bit();
	const char *ccServoPos = baServoPos.data();
	
	m_serial.write("setServoOnePos");
	m_serial.write(ccServoPos);
	m_serial.write("\r\n");
	m_serial.waitForBytesWritten(1);

	// Do we need the read back?
	/*
	QString sLine = ReadLine();

	// Parse out the response
	QStringList slTokens = sLine.split(',');
	*/
}

// ! Engine RPM = setServoTwoPos !
void Agent::SetMotorSpeed(int iMotorSpeedCmd)
{
	QString sMotorSpeed = QString::number(iMotorSpeedCmd);
	QByteArray baMotorSpeed = sMotorSpeed.toLocal8Bit();
	const char *ccMotorSpeed = baMotorSpeed.data();

	m_serial.write("setServoTwoPos");
	m_serial.write(ccMotorSpeed);
	m_serial.write("\r\n");
	m_serial.waitForBytesWritten(1);

	// Do we need the read back?
	/*
	QString sLine = ReadLine();

	// Parse out the response
	QStringList slTokens = sLine.split(',');
	*/
}


