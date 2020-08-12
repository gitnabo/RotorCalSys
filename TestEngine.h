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
	void NewData(Agent::Data data);
	void Log(QString sMsg);

private:
	void LOG(QString sMsg);
	volatile bool m_bStopRequest = false;
	virtual void run() override;	///< The thread function where 
	void Wait(int iMs);
	void CheckAbort();
	void RunTest();
	void RunDummyData();
	QString m_sPort;
};

