#pragma once

#include <QObject>

#include "structures.hpp"

class Network : public QObject
{
    Q_OBJECT
public:
    explicit Network(QObject *parent = 0);

signals:
    void turn_received(const Turn & turn);
    void welcome_received(const Welcome & welcome);

public slots:
};

