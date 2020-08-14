#include "Agent.h"
#include <QSerialPortInfo>
#include <QElapsedTimer>
#include <QThread>

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
		throw QString("Could not open serial port %1").arg("x");

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
		throw QString("No response from test stand");

	// Spin a bit so that we can hopefully just read a single line
	QElapsedTimer tmr;
	tmr.start();
	while (!m_serial.canReadLine())
	{
		m_serial.waitForReadyRead(30);
		if (tmr.elapsed() > 1000)
			throw QString("No response from test stand (no full line)");
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
	int iDataPkgSize = 9;
	QStringList slTokens = sLine.split(',');
	if(iDataPkgSize != slTokens.count())
		throw QString("Invalid printContinuous response size");

	// Parse the line tokens "Read:,85435976,-0.34,0.00,0.04,0.17,0.00,50,50"
	Data data;
	memset(&data, 0, sizeof(data)); /// Clears data from any previous data
	bool bOk;      
	
	// data.sRead = slTokens.at(0); /// Parsing "Read:" for consistency // Generating error

	data.fTime = slTokens.at(1).toFloat(&bOk); 
	if (!bOk) {
		throw QString("Bad value received from Arduino: fTime");
	}

	data.fLoadCell = slTokens.at(2).toFloat(&bOk);
	if (!bOk) {
		throw QString("Bad value received from Arduino: fLoadCell");
	}

	data.fServoCurrent = slTokens.at(3).toFloat(&bOk);
	if (!bOk) {
		throw QString("Bad value received from Arduino: fServoCurrent");
	}

	data.fServoVoltage = slTokens.at(4).toFloat(&bOk);
	if (!bOk) {
		throw QString("Bad value received from Arduino: fServoVoltage");
	}

	data.fMotorControllerCurrent = slTokens.at(5).toFloat(&bOk);
	if (!bOk) {
		throw QString("Bad value received from Arduino: fMotorControllerCurrent");
	}

	data.fMotorControllerVoltage = slTokens.at(6).toFloat(&bOk);
	if (!bOk) {
		throw QString("Bad value received from Arduino: fMotorControllerVoltage");
	}

	data.fServoPostion = slTokens.at(7).toFloat(&bOk);
	if (!bOk) {
		throw QString("Bad value received from Arduino: fServoPostion");
	}

	data.fMotorRpmSetting = slTokens.at(8).toFloat(&bOk);
	if (!bOk) {
		throw QString("Bad value received from Arduino: fMotorRpmSetting");
	}

	return data; 
}

// ! setServoTwoPos = Engine !
void Agent::SetRPM(float fRPM)
{
	m_serial.write("setServoTwoPos\r\n");
	m_serial.waitForBytesWritten(1);

	QString sLine = ReadLine();

	// Parse out the response
	QStringList slTokens = sLine.split(',');
}

// ! setServoOnePos = Servo !
void Agent::SetPitch(float fDegrees)
{

}
