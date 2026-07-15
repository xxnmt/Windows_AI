#include <QApplication>

#include "appcontroller.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    AppController ctrl;
    ctrl.startApp();
    return a.exec();
}