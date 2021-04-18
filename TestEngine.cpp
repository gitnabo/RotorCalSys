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
	start(); /// QThread Method which initiates an additional threads
}

void TestEngine::Stop()
{
	// To trigger: throw AbortException();
	m_bStopRequest = true;	
	// m_pAgent->SetMotorSpeedRPM(0);
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
	if (m_bStopRequest) {
		throw Exception("abort request from user");
	}
		
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
		// Seq_EnduranceTestOneAngle();
		Seq_Calib_A();
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


void TestEngine::WaitAndGetData(int ms) {
	QElapsedTimer tmr;
	tmr.start();
	while (tmr.elapsed() < ms)
	{
		Agent::Data data = m_pAgent->GetData();
		emit NewData(data);
		LOG("Load Cell Kg:" + QString::number(data.fLoadCellKg));
		Wait(m_iSampleMs);
	}
}


// Sequences

/*
void TestEngine::Seq_SwDev_A()
{
	LOG("Seq Opening: Seq_SwDev_A");

	// Setup the test agent for communications
	Agent agent;
	agent.Open(m_sPort);
	m_pAgent = &agent;


	// Start Engine
	agent.SetMotorSpeedRPM(0); // To turn on the ESC
	LOG("Motor Speed Set To 0");
	WaitAndGetData(1000); 
	agent.SetMotorSpeedRPM(2000);
	LOG("Motor Speed Set To 1000");
	WaitAndGetData(15000);

	// TEMP: for Engine Testing
	
	// WaitAndGetData(m_iDelayForMotorRPM); // Delay for Motor RPM
	WaitAndGetData(2000);

	// Iteration of the Angle Of Attack 
	float fDegree = m_fAngleAtStartOfTestDegree;
	QElapsedTimer tmrMs;
	tmrMs.start();
	for (fDegree; fDegree <= m_fAngleAtEndOfTestDegree; fDegree =+ fDegree + 6) {
		agent.SetPitch(fDegree);
		emit NewPitch(fDegree);
		QString sLogMsg = "Angle of Attack:" + QString::number(fDegree);
		LOG(sLogMsg);		
		Wait(m_iSampleMs); //  To give time for the rotor to reach angle 
		// Gather data for a little while
		QElapsedTimer tmr;
		tmr.start();
		while (tmr.elapsed() < m_iTimeSpentAtAOA)
		{
			Agent::Data data = m_pAgent->GetData();
			data.iSampleMs = tmrMs.elapsed();
			emit NewData(data);
			int iRemainingMs = m_iSampleMs - tmr.elapsed();
			iRemainingMs = qMax(iRemainingMs, 0);	// Not less than zero
			Wait(iRemainingMs);
		}
	}	
	

	agent.SetMotorSpeedRPM(0);
	LOG("Seq Closing: Seq_SwDev_A");
	m_pAgent->Close();
}
*/

void TestEngine::Seq_Calib_A()
{
	LOG("Seq Opening: Seq_Calib_A");

	// Setup the test agent for communications
	Agent agent;
	m_pAgent = &agent;/// This is just to have access outside of this sequence
	agent.Open(m_sPort);
	// Agent::Data data;

	// Start Engine
	m_pAgent->SetPitch(0);
	Wait(2000);
	m_pAgent->SetPitch(8);
	Wait(2000);
	m_pAgent->SetPitch(0);
	Wait(2000);
	m_pAgent->SetMotorSpeedRPM(0); // To turn on the ESC
	Wait(1000); // Delay for the motor cmd to reach ESC

	m_pAgent->SetMotorSpeedRPM(m_iMotorRPM); // Set Motor to RPM
	QString sLogMsg = "Motor Speed Set To:" + QString::number(m_iMotorRPM);
	LOG(sLogMsg);
	Wait(m_iDelayForMotorRPM); // Delay for Motor RPM

	// Iteration of the Angle Of Attack 
	float fDegree = m_fAngleAtStartOfTestDegree;
	QElapsedTimer tmrMs;
	tmrMs.start();
	for (fDegree; fDegree <= m_fAngleAtEndOfTestDegree; fDegree++) {
		sLogMsg = "Angle of Attack:" + QString::number(fDegree);
		LOG(sLogMsg);
		m_pAgent->SetPitch(fDegree);
		emit NewPitch(fDegree);
		Wait(m_iSampleMs); //  To give time for the rotor to reach angle 
		// Gather data for a little while	

		QElapsedTimer tmr;
		tmr.start();
		while (tmr.elapsed() < m_iTimeSpentAtAOA)
		{
			// sLogMsg = "Get Data - TestEngine Line 206:";
			// LOG(sLogMsg);
			Agent::Data data = m_pAgent->GetData();
			data.iSampleMs = tmrMs.elapsed();
			emit NewData(data);

			// Display Lift
			sLogMsg = QString::number(data.fLoadCellKg) + "kg ; " + QString::number(data.fMotorControllerCurrent) + "A ; " + QString::number(data.fMotorControllerVoltage) + "V ; " + 
					  QString::number(data.fMotorControllerCurrent * data.fMotorControllerVoltage) + "W";
			LOG(sLogMsg);
			int iRemainingMs = m_iSampleMs - tmr.elapsed();
			iRemainingMs = qMax(iRemainingMs, 0);	// Not less than zero
			Wait(iRemainingMs);
		}
	}
	   
	// Shut Down System
	agent.SetMotorSpeedRPM(0);
	m_pAgent->SetPitch(1);	
	LOG("Sequence Closing");
	agent.Close();
}


void TestEngine::Seq_EnduranceTestOneAngle()
{
	LOG("Seq Opening: Seq_EnduranceTestOneAngle");

	// Setup the test agent for communications
	Agent agent;
	m_pAgent = &agent;/// This is just to have access outside of this sequence
	agent.Open(m_sPort);
	
	// Agent::Data data;

	// Start Engine
	m_pAgent->SetPitch(8);
	m_pAgent->SetPitch(0);
	m_pAgent->SetPitch(1);
	m_pAgent->SetMotorSpeedRPM(0); // To turn on the ESC
	Wait(1000); // Delay for the motor cmd to reach ESC

	m_pAgent->SetMotorSpeedRPM(m_iMotorRPM); // Set Motor to RPM
	QString sLogMsg = "Motor Speed Set To:" + QString::number(m_iMotorRPM);
	LOG(sLogMsg);
	Wait(m_iDelayForMotorRPM); // Delay for Motor RPM

	// Set Angle Of Test
	float fDegree = 6;
	int iTestDurationMs = 3600000*3; // 3,600,000 is 1hr
	QElapsedTimer tmrMs;
	tmrMs.start();
	while (tmrMs.elapsed() < iTestDurationMs){
		sLogMsg = "Angle of Attack:" + QString::number(fDegree);
		LOG(sLogMsg);
		m_pAgent->SetPitch(fDegree);
		emit NewPitch(fDegree);
		Wait(m_iSampleMs); //  To give time for the rotor to reach angle 
		// Gather data for a little while	

		QElapsedTimer tmr;
		tmr.start();
		while (tmrMs.elapsed() < iTestDurationMs)
		{
			// sLogMsg = "Get Data - TestEngine Line 206:";
			// LOG(sLogMsg);
			Agent::Data data = m_pAgent->GetData();
			data.iSampleMs = tmrMs.elapsed();
			emit NewData(data);

			// Display Lift
			sLogMsg = QString::number(data.fLoadCellKg) + "kg ; " + QString::number(data.fMotorControllerCurrent) + "A ; " + QString::number(data.fMotorControllerVoltage) + "V ; " +
				QString::number(data.fMotorControllerCurrent * data.fMotorControllerVoltage) + "W";
			LOG(sLogMsg);

			int iRemainingMs = m_iSampleMs - tmr.elapsed();
			iRemainingMs = qMax(iRemainingMs, 0);	// Not less than zero
			Wait(iRemainingMs);
		}
	}

	// Shut Down System
	agent.SetMotorSpeedRPM(0);
	m_pAgent->SetPitch(1);
	LOG("Sequence Closing");
	agent.Close();
}


void TestEngine::Seq_RotorLifeTest_MotorOn()
{
	LOG("Seq Opening: Seq_RotorLifeTest_MotorOn");


	// Setup the test agent for communications
	Agent agent;
	m_pAgent = &agent;/// This is just to have access outside of this sequence
	agent.Open(m_sPort);
	// Agent::Data data;

	// Start Engine
	m_pAgent->SetPitch(0);
	m_pAgent->SetMotorSpeedRPM(0); // To turn on the ESC
	Wait(1000); // Delay for the motor cmd to reach ESC

	m_pAgent->SetMotorSpeedRPM(m_iMotorRPM); // Set Motor to RPM
	QString sLogMsg = "Motor Speed Set To:" + QString::number(m_iMotorRPM);
	LOG(sLogMsg);
	Wait(m_iDelayForMotorRPM); // Delay for Motor RPM

	// Iteration of the Angle Of Attack 
	float fDegree = m_fAngleAtStartOfTestDegree;
	QElapsedTimer tmrMs;
	tmrMs.start();
	
	int iCycleIndex = 0; 
	/// Angle Of Attack - Full Cycle Inc. to Dec.
	for (iCycleIndex; iCycleIndex <= m_iCycleNum; iCycleIndex++) {

		/// Angle Of Attack - Increasing
		for (fDegree; fDegree <= m_fAngleAtEndOfTestDegree; fDegree++) {
			m_pAgent->SetPitch(fDegree);
			emit NewPitch(fDegree);
			sLogMsg = "Angle of Attack:" + QString::number(fDegree);
			LOG(sLogMsg);
			Wait(m_iSampleMs); //  To give time for the rotor to reach angle 
			// Gather data for a little while	

			QElapsedTimer tmr;
			tmr.start();
			while (tmr.elapsed() < m_iTimeSpentAtAOA_LifeTest) // Time Spent at each angle is unique
			{
				Agent::Data data = m_pAgent->GetData();
				data.iSampleMs = tmrMs.elapsed();
				emit NewData(data);

				// Display Lift
				sLogMsg = QString::number(data.fLoadCellKg) + "kg ; " + QString::number(data.fMotorControllerCurrent) + "A ; " + QString::number(data.fMotorControllerVoltage) + "V ; " +
					QString::number(data.fMotorControllerCurrent * data.fMotorControllerVoltage) + "W";
				LOG(sLogMsg);
				int iRemainingMs = m_iSampleMs - tmr.elapsed();
				iRemainingMs = qMax(iRemainingMs, 0);	// Not less than zero
				Wait(iRemainingMs);
			}
		}

		/// Angle Of Attack - Decreasing
		for (fDegree; fDegree >= m_fAngleAtStartOfTestDegree; fDegree--) { // Changed m_fAngleAtStartOfTestDegree
			m_pAgent->SetPitch(fDegree);
			emit NewPitch(fDegree);
			sLogMsg = "Angle of Attack:" + QString::number(fDegree);
			LOG(sLogMsg);
			Wait(m_iSampleMs); //  To give time for the rotor to reach angle 
							   // Gather data for a little while	

			QElapsedTimer tmr;
			tmr.start();
			while (tmr.elapsed() < m_iTimeSpentAtAOA_LifeTest) // Time Spent at each angle is unique
			{
				Agent::Data data = m_pAgent->GetData();
				data.iSampleMs = tmrMs.elapsed();
				emit NewData(data);

				// Display Lift
				sLogMsg = QString::number(data.fLoadCellKg) + "kg ; " + QString::number(data.fMotorControllerCurrent) + "A ; " + QString::number(data.fMotorControllerVoltage) + "V ; " +
					QString::number(data.fMotorControllerCurrent * data.fMotorControllerVoltage) + "W";
				LOG(sLogMsg);
				int iRemainingMs = m_iSampleMs - tmr.elapsed();
				iRemainingMs = qMax(iRemainingMs, 0);	// Not less than zero
				Wait(iRemainingMs);
			}
		}
	}

	// Shut Down System
	agent.SetMotorSpeedRPM(0);
	m_pAgent->SetPitch(1);
	LOG("Sequence Closing");
	agent.Close();
}

/*
void TestEngine::Seq_Study_at_Small_degree()
{
	LOG("Seq Opening: Seq_Study_at_Small_degree");

	// Setup the test agent for communications
	Agent agent;
	m_pAgent = &agent;/// This is just to have access outside of this sequence
	agent.Open(m_sPort);
	// Agent::Data data;

	// Start Engine
	m_pAgent->SetPitch(-1);
	m_pAgent->SetMotorSpeedRPM(0); // To turn on the ESC
	WaitAndGetData(1000);

	m_pAgent->SetMotorSpeedRPM(m_iMotorRPM);
	QString sLogMsg = "Motor Speed Set To:" + QString::number(m_iMotorRPM);
	LOG(sLogMsg);

	WaitAndGetData(m_iDelayForMotorRPM);// ###MOD### m_iDelayForMotorRPM // Delay for Motor RPM

		// Iteration of the Angle Of Attack 
	float fDegree = -1; // ###  m_fAngleAtStartOfTestDegree
	QElapsedTimer tmrMs;
	tmrMs.start();
	for (fDegree; fDegree <= 3; fDegree = fDegree + 0.25) { //### m_fAngleAtEndOfTestDegree
										          	        //### fDegree++
		m_pAgent->SetPitch(fDegree);
		emit NewPitch(fDegree);
		sLogMsg = "Angle of Attack:" + QString::number(fDegree);
		LOG(sLogMsg);
		Wait(m_iSampleMs); //  To give time for the rotor to reach angle 
		// Gather data for a little while	

		QElapsedTimer tmr;
		tmr.start();
		while (tmr.elapsed() < m_iTimeSpentAtAOA)
		{
			Agent::Data data = m_pAgent->GetData();
			data.iSampleMs = tmrMs.elapsed();
			emit NewData(data);
			sLogMsg = "Load Cell Kg:" + QString::number(data.fLoadCellKg);
			LOG(sLogMsg);
			int iRemainingMs = m_iSampleMs - tmr.elapsed();
			iRemainingMs = qMax(iRemainingMs, 0);	// Not less than zero
			Wait(iRemainingMs);
		}
	}

	// Shut Down System
	m_pAgent->SetPitch(0);
	agent.SetMotorSpeedRPM(0);
	LOG("Sequence Closing");
	agent.Close();
}
*/