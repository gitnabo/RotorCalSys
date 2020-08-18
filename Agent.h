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

	
	void SetPitch(float fServoPos); ///< Or PWM?
	void SetMotorSpeed(float fMotorSpeedCmd);
	

	struct Data {		
		float fTime; /// Unformatted
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

	void RunSequence();

	float ConvPwmToDegree(float fPwmAOA); /// Angle of Attack
	float ConvDegreeToPwm(float fDegreeAOA); /// Angle of Attack
	float fDegreeAOA; /// Angle of Attack

	// Rotor Calibration Constants
	float m_fRotorConstSlope = 0.024485714;
	float m_fRotorConst0Intcerpt = -31.46047619;
	float m_fAngleAtStartOfTestDegree = -1;
	float m_fAngleAtEndOfTestDegree = 12;
	int   m_iTimeSpentAtAOA = 30000;


};


Q_DECLARE_METATYPE(Agent::Data)