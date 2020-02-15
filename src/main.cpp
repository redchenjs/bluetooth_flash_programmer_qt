/*
 * main.cpp
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <csignal>
#include <QtGlobal>

#include "flash.h"

static FlashProgrammer flash;

void signalHandle(int signum)
{
    switch (signum) {
        case SIGINT:
        case SIGTERM:
        case SIGKILL:
            flash.stop();
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    signal(SIGINT, signalHandle);
    signal(SIGTERM, signalHandle);
    signal(SIGKILL, signalHandle);

    QObject::connect(&flash, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, &flash, [=]()->void{flash.start(argc, argv);});

    return app.exec();
}
