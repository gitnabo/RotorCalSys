#pragma once
#include "SerialPort.h"

/**
@brief API for the Arduino test stand

We talk to the Arduino test stand with a serial port
*/
class Agent
{
public:
	Agent();
	~Agent();

	void Open();
	void Close();

	void SetRPM(float fRPM);
	void SetPitch(float fDegrees); ///< Or PWM?

	struct Data {
		float fLiftForce;
		float fPitchServoA;
		float fPitchServoV;
		float fMotorPosition;
	};
	Data GetData();

private:
	SerialPort m_serial;
};

