/*
 * main.cpp
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <csignal>

#include <QtCore>
#include <QtGlobal>

#include "flash.h"

static FlashProgrammer flash;

void messageHandle(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    Q_UNUSED(msg);
}

void signalHandle(int signum)
{
    switch (signum) {
    case SIGINT:
    case SIGTERM:
        flash.stop(ERR_ABORT);
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qInstallMessageHandler(messageHandle);

    signal(SIGINT, signalHandle);
    signal(SIGTERM, signalHandle);

    QObject::connect(&flash, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, &flash, [&]() -> void { flash.start(argc, argv); });

    return app.exec();
}
