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
	dcb.XonChar = 0xE5;


	// For now, fixed at COM4
	m_serial.SetCommPort(4);
	m_serial.open(QIODevice::ReadWrite);
	m_serial.SetDCB(&dcb);
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
	m_serial.write("printVals\r\n");
	m_serial.Flush();

	// Read data from the serial port
	QByteArray ba = m_serial.read(64);
	QString s(ba);

	// Parse out the data
	Data data;
	memset(&data, 0, sizeof(data));
	return data;
}