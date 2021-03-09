#pragma once
#include <QSerialPort>

// OLD Rotor Calibration Constants
/// const float m_fRotorConstSlope = 0.024485714f; // DELETE?
/// const float m_fRotorConstIntc = -31.46047619f; // DELETE?


/// Load Cell
const float m_fLoadCellGainSlope = 1.098320069f; /// Based on exp with Doug
const float m_fLoadCellGainIntc = 0.108237333f; /// Based on exp with Doug

/// Motor
const float m_fMotorConstSlope = 3.861666667f; /// Based on measurement Cyril & Ish Sept 14th 2020 
const float m_fMotorConstInct = -4265.944444f; /// Based on measurement Cyril & Ish Sept 14th 2020 


/// --- NEW ROTOR ---
const float m_fRotorPwmToDegAoaSlope = 0.013222f; // Based on measurement
const float m_fRotorPwmToDegAoaIntc = -13.6744f; // Base on measurement 

const float m_fRotorPwmToServoDegSlope = -0.0823f;
const float m_fRotorPwmToServoDegIntc = 125.35f;


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

	void SetMotorSpeedRPM(float fMotorSpeedRpm);

	static float ConvPwmToAoaDegree(float fPwmAOA); /// Angle of Attack
	static float ConvDegreeToPwm(float fDegreeAOA); // # ToDo Change to PWM
	static float ConvPwmToServoDeg(float fPwm);

	


	struct Data {		
		int iSampleMs; /// Returns the number of milliseconds passed since the 
		             /// Arduino board began running the current program. 
					 /// This number will overflow (go back to zero), after approximately 50 days.
		float fLoadCellKg; /// Kg
		float fServoCurrent; /// mA
		float fServoVoltage; /// V
		float fMotorControllerCurrent; /// A
		float fMotorControllerVoltage; /// V
	    float fServoPosPwm; /// us pulse width
	    float fServoPosDegEstimate; /// Estimate of Degree based on previous Calc

		float fMotorSpeedPwm; /// us pulse width
		float fMotorSpeedRpmData; /// Calc using fMotorSpeedPwm
	};
	Data GetData();
	Agent::Data GetDataTEST();
	float GetScale();	///< Just read the scale

	
private:
	QSerialPort m_serial;
	QString ReadLine();

	QString Req_Echo(const QString& sMsg);

	// Do a complete transaction
	QString Tx(QString sReq);
	uint m_uiRequestID = 0;
};


Q_DECLARE_METATYPE(Agent::Data)