#include "pch.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QSettings>
#include <QMessageBox>
#include <QPointF>
#include <QFile.h>
#include <QTextStream.h>
#include <QDateTime.h>


MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	m_pTestEngine = new TestEngine(this);
	connect(m_pTestEngine, &TestEngine::Stopped, this, &MainWindow::OnStopped);
	connect(m_pTestEngine, &TestEngine::Error, this, &MainWindow::OnError);
	
	connect(m_pTestEngine, &TestEngine::Log, this, &MainWindow::OnLog);
	connect(m_pTestEngine, &TestEngine::NewData, this, &MainWindow::OnNewData);
	connect(m_pTestEngine, &TestEngine::NewPitch, this, &MainWindow::OnNewPitch);

	// Populate the serial port combo dialog
	QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
	for (QSerialPortInfo port : ports)
		ui->cbSerialPorts->addItem(port.portName());

	// Pull the initial values from settings
	LoadSettings();

	UpdateControls();

	m_pTimerUpdate = new QTimer(this);
	m_pTimerUpdate->setInterval(2 * 60000);
	m_pTimerUpdate->setSingleShot(false);
	connect(m_pTimerUpdate, &QTimer::timeout, this, &MainWindow::OnUpdateTimer);
	m_pTimerUpdate->start();
	setWindowTitle("Rotor Calibrator");
	CreateChart();
}

MainWindow::~MainWindow()
{
	on_pbStop_clicked();
    delete ui;
}


void MainWindow::OnUpdateTimer()
{
	static QStringList slTitles = QStringList()
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		<< ""
		;
	static int iPos = 0;

	// Build a title 
	QString sMsg = slTitles[iPos++];
	if (iPos >= slTitles.count())
		iPos = 0;	// wraparound

	QString sTitle = "Rotor Calibrator";

	if (!sMsg.isEmpty())
		sTitle += " - " + sMsg;

	setWindowTitle(sTitle);
}


void MainWindow::LoadSettings()
{
	QSettings settings("GeoScout", "RotorSysCal");
	int iPortSetting = settings.value("SerialPort").toInt();

	// Apply to combo box
	for (int i = 0; i < ui->cbSerialPorts->count(); ++i)
	{
		QString s = ui->cbSerialPorts->itemText(i);
		s.remove("COM");
		int iPort = s.toInt();
		if (iPort == iPortSetting)
		{
			ui->cbSerialPorts->setCurrentIndex(i);
			break;
		}
	}
}

void MainWindow::SaveSettings()
{
	QSettings settings("GeoScout", "RotorSysCal");

	// Find the selected port
	QString sCom = ui->cbSerialPorts->currentText();
	sCom.remove("COM");
	int iPort = sCom.toInt();
	settings.setValue("SerialPort", iPort);
}

void MainWindow::on_cbSerialPorts_currentIndexChanged(int /*iIndex*/)
{
	SaveSettings();
}

void MainWindow::UpdateControls()
{
	bool bRunning = m_pTestEngine->IsRunning();
	ui->cbSerialPorts->setEnabled(!bRunning);
	ui->pbStart->setEnabled(!bRunning);
	ui->pbStop->setEnabled(bRunning);
}


void MainWindow::on_pbStart_clicked()
{
	m_vectData.clear();

	//Clear the slope and intercept output
	ui->lineEdit_Slope->clear();
	ui->lineEdit_Intercept->clear();
	ResetChart();
	ui->pteLog->setPlainText("");
	QString sSerialPortName = ui->cbSerialPorts->currentText();
	m_pTestEngine->Start(sSerialPortName);
	UpdateControls();
}

void MainWindow::on_pbStop_clicked()
{
	// Stops the motor
	Agent agent;
	agent.SetMotorSpeed(1000);

	m_pTestEngine->Stop();
	UpdateControls();
}

