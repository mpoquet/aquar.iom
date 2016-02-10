#include "visu.hpp"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Visu w;
    w.show();

    return a.exec();
}
