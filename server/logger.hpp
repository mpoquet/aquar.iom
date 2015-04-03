#pragma once

#include <QObject>
#include <QTextStream>

class Logger : public QObject
{
    Q_OBJECT
public:
    Logger(QObject *parent = nullptr);

public slots:
    void handleMessage(const QString & message);

private:
    QTextStream * _stream;
};
