#include <QApplication>

#include "appcontroller.h"

int main(int argc, char *argv[]) {
    //不创建应用名会报错
    QCoreApplication::setApplicationName("Windows_AI");
    // QCoreApplication::setOrganizationName("MakoDesktop");

    QApplication a(argc, argv);
    AppController ctrl;
    ctrl.startApp();

    return a.exec();
}