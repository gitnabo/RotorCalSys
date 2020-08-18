#include "pch.h"
#include "mainwindow.h"
#include "Agent.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	QCoreApplication::setOrganizationName("GeoScout");
	QCoreApplication::setOrganizationDomain("geoscout.com");
	QCoreApplication::setApplicationName("RotorSysCal");

	qRegisterMetaType<Agent::Data>("Agent::Data");

    MainWindow w;
    w.show();
    return a.exec();
}
