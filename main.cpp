#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	QCoreApplication::setOrganizationName("GeoScout");
	QCoreApplication::setOrganizationDomain("geoscout.com");
	QCoreApplication::setApplicationName("RotorSysCal");

    MainWindow w;
    w.show();
    return a.exec();
}
