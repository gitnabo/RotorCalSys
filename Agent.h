#pragma once
#include <QSerialPort>

// Rotor Calibration Constants
const float m_fRotorConstSlope = 0.024485714f;
const float m_fRotorConstIntc = -31.46047619f;
const float m_fLoadCellGainSlope = 1.098320069f; /// Based on exp with Doug
const float m_fLoadCellGainIntc = 0.108237333f; /// Based on exp with Doug
const float m_fMotorConstSlope = 261.93f; /// Based on Lenny's exp in early 2020
const float m_fMotorConstInct = 1465.9f; /// Based on Lenny's exp in early 2020


/**
@brief API for the Arduino test stand

We talk to the Arduino test stand with a serial port
*/
class Agent
{
public:
	Agent();
	~Agent();

	void Open(const QString& sPort);
	void Close();

	
	void SetPitch(float fDegree); 
	void SetMotorSpeed(float fMotorSpeedRpm);
	

	struct Data {		
		float fTime; /// Returns the number of milliseconds passed since the 
		             /// Arduino board began running the current program. 
					 /// This number will overflow (go back to zero), after approximately 50 days.
		float fLoadCell; /// Lbs
		float fServoCurrent; /// mA
		float fServoVoltage; /// V
		float fMotorControllerCurrent; /// A
		float fMotorControllerVoltage; /// V
	    float fServoPos; /// us pulse width
		float fMotorSpeed; /// us pulse width
	};
	Data GetData();



	
private:
	QSerialPort m_serial;
	QString ReadLine();


	float ConvPwmToDegree(float fPwmAOA); /// Angle of Attack
	float ConvDegreeToPwm(float fDegreeAOA); /// Angle of Attack
};


Q_DECLARE_METATYPE(Agent::Data)