void MainWindow::OnStopped()
{
	CreateTelFile(); // TEMP


	// Outputs Avg Lift at each Angle of Attack measured, 
	QVector<QPointF> vRotorSetPointAvg; // x = fDegreeSet & y = fAvgLift	   
	for (int i = 0; i < m_listSetpointSamples.size(); i++) { 		
		QPointF pointfRotorSetPointAvg(m_listSetpointSamples.at(i).fDegree,					    // x = fDegreeSet &
				                       CaclLiftAvgLbs(m_listSetpointSamples.at(i).vectSamples));// y = fAvgLift
		
		vRotorSetPointAvg.append(pointfRotorSetPointAvg);
	}

	// Outputs the RotorCalibration
	QVector<float> vfLinearRegression = LinearRegression(vRotorSetPointAvg);
		
	ui->lineEdit_Slope->setText(QString::number(vfLinearRegression.at(0)));
	ui->lineEdit_Intercept->setText(QString::number(vfLinearRegression.at(1)));

	   
	UpdateControls();
}


void MainWindow::OnError(QString sMsg)
{
	QMessageBox::critical(this, "Error", sMsg);
}

void MainWindow::OnLog(QString sMsg)
{
	ui->pteLog->appendPlainText(sMsg);
}


void MainWindow::CreateChart()
{
	QChart *pChart = ui->wChartView->chart();
	pChart->setTitle("Data Chart");
	QValueAxis *axisX = new QValueAxis;
	axisX->setTickCount(10);
	pChart->addAxis(axisX, Qt::AlignBottom);
	m_pLineSeries = new QLineSeries;
	pChart->addSeries(m_pLineSeries);

	QValueAxis *axisY = new QValueAxis;
	axisY->setLinePenColor(m_pLineSeries->pen().color());

	pChart->addAxis(axisY, Qt::AlignLeft);
	m_pLineSeries->attachAxis(axisX);
	m_pLineSeries->attachAxis(axisY);
}

void MainWindow::ResetChart()
{
	//QChart *pChart = ui->wChartView->chart();
	m_pLineSeries->clear();
}


void MainWindow::OnNewPitch(float fDegrees)
{
	SetPoint sp;
	sp.fDegree = fDegrees;
	m_listSetpointSamples += sp; /// Samples as in data, not as in example data
}

void MainWindow::OnNewData(Agent::Data data)
{
	m_vectData += data;

	QChart *pChart = ui->wChartView->chart();
	pChart->removeSeries(m_pLineSeries);
	QPointF ptF(m_vectData.count() - 1, data.fLoadCell);
	*m_pLineSeries << ptF;

	if (m_pLineSeries->count() > 50)
		m_pLineSeries->remove(0);
	pChart->addSeries(m_pLineSeries);

	// Also add to our more organized set point storage method
	if (!m_listSetpointSamples.isEmpty())
	{
		m_listSetpointSamples.last().vectSamples += data; /// This is where the tel db is updated
	}
	/*for (SetPoint& sp : m_listSetpointSamples)
	{
		// Process this one setpoint
		for (Agent::Data& data : sp.vectSamples)
		{

		}
	}*/
}

float MainWindow::CaclLiftAvgLbs(QVector<Agent::Data>  LiftDataLbs) {
	float fSumOfLiftLbs = 0.0f;
	for (int i = 0; i < LiftDataLbs.size(); i++) {
		fSumOfLiftLbs += LiftDataLbs.at(i).fLoadCell;
	}
	float fAvgOfLiftLbs = fSumOfLiftLbs / LiftDataLbs.size();

	return fAvgOfLiftLbs;
}

QVector<float> MainWindow::LinearRegression(QVector<QPointF> data)
{
	/// Example from
	/// https://www.statisticshowto.com/probability-and-statistics/regression-analysis/find-a-linear-regression-equation/
	float fSumX = 0.0f;
	float fSumY = 0.0f;
	float fSumXY = 0.0f;
	float fSumXX = 0.0f;
	float fSumYY = 0.0f;
	for (QPointF pt : data)
	{
		fSumX += pt.x();
		fSumY += pt.y();
		fSumXY += pt.x() * pt.y();
		fSumXX += pt.x() * pt.x();
		fSumYY += pt.y() * pt.y();
	}

	float fN = data.count();

	// Slope
	float fM = (fN * fSumXY - fSumX * fSumY)
		/ (fN*fSumXX - fSumX * fSumX);

	// Intercept
	float fB = (fSumY * fSumXX - fSumX * fSumXY)
		/ (fN*fSumXX - fSumX * fSumX);

	QVector<float> vectCoeffs;
	vectCoeffs += fM;
	vectCoeffs += fB;	
	return vectCoeffs;
}

