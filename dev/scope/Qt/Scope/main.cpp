#include "scope.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Scope w;
    w.show();
    return a.exec();
}
