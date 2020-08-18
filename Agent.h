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
};


Q_DECLARE_METATYPE(Agent::Data)