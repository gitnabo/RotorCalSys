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
	m_serial.readLine();
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

/*
struct Data {
	float fLiftForce;
	float fPitchServoA;
	float fPitchServoV;
	float fMotorPosition;
};*/
Agent::Data Agent::GetData()
{
	// Send the command to request the data
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

	/// Check data coming out of the Arduino
	bool bOk;
	for (int i = 1; i < (iDataPkgSize - 1); i++) { /// (iDataPkgSize-1) bc Read is skipped
		slTokens.at(i).toFloat(&bOk);
		if (!bOk) {		
			throw QString("Bad value A received");
		}
	}

	data.fTime = slTokens.at(1).toFloat(); /// Continue for the rest of the values

	return data;
}

void Agent::SetRPM(float fRPM)
{
	m_serial.write("setRpm\r\n");
	m_serial.waitForBytesWritten(1);

	QString sLine = ReadLine();

	// Parse out the response
	QStringList slTokens = sLine.split(',');

}

void Agent::SetPitch(float fDegrees)
{

}
