#include "TestEngine.h"
#include <QElapsedTimer>
#include "Agent.h"




TestEngine::TestEngine(QObject *parent)
	: QThread(parent)
{
}


TestEngine::~TestEngine()
{
}


void TestEngine::Start(const QString& sPort)
{
	m_bStopRequest = false;
	m_sPort = sPort;
	start();
}

void TestEngine::Stop()
{
	m_bStopRequest = true;

	while (IsRunning())
	{
		QThread::msleep(50);
	}
}

bool TestEngine::IsRunning()
{
	return isRunning();
}


void TestEngine::LOG(QString sMsg)
{
	emit Log(sMsg);
}


void TestEngine::CheckAbort()
{
	if (m_bStopRequest)
		throw QString("abort request from user");
}

void TestEngine::Wait(int iMs)
{
	QElapsedTimer timer;
	timer.start();
	while (true)
	{
		if (timer.elapsed() > iMs)
			break;	// Done

		if (m_bStopRequest)
			throw QString("abort request from user");
		msleep(1);
	}
}

void TestEngine::run() /// Entry Point
{
	emit Started();
	try
	{
		RunTest();
	}
	catch (const QString& sMsg)
	{
		LOG("EXCEPTION: " + sMsg);
		emit Error(sMsg);
	}
	emit Stopped();
}



void TestEngine::RunTest()
{
	LOG("Hello");

	// Setup the test agent for communications
	Agent agent;
	agent.Open(m_sPort);

	// Read in a loop infinitely
	/// This is temp until Agent is completed
	while (true)
	{
		Agent::Data data = agent.GetData();
		agent.SetPitch(1280);
		agent.SetPitch(1680);
		Wait(400);
	}

	LOG("Goodbye");

	agent.Close();
}