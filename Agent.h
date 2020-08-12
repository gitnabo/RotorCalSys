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

	void SetRPM(float fRPM);
	void SetPitch(float fDegrees); ///< Or PWM?

	struct Data {		
		QString Read;
		float fTime; /// Unformated
		float fLoadCell; /// Lbs
		float fServoCurrent; /// mA
		float fServoVoltage; /// V
		float fMotorControllerCurrent; /// A
		float fMotorControllerVoltage; /// V
		float fServoPostion; /// us pulse width
		float fMotorRpmSetting; /// us pulse width
	};
	Data GetData();

private:
	QSerialPort m_serial;
	QString ReadLine();
};

