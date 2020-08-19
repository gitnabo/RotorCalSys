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
	


	struct SetPoint {
		float fDegree = 0.0f;
		QVector<Agent::Data> vectSamples;
		};

	QList<SetPoint> m_listSetpointSamples;	///< A more structured way to save our data. Organized by 'setpoint/degree'.
	


	struct RotorSetPointAvg {
		float fDegreeSet = 0.0f;
		float fAvgLift = 0.0f;
	};

	QList<RotorSetPointAvg> m_listRotorSetPointAvg;



	float CaclDegreeAvg(SetPoint);



	struct RotorCalibration {
		int RotorSerialNum;
		float LiftCurveSlope;
		float LiftCurve0Intercept;
	};


};