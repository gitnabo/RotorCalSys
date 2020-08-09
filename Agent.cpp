#include "Agent.h"



Agent::Agent()
{
}


Agent::~Agent()
{
	Close();
}


void Agent::Open()
{
	// For now, fixed at COM4
	m_serial.Open(4, 57600);
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

	// Read data from the serial port
	QByteArray ba = m_serial.read(64);
	QString s(ba);

	// Parse out the data
	Data data;
	memset(&data, 0, sizeof(data));
	return data;
}