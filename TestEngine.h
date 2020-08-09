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

	void Start();
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
	void RunTest();
};

