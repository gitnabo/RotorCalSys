#include "pch.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QSettings>
#include <QMessageBox>

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
		<< "ASphincterSaysWhat?"
		<< ""
		<< "Istvan is watching you"
		<< ""
		<< "Hey momo"
		<< ""
		<< ""
		<< "Don't try this at home kids!"
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
	ResetChart();
	ui->pteLog->setPlainText("");
	QString sSerialPortName = ui->cbSerialPorts->currentText();
	m_pTestEngine->Start(sSerialPortName);
	UpdateControls();
}

void MainWindow::on_pbStop_clicked()
{
	m_pTestEngine->Stop();
	UpdateControls();
}


void MainWindow::OnStopped()
{
	
	// Outputs the average lift at each Angle of Attack measured, 
	// and stores in m_listRotorSetPointAvg
	for (int i = 0; i < m_listSetpointSamples.size(); i++) { // 		
		/*RotorSetPointAvg rotorSetPointAvg = { m_listSetpointSamples.at(i).fDegree,
											  CaclLiftAvgLbs(m_listSetpointSamples.at(i).vectSamples)};	*/ // This doesn't either
		RotorSetPointAvg rotorSetPointAvg;
		rotorSetPointAvg.fDegreeSet = m_listSetpointSamples.at(i).fDegree;
		rotorSetPointAvg.fAvgLift = CaclLiftAvgLbs(m_listSetpointSamples.at(i).vectSamples);

		m_listRotorSetPointAvg += rotorSetPointAvg; /// Append to list
	}; 


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
	m_listSetpointSamples += sp;
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
		m_listSetpointSamples.last().vectSamples += data;
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