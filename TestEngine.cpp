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
	float fFoo = ConvPwmToDegree(fTestPwm); // Test dewdwe

	float fTestDegree = 6.4; /// Should be 1550pwm
	float fMoo = ConvDegreeToPwm(fTestDegree); // grsege


	try
	{
		RunSequence();
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

	// Iteration of the Angle Of Attack 
	float fDegree = m_fAngleAtStartOfTestDegree;
	while (fDegree < m_fAngleAtEndOfTestDegree) {
		
		agent.SetPitch(ConvDegreeToPwm(fDegree));
		emit NewPitch(fDegree);

		for (int i = 0; i < m_iTimeSpentAtAOA; i + 200){ /// 5 Readings per sec
			data = agent.GetData();
			emit NewData(data);
			Wait(200);
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