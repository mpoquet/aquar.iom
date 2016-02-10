#pragma once

#include <QWidget>

#include "structures.hpp"

class Visu : public QWidget
{
    Q_OBJECT

public:
    Visu(QWidget *parent = 0);
    ~Visu();

private slots:
    void onWelcomeReceived(const Welcome & welcome);
    void onTurnReceived(const Turn & turn);
};

