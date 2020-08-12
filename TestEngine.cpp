#include "TestEngine.h"
#include <QElapsedTimer>
#include "Agent.h"


class Exception : public QString {
public:
	Exception() {}
	Exception(QString sMsg)
		: QString(sMsg)
	{

	}
};

class AbortException : public Exception {
};



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
		throw Exception("abort request from user");
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
			throw AbortException();
		msleep(1);
	}
}

void TestEngine::run()
{
	emit Started();
	try
	{
		RunDummyData();
		RunTest();
	}
	catch (const AbortException&)
	{
		// Don't say anything about an abort
	}
	catch (const Exception& ex)
	{
		LOG("EXCEPTION: " + ex);
		emit Error(ex);
	}
	emit Stopped();
}


void TestEngine::RunDummyData()
{
	auto funcGen = []() {
		int iRnd = qrand();
		int iVal = iRnd % 100;
		return (float)iVal;
	};

	int i = 1;
	while (true)
	{
		QString sMsg = QString("%1 line").arg(i++);
		LOG(sMsg);

		Agent::Data data;
		data.fTime = funcGen();
		data.fLoadCell = funcGen(); /// Lbs
		data.fServoCurrent = funcGen(); /// mA
		data.fServoVoltage = funcGen(); /// V
		data.fMotorControllerCurrent = funcGen(); /// A
		data.fMotorControllerVoltage = funcGen(); /// V
		data.fServoPostion = funcGen(); /// us pulse width
		data.fMotorRpmSetting = funcGen(); /// us pulse width
		emit NewData(data);

		Wait(200);
	}
}

void TestEngine::RunTest()
{
	LOG("Hello");

	// Setup the test agent for communications
	Agent agent;
	agent.Open(m_sPort);

	// Read in a loop infinitely
	while (true)
	{
		Agent::Data data = agent.GetData();
		Wait(400);
	}

	LOG("Goodbye");

	agent.Close();
}