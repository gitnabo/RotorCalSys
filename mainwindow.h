#pragma once

#include <QWidget>
#include "TestEngine.h"
#include <QTimer>
#include <QVector>
#include <QtCharts>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
	void on_cbSerialPorts_currentIndexChanged(int iIndex);
	void UpdateControls();
	void on_pbStart_clicked();
	void on_pbStop_clicked();
	void OnStopped();
	void OnError(QString sMsg);
	void OnUpdateTimer();
	void OnNewData(Agent::Data data);
	void OnLog(QString sMsg);

private:
	QVector<Agent::Data> m_vectData;
	QLineSeries* m_pLineSeries = nullptr;
	void CreateChart();
	void ResetChart();
	void SaveSettings();
	void LoadSettings();
    Ui::MainWindow *ui;
	TestEngine *m_pTestEngine = nullptr;
	QTimer* m_pTimerUpdate = nullptr;
};