#pragma once
#include <QThread>
#include "Agent.h"

/**
@brief The test thread to control and gather data
*/
class TestEngine : public QThread
{
	Q_OBJECT
public:
	TestEngine(QObject *parent);
	~TestEngine();	

	void Start(const QString& sPort);
	void Stop();
	bool IsRunning();	

signals:
	void Started();
	void Error(QString sError);
	void Stopped();
	void NewPitch(float fDegrees);
	void NewData(Agent::Data data);
	void Log(QString sMsg);

private:
	void LOG(QString sMsg);
	volatile bool m_bStopRequest = false;
	virtual void run() override;	///< The thread function where 
	void Wait(int iMs);
	void CheckAbort();
	void RunSequence();
	void RunDummyData();
	QString m_sPort;

	float ConvPwmToDegree(float fPwmAOA); /// Angle of Attack
	float ConvDegreeToPwm(float fDegreeAOA); /// Angle of Attack
	float fDegreeAOA; /// Angle of Attack

	// Rotor Calibration Constants
	float m_fRotorConstSlope          =  0.024485714;
	float m_fRotorConst0Intcerpt      = -31.46047619;
	float m_fAngleAtStartOfTestDegree = -1;          
	float m_fAngleAtEndOfTestDegree   = 12;			 
	int   m_iTimeSpentAtAOA           = 1000; // TEMP 30000 -> 1000  for TS      

};

