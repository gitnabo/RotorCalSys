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