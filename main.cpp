#include "mainwindow.h"

#include <QApplication>

/*
Master API 47646613f1c442ab94df5742049f2d3b
 */



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
