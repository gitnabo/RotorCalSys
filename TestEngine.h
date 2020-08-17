#pragma once
#include <QThread>

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
	void NewData(float fRPM, float fV, float fLiftUNITS);
	void Log(QString sMsg);

private:
	void LOG(QString sMsg);
	volatile bool m_bStopRequest = false;
	virtual void run() override;	///< The thread function where 
	void Wait(int iMs);
	void CheckAbort();
	void RunSequence();
	QString m_sPort;

	float ConvPwmToDegree(float fPwmAOA); /// Angle of Attack
	float ConvDegreeToPwm(float fDegreeAOA); /// Angle of Attack
	float fDegreeAOA; /// Angle of Attack

	// Rotor Calibration Constants
	float m_fRotorConstSlope          =  0.024485714; // Updated on Aug 17 2020
	float m_fRotorConst0Intcerpt      = -31.46047619; // Updated on Aug 17 2020
	float m_fAngleAtStartOfTestDegree = -1;          // Updated on Aug 17 2020
	float m_fAngleAtEndOfTestDegree   = 12;			 // Updated on Aug 17 2020
	int   m_iTimeSpentAtAOA           = 30000;       // Updated on Aug 17 2020

};

