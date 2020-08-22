#include "pch.h"
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
	try
	{
		// Always have Warning Seq
		Seq_StartWarning;
		Seq_SwDev_A();	
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

void TestEngine::Seq_StartWarning()
{
	LOG("Seq Opening: Seq_StartWarning");
	LOG("!!! Warning Sequence Starting !!!");
	LOG("!!! Stand Clear of the Rotor System !!!");

	// Setup the test agent for communications
	Agent agent;
	agent.Open(m_sPort);
	Agent::Data data;

	
	// Iteration of the Angle Of Attack 
	float fDegreeStart      = -3;
	float fDegreeEnd        = 12;
	float fDegreeStep       = 0.5; 
	float fDegreeUpdateTime = 250; // ms

	// Cycle Up
	float fDegree = fDegreeStart;
	for (fDegree; fDegree <= fDegreeEnd; fDegree + fDegreeStep) {
		agent.SetPitch(fDegree);
		emit NewPitch(fDegree);
		Wait(fDegreeUpdateTime); 
	}
	LOG("!!! Warning Sequence Starting !!!");

	// Cycle Down
	float fDegree = fDegreeEnd;
	for (fDegree; fDegree >= fDegreeStart; fDegree - fDegreeStep) {
		agent.SetPitch(fDegree);
		emit NewPitch(fDegree);
		Wait(fDegreeUpdateTime);
	}
	LOG("!!! Warning Sequence Starting !!!");

	// Cycle Up
	float fDegree = fDegreeStart;
	for (fDegree; fDegree <= fDegreeEnd; fDegree + fDegreeStep) {
		agent.SetPitch(fDegree);
		emit NewPitch(fDegree);
		Wait(fDegreeUpdateTime);
	}
	LOG("!!! Warning Sequence Starting !!!");

	// Cycle Down
	float fDegree = fDegreeEnd;
	for (fDegree; fDegree >= fDegreeStart; fDegree - fDegreeStep) {
		agent.SetPitch(fDegree);
		emit NewPitch(fDegree);
		Wait(fDegreeUpdateTime);
	}


	LOG("Seq Closing: Seq_StartWarning");
	agent.Close();
}

void TestEngine::Seq_SwDev_A()
{
	LOG("Seq Opening: Seq_SwDev_A");

	// Setup the test agent for communications
	Agent agent;
	agent.Open(m_sPort);		
	Agent::Data data;

	int iSamplesPerSetpoint = m_iTimeSpentAtAOA / m_iSampleMs;

	// Iteration of the Angle Of Attack 
	float fDegree = m_fAngleAtStartOfTestDegree;
	for (fDegree; fDegree <= m_fAngleAtEndOfTestDegree; fDegree++) {	
		agent.SetPitch(fDegree);
		emit NewPitch(fDegree);
		QString sLogMsg = "Angle of Attack:" + QString::number(fDegree);
		LOG(sLogMsg);		
		for (int i = 0; i < iSamplesPerSetpoint; ++i){
			data = agent.GetData();
			emit NewData(data);
			Wait(m_iSampleMs);			
		}
	}

	LOG("Seq Closing: Seq_SwDev_A");
	agent.Close();
}

void TestEngine::Seq_Calib_A()
{
	LOG("Seq Opening: Seq_Calib_A");

	// Setup the test agent for communications
	Agent agent;
	agent.Open(m_sPort);
	Agent::Data data;
	
	// Start Engine
	agent.SetMotorSpeed(1000); // To turn on the ESC
	Wait(1000);
	agent.SetMotorSpeed(2010); // Speed for S48 Blades
	WaitAndGetData(m_iDelayForMotorRPM); // Delay for Motor RPM


	// Iteration of the Angle Of Attack 
	int iSamplesPerSetpoint = m_iTimeSpentAtAOA / m_iSampleMs;
	float fDegree = m_fAngleAtStartOfTestDegree;
	for (fDegree; fDegree <= m_fAngleAtEndOfTestDegree; fDegree++) {
		agent.SetPitch(fDegree);
		emit NewPitch(fDegree);
		QString sLogMsg = "Angle of Attack:" + QString::number(fDegree);
		LOG(sLogMsg);
		for (int i = 0; i < iSamplesPerSetpoint; ++i) {
			data = agent.GetData();
			emit NewData(data);
			Wait(m_iSampleMs);
		}
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

void TestEngine::WaitAndGetData(int ms) {
	// Setup the test agent for communications
	Agent agent;
	agent.Open(m_sPort);
	Agent::Data data;


	int iRateofDataGet = 10; // ms
	for (int i = 0; i < ms; i + iRateofDataGet) {
		data = agent.GetData();
		emit NewData(data);
		Wait(iRateofDataGet);
	}

}