#include <QCoreApplication>

#include "server.hpp"
#include "logger.hpp"

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);

    auto s = new Server();
    auto l = new Logger();

    a.connect(s, SIGNAL(message(QString)), l, SLOT(handleMessage(QString)));

    return a.exec();
}
