#include <QApplication>
#include <QWidget>

#include <iostream>
#include "format.h"

int main(int argc, char *argv[]) {
    std::cout<<"Hello GFXReconstruct Viewer!"<<std::endl;

    QApplication app(argc, argv);

    QWidget window;

    window.setWindowTitle("GFXReconstruct Viewer");

    window.show();

    return app.exec();
}