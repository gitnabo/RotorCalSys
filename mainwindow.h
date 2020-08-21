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
	void OnNewPitch(float fDegrees);
	void OnLog(QString sMsg);

private:
	QLineSeries* m_pLineSeries = nullptr;
	void CreateChart();
	void ResetChart();
	void SaveSettings();
	void LoadSettings();
    Ui::MainWindow *ui;
	TestEngine *m_pTestEngine = nullptr;
	QTimer* m_pTimerUpdate = nullptr;
	
	// Stores load force at a specific degree
	QVector<Agent::Data> m_vectData;	///< The simple way to store all our samples, used for charting
	

	int iRotorSerialNumTEMP;

	struct SetPoint {
		float fDegree = 0.0f;
		QVector<Agent::Data> vectSamples;
		};
	
	QList<SetPoint> m_listSetpointSamples;	///< A more structured way to save our data. Organized by 'setpoint/degree'.
	

	float CaclLiftAvgLbs(QVector<Agent::Data>  LiftDataLbs);
	   

	QVector<QPointF> m_vRotorSetPointAvg; // x = fDegreeSet &
										  // y = fAvgLift
	   
	QVector<float> LinearRegression(QVector<QPointF> data); // (Slope, Intercept)
			

	struct RotorCalibration {
		int RotorSerialNum;
		QVector<float> vfLinearRegression;// (Slope, Intercept)
	};   	 
};