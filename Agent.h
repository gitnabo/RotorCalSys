#pragma once
#include <QSerialPort>


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

	// Rotor Calibration Constants
	float m_fRotorConstSlope = 0.024485714f;
	float m_fRotorConst0Intcerpt = -31.46047619f;
	float m_fLoadCellGainSlope = 1.098320069f; /// Based on exp with Doug
	float m_fLoadCellGainIntc = 0.108237333f; /// Based on exp with Doug
};


Q_DECLARE_METATYPE(Agent::Data)