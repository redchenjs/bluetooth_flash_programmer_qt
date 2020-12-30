/*
 * flash.h
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef FLASH_H
#define FLASH_H

#include <QtCore>
#include <QtBluetooth>

#define NONE         1
#define OK           0
#define ERR_ARG     -1
#define ERR_FILE    -2
#define ERR_ABORT   -3
#define ERR_SOCKET  -4
#define ERR_DISCOVR -5

#define RW_NONE  0
#define RW_READ  1
#define RW_WRITE 2

class FlashProgrammer : public QObject
{
    Q_OBJECT

public:
    void stop(int err = OK);
    void start(int argc, char *argv[]);

private:
    char **m_arg = nullptr;

    QBluetoothAddress m_address;
    QBluetoothDeviceDiscoveryAgent *m_discovery = nullptr;
    QBluetoothSocket *m_socket = nullptr;

    int m_cmd_idx = 0;
    char m_cmd_str[32] = {0};

    QFile *data_fd = nullptr;
    uint32_t data_size = 0;
    uint32_t data_done = 0;

    int err_code = NONE;
    int rw_state = RW_NONE;

    void printUsage(void);

private slots:
    void sendData(void);
    void sendCommand(void);

    void processData(void);

    void deviceDiscovered(const QBluetoothDeviceInfo &device);
    void deviceDiscoveryFinished(void);
    void deviceConnected(void);
    void deviceDisconnected(void);

    void errorDiscovery(QBluetoothDeviceDiscoveryAgent::Error err);
    void errorSocket(QBluetoothSocket::SocketError err);

signals:
    void finished(int err = OK);
};

#endif // FLASH_H
