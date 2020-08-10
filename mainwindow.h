#pragma once

#include <QWidget>
#include "TestEngine.h"

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

private:
	void SaveSettings();
	void LoadSettings();
    Ui::MainWindow *ui;
	TestEngine *m_pTestEngine = nullptr;
};