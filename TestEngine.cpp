#include "TestEngine.h"
#include <QElapsedTimer>
#include "Agent.h"
#include "Exception.h"
#include <QDebug>




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
	qDebug() << sMsg;
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



void TestEngine::run() /// Entry Point
{
	emit Started();
	float fTestPwm   = 1350; /// Should be 1.7degrees
	float fFoo = ConvPwmToDegree(fTestPwm);

	float fTestDegree = 6.4; /// Should be 1550pwm
	float fMoo = ConvDegreeToPwm(fTestDegree);


	try
	{
		LOG("Sequence is Opening");
		//RunSequence();	
		// Troubleshooting 
		Agent agent;
		agent.Open(m_sPort);
		Agent::Data data;

		data = agent.GetData();
		agent.SetPitch(1280);
		agent.SetPitch(1500);
		agent.SetPitch(1280);
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

void TestEngine::RunSequence()
{
	LOG("Sequence Opening");

	// Setup the test agent for communications
	Agent agent;
	agent.Open(m_sPort);		
	Agent::Data data;
	
	// ToDo: Add a warning Sequence

	const int iSampleMs = 200;
	int iSamplesPerSetpoint = m_iTimeSpentAtAOA / iSampleMs;

	// Iteration of the Angle Of Attack 
	float fDegree = m_fAngleAtStartOfTestDegree;
	while (fDegree < m_fAngleAtEndOfTestDegree) {
		
		agent.SetPitch(fDegree);
		emit NewPitch(fDegree);

		for (int i = 0; i < iSamplesPerSetpoint; ++i){
			data = agent.GetData();
			emit NewData(data);
			Wait(iSampleMs);
		}
		fDegree++;
	}

	LOG("Sequence Closing");
	agent.Close();
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
		//data.fServoPostion = funcGen(); /// us pulse width
		//data.fMotorRpmSetting = funcGen(); /// us pulse width
		emit NewData(data);

		Wait(200);
	}
}