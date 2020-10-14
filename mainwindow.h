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
	QVector<Agent::Data> m_vectData;	

	// Organize degree set point and all readings from load cell at that degree set point 
	struct SetPoint {
		float fDegree = 0.0f;
		QVector<Agent::Data> vectSamples;
		};
	QList<SetPoint> m_listSetpointSamples;

	float CaclLiftAvgKg(QVector<Agent::Data>  LiftDataKg);	

	QVector<float> LinearRegression(QVector<QPointF> data); // (Slope, Intercept)			

	struct RotorCalibration {
		int RotorSerialNum;
		QVector<float> vfLinearRegression; // (Slope, Intercept)
	}; 
	float CaclServoNeutralOffsetDeg(QVector<float> vfLinearRegressionPara);

	void CreateTelFile(QVector<float> vfLinearRegressionPara, float fCaclServoNeutralOffsetDeg);
	
	// System Constants
	const QString m_sRotorRevision = "1.2";
	const float m_fRotorStandardLiftCurveSlope = 1.304605f; // Based on measurement of Carbon Fiber on old Rotor
	const float m_fRotorStandardLiftCurveInct = -0.201835f; // Based on measurement of Carbon Fiber on old Rotor
	const float m_fAngleOfStudyAoaDeg = 6.9f;       // Based on Carbon Fiber V1 On New rotor. 
		                                            // That should be the AoA when the servo at 1520 PWM and
		                                            // the servo will be horizontal 


	 
};