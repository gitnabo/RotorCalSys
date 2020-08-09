#include "TestEngine.h"
#include <QElapsedTimer>




TestEngine::TestEngine(QObject *parent)
	: QThread(parent)
{
}


TestEngine::~TestEngine()
{
}


void TestEngine::Start()
{
	m_bStopRequest = false;
	start();
}

void TestEngine::Stop()
{
	m_bStopRequest = true;
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

void TestEngine::run()
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

	Wait(5000);

	LOG("Goodbye");
}