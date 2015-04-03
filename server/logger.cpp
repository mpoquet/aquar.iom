#include "logger.hpp"

Logger::Logger(QObject *parent) : QObject(parent)
{
    _stream = new QTextStream(stdout, QIODevice::WriteOnly);
}

void Logger::handleMessage(const QString &message)
{
    *_stream << message << endl;
    _stream->flush();
}