void MainWindow::CreateTelFile() {
	// Tel File Location
	QString sFileLocation = "C:/Dev/RotorCalSys/Output Files/";	

	// Tel File Name
	QString sRotorNum = ui->lineEdit_RotorNum->text();
	if (ui->lineEdit_RotorNum->text() == NULL) {
		sRotorNum = "NA";
	}
	QDateTime DateTime = QDateTime::currentDateTime();
	QString sDateTimeFileFormat = "yyyyMMdd_hhmmss";
	QString sDateTime = DateTime.toString(sDateTimeFileFormat);
	QString sFileName = sFileLocation + "R_" + sRotorNum + "_D_" + sDateTime + ".csv";
	QFile file(sFileName);

	// Rotor Calibration Constants For Telemetry File
	if (file.open(QIODevice::WriteOnly)) {	
		Agent agent;
		QTextStream stream(&file);
		
		#pragma region Create Header for Rotor Calibration Constants - Line 1		
		stream << "Rotor #" << "," << "Rotor Rev" << ","
			<< "Test Date" << "," << "Test Time" << endl;
		// Parse Rotor Calibration Constants
		stream << ui->lineEdit_RotorNum->text() << "," << QString::number(agent.m_fRotorRevision) << ","
			<< DateTime.toString("yyyy MM d") << "," << DateTime.toString("hh:mm:ss") << endl << endl;
		#pragma endregion

		#pragma region Create Header for Rotor Calibration Constants - Line 2		
		stream << "Def Rotor Const Slope" << "," << "Def Rotor Const Intc" << "," 
			   << "Load Cell Gain Slope" << "," << "Load Cell Gain Intc" << endl;
		// Parse Rotor Calibration Constants
		stream << QString::number(agent.m_fRotorConstSlope) << "," << QString::number(agent.m_fRotorConstIntc) << "," 
			   << QString::number(agent.m_fLoadCellGainSlope) << "," << QString::number(agent.m_fLoadCellGainIntc) << endl << endl;
		#pragma endregion
		
		#pragma region Create Header for Rotor Calibration Constants - Line 3
		// Create Header for Rotor Calibration Constants - Line 2
		stream << "Angle Start (deg)" << "," << "Angle End (deg)" << ","
		       << "Time at Angle (ms)" << "," << "Sample Rate (ms)" << endl;
		// Parse Rotor Calibration Constants
		stream << QString::number(m_pTestEngine->m_fAngleAtStartOfTestDegree) << "," << QString::number(m_pTestEngine->m_fAngleAtEndOfTestDegree) << ","
			<< QString::number(m_pTestEngine->m_iTimeSpentAtAOA) << "," << QString::number(m_pTestEngine->m_iSampleMs) << endl << endl;
		#pragma endregion

		#pragma region Create Header for Rotor Calibration Constants - Line 4
		// Create Header for Rotor Calibration Constants - Line 3
		stream << "Motor Const Slope" << "," << " Motor Const Inct" << ","
			   << "Motor Delay (ms)" << "," << "_" << endl;
		// Parse Rotor Calibration Constants
		stream << QString::number(agent.m_fMotorConstSlope) << "," << QString::number(agent.m_fMotorConstInct) << ","
			   << QString::number(m_pTestEngine->m_iDelayForMotorRPM) << "," << "_ _ _" << endl << endl << endl;
		#pragma endregion

		// Create Header for Telemetry Data 
		stream << "Time (ms)" << "," << "Load Cell (Lbs)" << "," << "Servo Cur (mA)" << ","	<< "Servo Volt (V)" << "," 
			   << "Motor Cur (A)" << "," << "Motor Vol (V)" << "," << "Servo Pos (us)" << "," << "Motor Speed (us)" << endl;
		// Parse Telemetry Data
		for (int i = 0; i < m_vectData.size(); i++) {
			stream << m_vectData[i].fTime << "," <<  m_vectData[i].fLoadCell << "," << m_vectData[i].fServoCurrent << "," << m_vectData[i].fServoVoltage << ","
				   << m_vectData[i].fMotorControllerCurrent << "," << m_vectData[i].fMotorControllerVoltage << "," << m_vectData[i].fServoPos << "," << m_vectData[i].fMotorSpeed << "," << endl;
		}
	}
	
	


}