/*
 * flash.cpp
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <cstdio>
#include <string>
#include <iostream>

#include <QtCore>
#include <QtBluetooth>

#include "flash.h"

#define TX_BUF_SIZE 990

#define CMD_FMT_ERASE_ALL "MTD+ERASE:ALL!"
#define CMD_FMT_ERASE     "MTD+ERASE:0x%x+0x%x"
#define CMD_FMT_WRITE     "MTD+WRITE:0x%x+0x%x"
#define CMD_FMT_READ      "MTD+READ:0x%x+0x%x"
#define CMD_FMT_INFO      "MTD+INFO?"

enum cmd_idx {
    CMD_IDX_ERASE_ALL = 0x0,
    CMD_IDX_ERASE     = 0x1,
    CMD_IDX_WRITE     = 0x2,
    CMD_IDX_READ      = 0x3,
    CMD_IDX_INFO      = 0x4
};

enum rsp_idx {
    RSP_IDX_NONE  = 0x0,
    RSP_IDX_TRUE  = 0x1,
    RSP_IDX_FALSE = 0x2
};

typedef struct {
    const bool flag;
    const char fmt[32];
} rsp_fmt_t;

static const rsp_fmt_t rsp_fmt[] = {
    { true,  "OK\r\n"    },
    { true,  "DONE\r\n"  },
    { false, "FAIL\r\n"  },
    { false, "ERROR\r\n" }
};

void FlashProgrammer::printUsage(void)
{
    std::cout << "Usage:" << std::endl;
    std::cout << "    " << m_arg[0] << " BD_ADDR COMMAND" << std::endl << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "    erase_all                     erase full flash chip" << std::endl;
    std::cout << "    erase addr length             erase flash from [addr] to [addr + length]" << std::endl;
    std::cout << "    write addr length data.bin    write flash from [addr] to [addr + length]" << std::endl;
    std::cout << "    read  addr length data.bin    read flash from [addr] to [addr + length]" << std::endl;
    std::cout << "    info                          get flash info" << std::endl;

    stop(ERR_ARG);
}

void FlashProgrammer::sendData(void)
{
    char read_buff[TX_BUF_SIZE] = {0};
    uint32_t data_remain = data_size - data_done;

    if (m_socket->bytesToWrite() != 0) {
        return;
    }

    if (data_remain == 0) {
        std::cout << ">> SENT:100%\r";

        data_fd->close();

        disconnect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sendData()));
    } else {
        std::cout << ">> SENT:" << data_done * 100 / data_size << "%\r";

        if (data_remain >= sizeof(read_buff)) {
            if (data_fd->read(read_buff, sizeof(read_buff)) != sizeof(read_buff)) {
                std::cout << std::endl << "=! ERROR";

                data_fd->close();

                stop(ERR_FILE);
                return;
            }

            m_socket->write(read_buff, sizeof(read_buff));

            data_done += sizeof(read_buff);
        } else {
            if (data_fd->read(read_buff, data_remain) != data_remain) {
                std::cout << std::endl << "=! ERROR";

                data_fd->close();

                stop(ERR_FILE);
                return;
            }

            m_socket->write(read_buff, data_remain);

            data_done += data_remain;
        }
    }
}

void FlashProgrammer::sendCommand(void)
{
    std::cout << "=> " << m_cmd_str;

    m_socket->write(m_cmd_str, static_cast<uint32_t>(strlen(m_cmd_str)));
}

void FlashProgrammer::processData(void)
{
    QByteArray recv = m_socket->readAll();
    char *recv_buff = recv.data();

    for (uint8_t i = 0; i < sizeof(rsp_fmt) / sizeof(rsp_fmt_t); i++) {
        if (strncmp(recv_buff, rsp_fmt[i].fmt, strlen(rsp_fmt[i].fmt)) == 0) {
            if (rw_state != RW_NONE) {
                std::cout << std::endl;
            }
            std::cout << "<= " << recv_buff;

            if (rsp_fmt[i].flag == true) {
                switch (m_cmd_idx) {
                case CMD_IDX_ERASE_ALL:
                case CMD_IDX_ERASE:
                    stop(OK);
                    break;
                case CMD_IDX_WRITE:
                    if (rw_state == RW_NONE) {
                        rw_state = RW_WRITE;

                        connect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sendData()));

                        sendData();
                    } else {
                        rw_state = RW_NONE;

                        stop(OK);
                    }
                    break;
                case CMD_IDX_READ:
                    rw_state = RW_READ;
                    std::cout << "<< RECV:0%\r";
                    break;
                default:
                    break;
                }
            } else {
                rw_state = RW_NONE;

                stop(OK);
            }

            return;
        }
    }

    if (rw_state == RW_READ) {
        uint32_t recv_size = static_cast<uint32_t>(recv.size());
        uint32_t data_remain = data_size - data_done;

        if (data_remain <= recv_size) {
            data_fd->write(recv_buff, data_remain);
            data_fd->close();

            data_done += data_remain;

            std::cout << "<< RECV:100%" << std::endl;
            std::cout << "== DONE" << std::endl;

            rw_state = RW_NONE;

            stop(OK);
        } else {
            data_fd->write(recv_buff, recv_size);

            data_done += recv_size;

            std::cout << "<< RECV:" << data_done * 100 / data_size << "%\r";
        }
    } else {
        std::cout << "<= " << recv_buff;

        stop(OK);
    }
}

void FlashProgrammer::deviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (device.coreConfigurations() & QBluetoothDeviceInfo::BaseRateCoreConfiguration) {
        if (device.address() == m_address) {
            m_socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);

            m_discovery->stop();
        }
    }
}

void FlashProgrammer::deviceDiscoveryFinished(void)
{
    if (m_socket) {
        connect(m_socket, SIGNAL(readyRead()), this, SLOT(processData()));
        connect(m_socket, SIGNAL(connected()), this, SLOT(deviceConnected()));
        connect(m_socket, SIGNAL(disconnected()), this, SLOT(deviceDisconnected()));
        connect(m_socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(errorSocket(QBluetoothSocket::SocketError)));

        m_socket->connectToService(m_address, QBluetoothUuid::SerialPort);
    } else {
        errorDiscovery(QBluetoothDeviceDiscoveryAgent::NoError);
    }
}

void FlashProgrammer::deviceConnected(void)
{
    sendCommand();
}

void FlashProgrammer::deviceDisconnected(void)
{
    errorSocket(QBluetoothSocket::NoSocketError);
}

void FlashProgrammer::errorDiscovery(QBluetoothDeviceDiscoveryAgent::Error err)
{
    qDebug() << err;

    stop(ERR_DISCOVR);
}

void FlashProgrammer::errorSocket(QBluetoothSocket::SocketError err)
{
    qDebug() << err;

    stop(ERR_SOCKET);
}

void FlashProgrammer::stop(int err)
{
    if (err_code == NONE) {
        err_code = err;

        if (rw_state != RW_NONE) {
            std::cout << std::endl;
        }

        switch (err) {
        case ERR_DISCOVR:
            std::cout << ">? ERROR" << std::endl;
            break;
        case ERR_SOCKET:
            std::cout << ">! ERROR" << std::endl;
            break;
        default:
            break;
        }

        if (m_socket) {
            m_socket->disconnectFromService();
        }

        emit finished(err);
    }
}

void FlashProgrammer::start(int argc, char *argv[])
{
    m_arg = argv;
    std::cout << std::unitbuf;

    if (argc >= 3) {
        QString command = QString(m_arg[2]);

        if (command == "erase_all" && argc == 3) {
            m_cmd_idx = CMD_IDX_ERASE_ALL;
            snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_ERASE_ALL"\r\n");
        } else if (command == "erase" && argc == 5) {
            uint32_t addr = 0, length = 0;

            std::sscanf(m_arg[3], "%i", &addr);
            std::sscanf(m_arg[4], "%i", &length);

            if (length <= 0) {
                std::cout << "Length must be greater than 0" << std::endl;

                stop(ERR_FILE);
                return;
            }

            m_cmd_idx = CMD_IDX_ERASE;
            snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_ERASE"\r\n", addr, length);
        } else if (command == "write" && argc == 6) {
            uint32_t addr = 0, length = 0;

            std::sscanf(m_arg[3], "%i", &addr);
            std::sscanf(m_arg[4], "%i", &length);

            if (length <= 0) {
                std::cout << "Length must be greater than 0" << std::endl;

                stop(ERR_FILE);
                return;
            }

            data_fd = new QFile(m_arg[5]);
            if (!data_fd->open(QIODevice::ReadOnly)) {
                std::cout << "Could not open file: " << m_arg[5] << std::endl;

                stop(ERR_FILE);
                return;
            }

            data_size = length;
            data_done = 0;

            m_cmd_idx = CMD_IDX_WRITE;
            snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_WRITE"\r\n", addr, length);
        } else if (command == "read" && argc == 6) {
            uint32_t addr = 0, length = 0;

            std::sscanf(m_arg[3], "%i", &addr);
            std::sscanf(m_arg[4], "%i", &length);

            if (length <= 0) {
                std::cout << "Length must be greater than 0" << std::endl;

                stop(ERR_FILE);
                return;
            }

            data_fd = new QFile(m_arg[5]);
            if (!data_fd->open(QIODevice::WriteOnly)) {
                std::cout << "Could not open file: " << m_arg[5] << std::endl;

                stop(ERR_FILE);
                return;
            }

            data_size = length;
            data_done = 0;

            m_cmd_idx = CMD_IDX_READ;
            snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_READ"\r\n", addr, length);
        } else if (command == "info" && argc == 3) {
            m_cmd_idx = CMD_IDX_INFO;
            snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_INFO"\r\n");
        } else {
            printUsage();

            stop(ERR_ARG);
            return;
        }
    } else {
        printUsage();

        stop(ERR_ARG);
        return;
    }

    m_address = QBluetoothAddress(m_arg[1]);
    m_discovery = new QBluetoothDeviceDiscoveryAgent(this);
    m_discovery->setLowEnergyDiscoveryTimeout(5000);

    connect(m_discovery, SIGNAL(canceled()), this, SLOT(deviceDiscoveryFinished()));
    connect(m_discovery, SIGNAL(finished()), this, SLOT(deviceDiscoveryFinished()));
    connect(m_discovery, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    connect(m_discovery, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)), this, SLOT(errorDiscovery(QBluetoothDeviceDiscoveryAgent::Error)));

    m_discovery->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);
}
