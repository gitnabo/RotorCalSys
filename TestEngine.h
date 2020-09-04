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

	// Rotor Calibration Constants
	const float m_fAngleAtStartOfTestDegree = 1.0f;  // ! Should Be -1    
	const float m_fAngleAtEndOfTestDegree = 12.0f;	// ! Should Be 13		 
	const int   m_iTimeSpentAtAOA = 15000; // TEMP 30000 -> 1000  for TS      
	const int m_iSampleMs = 1000;
	const int m_iDelayForMotorRPM = 20000;
	const int m_iMotorRPM = 2960;

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


};

