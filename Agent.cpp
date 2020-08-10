#include "Agent.h"
#include <windows.h>

Agent::Agent()
{
}


Agent::~Agent()
{
	Close();
}


void Agent::Open()
{
	// Setup the DCB
	DCB dcb;
	memset(&dcb, 0, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = 57600;
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
	dcb.XonChar = 0x03;
	dcb.XoffChar = 0xE5;


	// For now, fixed at COM4
	//m_serial.SetCommPort(3);
	//m_serial.open(QIODevice::ReadWrite);
	//m_serial.SetDCB(&dcb);
	m_serial.Open(3, 57600);

	COMMTIMEOUTS to;
	memset(&to, 0, sizeof(to));
	to.ReadIntervalTimeout = 10;
	to.ReadTotalTimeoutMultiplier = 2;
	to.ReadTotalTimeoutConstant = 100;
	to.WriteTotalTimeoutMultiplier = 2;
	to.WriteTotalTimeoutConstant = 50;
	m_serial.SetTimeouts(&to);
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
	m_serial.Flush();

	// Read data from the serial port
	QByteArray ba = m_serial.read(15);
	QString s(ba);

	// Parse out the data
	Data data;
	memset(&data, 0, sizeof(data));
	return data;
}