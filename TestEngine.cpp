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

// Angle of attack: Convert from PWM to Degrees 
float TestEngine::ConvPwmToDegree(float fPwmAOA)
{
	float fDegree;
	fDegree = fPwmAOA * m_fRotorConstSlope + m_fRotorConst0Intcerpt;
	return fDegree;
}

// Angle of attack: Convert from PWM to Degrees 
float TestEngine::ConvDegreeToPwm(float fDegreeAOA)
{
	
	float fPwm;
	fPwm = (fDegreeAOA - m_fRotorConst0Intcerpt) / m_fRotorConstSlope;
	return fPwm;

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
		RunSequence();
	}
	catch (const QString& sMsg)
	{
		LOG("EXCEPTION: " + sMsg);
		emit Error(sMsg);
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

	// Iteration of the Angle Of Attack 
	while (m_fAngleAtStartOfTestDegree < m_fAngleAtEndOfTestDegree) {
		
		agent.SetPitch(ConvDegreeToPwm(m_fAngleAtStartOfTestDegree));

		for (int i = 0; i < m_iTimeSpentAtAOA; i + 200){ /// 5 Readings per sec
			data = agent.GetData();
			Wait(200);
		}
		m_fAngleAtStartOfTestDegree++;
	}

	LOG("Sequence Closing");
	agent.Close();
}