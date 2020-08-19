#include "pch.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QSettings>
#include <QMessageBox>
#include <QPointF>

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
	TestLinearRegresssion();
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

	// Also add to our more organized setpoint storage method
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


void MainWindow::TestLinearRegresssion()
{
	QVector<QPointF> points;
	points += QPointF(43, 99);
	points += QPointF(21, 65);
	points += QPointF(25, 79);
	points += QPointF(42, 75);
	points += QPointF(57, 87);
	points += QPointF(59, 81);

	QVector<float> coeffs = LinearRegression(points);
}

/// Example stolen from
/// https://www.statisticshowto.com/probability-and-statistics/regression-analysis/find-a-linear-regression-equation/
QVector<float> MainWindow::LinearRegression(QVector<QPointF> data)
{
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
	float fB = (fSumY * fSumXX - fSumX * fSumXY)
			/ (fN*fSumXX - fSumX * fSumX);
	float fM = (fN * fSumXY - fSumX * fSumY)
			/ (fN*fSumXX - fSumX * fSumX);
	QVector<float> vectCoeffs;
	vectCoeffs += fB;
	vectCoeffs += fM;
	return vectCoeffs;
}