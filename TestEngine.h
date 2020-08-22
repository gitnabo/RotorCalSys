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

	void SeqStartWarning();
	void RunSequence();


	void RunDummyData();
	QString m_sPort;

	// Rotor Calibration Constants
	float m_fAngleAtStartOfTestDegree = 1;  // ! Should Be -1    
	float m_fAngleAtEndOfTestDegree   = 3;	// ! Should Be 13		 
	int   m_iTimeSpentAtAOA           = 500; // TEMP 30000 -> 1000  for TS      
	const int m_iSampleMs = 250;
};

