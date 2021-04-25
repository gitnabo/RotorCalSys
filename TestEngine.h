#pragma once
#include <QThread>
#include "Agent.h"

/*
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

	// Constants
	/// Constants - Rotor Calibration 
	const int m_iMotorRPM = 2960;// Speed for Carbon Fiber Blades is 2960
	const int m_iDelayForMotorRPM = 45000; // ### Should be 45000
	const float m_fAngleAtStartOfTestDegree = 0.0f;     
	const float m_fAngleAtEndOfTestDegree = 11.0f;		 
	const int m_iTimeSpentAtAOA = 40000;
	const int m_iSampleMs = 1000;

	/// Constants - Life Test
	const int m_iTimeSpentAtAOA_LifeTest = 1000;
	const int m_iCycleNum = 3; // Num of times the rotor does a full cycle up and down
		   	 
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
	void WaitAndGetData(int ms);
	void CheckAbort();

	void RunDummyData();
	QString m_sPort;
	Agent* m_pAgent = nullptr;

	// Sequences
	void Seq_StartWarning();
	void Seq_SwDev_A();
	void Seq_Calib_A();
	void Seq_EnduranceTestOneAngle();
	void Seq_RotorLifeTest_MotorOn();
	void Seq_Calib_B_PWM();
	void Seq_Study_at_Small_degree();

};

