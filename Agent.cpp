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
}

void Agent::Close()
{
	m_serial.close();
}

void Agent::SetRPM(float fRPM)
{

}

void Agent::SetPitch(float fDegrees)
{

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

	// Read data from the serial port
	if (!m_serial.waitForReadyRead(TIMEOUT_MS))
		throw QString("No response from test stand");

	QElapsedTimer tmr;
	tmr.start();
	while (!m_serial.canReadLine())
	{
		QThread::msleep(30);
		if(tmr.elapsed() > 1000)
			throw QString("No response from test stand (no full line)");
	}

	QString sLine(m_serial.readLine());

	// Parse out the data
	QStringList slTokens = sLine.split(',');
	Data data;
	memset(&data, 0, sizeof(data));
	return data;
}