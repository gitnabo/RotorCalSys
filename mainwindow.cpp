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
	m_pTestEngine->Stop();
	UpdateControls();
}

void MainWindow::OnStopped()
{
	// Outputs Avg Lift at each Angle of Attack measured, 
	QVector<QPointF> vRotorSetPointAvg; // x = fDegreeSet & y = fAvgLift	   
	for (int i = 0; i < m_listSetpointSamples.size(); i++) { 		
		QPointF pointfRotorSetPointAvg(m_listSetpointSamples.at(i).fDegree,					    // x = fDegreeSet &
				                       CaclLiftAvgKg(m_listSetpointSamples.at(i).vectSamples));// y = fAvgLift
		
		vRotorSetPointAvg.append(pointfRotorSetPointAvg);
	}

	// Calculate Rotor Calibration Slope and Intc
	QVector<float> vfLinearRegression = LinearRegression(vRotorSetPointAvg);

	
	ui->lineEdit_Slope->setText(QString::number(vfLinearRegression.at(0)));
	ui->lineEdit_Intercept->setText(QString::number(vfLinearRegression.at(1)));
	
	CreateTelFile(vfLinearRegression, CaclServoNeutralOffsetDeg(vfLinearRegression));
	   	   
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
	pChart->setAnimationOptions(QChart::NoAnimation); 
	pChart->setTitle("Load Cell % ");
	QValueAxis *axisX = new QValueAxis;
	axisX->setTickCount(10);
	pChart->addAxis(axisX, Qt::AlignBottom);
	m_pLineSeries = new QLineSeries; /// Bc this is a pointer
	pChart->addSeries(m_pLineSeries);

	QValueAxis *axisY = new QValueAxis;
	axisY->setRange(0, 100); 
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
	QPointF ptF(m_vectData.count() - 1, data.fLoadCellKg);
	*m_pLineSeries << ptF;

	if (m_pLineSeries->count() > 500)
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

float MainWindow::CaclLiftAvgKg(QVector<Agent::Data>  LiftDataKg) {
	float fSumOfLiftKg = 0.0f;
	for (int i = 1; i < LiftDataKg.size() - 1; i++) { // Start at 1 to eliminate the first and last
		fSumOfLiftKg += LiftDataKg.at(i).fLoadCellKg;
	}
	float fAvgOfLiftKg = fSumOfLiftKg / LiftDataKg.size();

	return fAvgOfLiftKg;
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

float MainWindow::CaclServoNeutralOffsetDeg(QVector<float> vfLinearRegressionPara) {	
	float fAoaOffsetDeg = (((m_fRotorStandardLiftCurveSlope * m_fAngleOfStudyAoaDeg) + m_fRotorStandardLiftCurveInct - vfLinearRegressionPara.at(1)) / vfLinearRegressionPara.at(0)) - m_fAngleOfStudyAoaDeg;
						// X2 - X1 = (((A1 * X1) + B1 - B2) / A2) - X1
						// X is Deg AoA & Y is Lift
		
	float fServoOffsetPwm = Agent::ConvDegreeToPwm(fAoaOffsetDeg);
	float fServoOffsetDeg = Agent::ConvPwmToServoDeg(fServoOffsetPwm); 

	return fServoOffsetDeg;
}

void MainWindow::CreateTelFile(QVector<float> vfLinearRegressionPara, float fCaclServoNeutralOffsetDeg) {
	// Tel File Location
	QString sFileLocation = "C:/Dev/Output Files/";	

	// Tel File Name
	QString sRotorNum = ui->lineEdit_RotorNum->text();
	if (sRotorNum.isEmpty()) {
		sRotorNum = "NA";
	}
	QDateTime DateTime = QDateTime::currentDateTime();
	QString sDateTimeFileFormat = "yyyyMMdd_hhmmss";
	QString sDateTime = DateTime.toString(sDateTimeFileFormat);
	QString sFileName = sFileLocation + "R_" + sRotorNum + "_D_" + sDateTime + ".csv";
	QFile file(sFileName);

	// Rotor Calibration Constants For Telemetry File
	if (!file.open(QIODevice::WriteOnly)) {
		return;
	}
	QTextStream stream(&file);
	
	// Parse Rotor Calibration Calculations
	stream << "ROTOR CALIBRATION SYSTEM - CALCULATIONS" << endl << endl;
	#pragma region Rotor Calibration Constants - Line 1		
	stream << "Rotor #" << "," << "ServoNeutralOffsetDeg" << ","
		   << "Cal Rotor Slope" << "," << "Cal Rotor Intc" << "," 
		   << "Rotor Rev" << "," << "Test Date" << "," 
		   << "Test Time" << endl;
	// Parse Rotor Calibration Constants
	stream << ui->lineEdit_RotorNum->text() << "," << QString::number(fCaclServoNeutralOffsetDeg) << ","
		   << QString::number(vfLinearRegressionPara.at(0)) << "," << QString::number(vfLinearRegressionPara.at(1)) << "," 
		   << m_sRotorRevision << "," << DateTime.toString("yyyy MM d") << "," 
		   << DateTime.toString("hh:mm:ss") << endl << endl << endl;
	#pragma endregion

	// Parse Rotor Calibration Constants
	stream <<"ROTOR CALIBRATION SYSTEM - CONSTANTS" << endl << endl;
	#pragma region Rotor Calibration Constants - Line 2		
	stream << "Pwm to Deg Aoa Slope" << "," << "Pwm to Deg Aoa Intc" << "," 
		   << "Standard Lift Curve Slope" << "," << "Standard Lift Curve Intc" << ","
		   << "Load Cell Gain Slope" << "," << "Load Cell Gain Intc" << endl;
	// Parse Rotor Calibration Constants
	stream << QString::number(m_fNEWRotorPwmToDegAoaSlope) << "," << QString::number(m_fNEWRotorPwmToDegAoaIntc) << ","
		   << QString::number(m_fRotorStandardLiftCurveSlope) << "," << QString::number(m_fRotorStandardLiftCurveInct) << ","
			<< QString::number(m_fLoadCellGainSlope) << "," << QString::number(m_fLoadCellGainIntc) << endl << endl;
	#pragma endregion
		
	#pragma region Rotor Calibration Constants - Line 3
	// Create Header for Rotor Calibration Constants - Line 2
	stream << "Angle Start (deg)" << "," << "Angle End (deg)" << ","
		    << "Time at Angle (ms)" << "," << "Sample Rate (ms)" << endl;
	// Parse Rotor Calibration Constants
	stream << QString::number(m_pTestEngine->m_fAngleAtStartOfTestDegree) << "," << QString::number(m_pTestEngine->m_fAngleAtEndOfTestDegree) << ","
		<< QString::number(m_pTestEngine->m_iTimeSpentAtAOA) << "," << QString::number(m_pTestEngine->m_iSampleMs) << endl << endl;
	#pragma endregion

	#pragma region Rotor Calibration Constants - Line 4
	// Create Header for Rotor Calibration Constants - Line 3
	stream << "Motor Const Slope" << "," << " Motor Const Inct" << ","
			<< "Motor Delay (ms)" << "," << "_" << endl;
	// Parse Rotor Calibration Constants
	stream << QString::number(m_fMotorConstSlope) << "," << QString::number(m_fMotorConstInct) << ","
			<< QString::number(m_pTestEngine->m_iDelayForMotorRPM) << "," << "_ _ _" << endl << endl << endl;
	#pragma endregion

	// Parse Rotor Calibration Telemetry Data
	stream << "ROTOR CALIBRATION SYSTEM - TELEMETRY DATA" << endl << endl;
	// Create Header for Telemetry Data 
	stream << "Time (ms)" << "," << "Load Cell (Kg)" << "," << "Servo Cur (mA)" << ","	<< "Servo Volt (V)" << "," 
		   << "Motor Cur (A)" << "," << "Motor Vol (V)" << "," << "Servo Pos (us)" << "," <<  " EST. Servo Pos (Deg)"
		   << "Motor Speed (us)" << "," << " Motor Speed (Rpm)" << endl;
	// Parse Telemetry Data
	for (int i = 0; i < m_vectData.size(); i++) {
		stream << m_vectData[i].iSampleMs << "," <<  m_vectData[i].fLoadCellKg << "," << m_vectData[i].fServoCurrent << "," << m_vectData[i].fServoVoltage << ","
			   << m_vectData[i].fMotorControllerCurrent << "," << m_vectData[i].fMotorControllerVoltage << "," << m_vectData[i].fServoPosPwm << "," << m_vectData[i].fServoPosDegEstimate << ","
		   	   << m_vectData[i].fMotorSpeedPwm << "," << m_vectData[i].fMotorSpeedRpmData << endl;
	}
}