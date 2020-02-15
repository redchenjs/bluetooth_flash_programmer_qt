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
#include <QtSerialPort/QSerialPort>

#include "flash.h"

#define OK           0
#define ERR_ARG     -1
#define ERR_FILE    -2
#define ERR_DEVICE  -3
#define ERR_REMOTE  -4

#define CMD_FMT_ERASE_ALL   "MTD+ERASE:ALL!"
#define CMD_FMT_ERASE       "MTD+ERASE:0x%x+0x%x"
#define CMD_FMT_WRITE       "MTD+WRITE:0x%x+0x%x"
#define CMD_FMT_READ        "MTD+READ:0x%x+0x%x"
#define CMD_FMT_INFO        "MTD+INFO?"

enum cmd_idx {
    CMD_IDX_ERASE_ALL = 0x0,
    CMD_IDX_ERASE     = 0x1,
    CMD_IDX_WRITE     = 0x2,
    CMD_IDX_READ      = 0x3,
    CMD_IDX_INFO      = 0x4,
};

enum rsp_idx {
    RSP_IDX_NONE  = 0x0,
    RSP_IDX_TRUE  = 0x1,
    RSP_IDX_FALSE = 0x2,
};

typedef struct {
    const bool flag;
    const char fmt[32];
} rsp_fmt_t;

static const rsp_fmt_t rsp_fmt[] = {
    {  true, "OK\r\n" },     // OK
    {  true, "DONE\r\n" },   // Done
    { false, "FAIL\r\n" },   // Fail
    { false, "ERROR\r\n" },  // Error
};

int flash_class::open_device(const QString &devname)
{
    m_device->setPortName(devname);
    if (m_device->open(QIODevice::ReadWrite)) {
        m_device->setBaudRate(921600);
        m_device->setDataBits(QSerialPort::Data8);
        m_device->setParity(QSerialPort::NoParity);
        m_device->setStopBits(QSerialPort::OneStop);
        m_device->setFlowControl(QSerialPort::HardwareControl);
        m_device->flush();
    } else {
        std::cout << "could not open device" << std::endl;
        return ERR_DEVICE;
    }

    return OK;
}

int flash_class::close_device(void)
{
    m_device->clearError();
    m_device->close();

    return OK;
}

void flash_class::process_data(void)
{
    QByteArray data = m_device->readAll();

    char *data_buff = data.data();
    uint32_t data_size = static_cast<uint32_t>(data.size());

    for (uint8_t i=0; i<sizeof(rsp_fmt)/sizeof(rsp_fmt_t); i++) {
        if (strncmp(data_buff, rsp_fmt[i].fmt, strlen(rsp_fmt[i].fmt)) == 0) {
            if (rsp_fmt[i].flag == true) {
                m_device_rsp = RSP_IDX_TRUE;
            } else {
                m_device_rsp = RSP_IDX_FALSE;
                if (rw_in_progress) {
                    std::cout << std::endl;
                }
            }
            if (data_size == strlen(rsp_fmt[i].fmt)) {
                std::cout << "<< " << data_buff;
                return;
            }
        }
    }

    if (data_size > 0 && data_fd != nullptr && data_recv != data_need) {
        if ((data_need - data_recv) < data_size) {
            data_fd->write(data_buff, data_need - data_recv);
            data_recv += data_need - data_recv;
        } else {
            data_fd->write(data, data_size);
            data_recv += data_size;
        }
    } else {
        m_device_rsp = RSP_IDX_TRUE;
        std::cout << "<< " << data_buff;
    }
}

void flash_class::clear_response(void)
{
    m_device_rsp = RSP_IDX_NONE;
}

size_t flash_class::check_response(void)
{
    m_device->waitForReadyRead();

    return m_device_rsp;
}

size_t flash_class::wait_for_response(void)
{
    while (m_device_rsp == RSP_IDX_NONE) {
        m_device->waitForReadyRead();
    }

    return m_device_rsp;
}

int flash_class::send_data(const char *data, uint32_t length)
{
    if (m_device->write(data, length) < 1) {
        return ERR_DEVICE;
    }

    m_device->waitForBytesWritten();

    return OK;
}

int flash_class::erase_all(const QString &devname)
{
    int ret = OK;
    char cmd_str[32] = {0};

    if (open_device(devname)) {
        return ERR_DEVICE;
    }

    // send command
    snprintf(cmd_str, sizeof(cmd_str), CMD_FMT_ERASE_ALL"\r\n");
    std::cout << ">> " << cmd_str;

    clear_response();

    ret = send_data(cmd_str, static_cast<uint32_t>(strlen(cmd_str)));
    if (ret != OK) {
        close_device();
        return ret;
    }

    wait_for_response();

    close_device();

    return ret;
}

int flash_class::erase(const QString &devname, uint32_t addr, uint32_t length)
{
    int ret = OK;
    char cmd_str[32] = {0};

    if (open_device(devname)) {
        return ERR_DEVICE;
    }

    // send command
    snprintf(cmd_str, sizeof(cmd_str), CMD_FMT_ERASE"\r\n", addr, length);
    std::cout << ">> " << cmd_str;

    clear_response();

    ret = send_data(cmd_str, static_cast<uint32_t>(strlen(cmd_str)));
    if (ret != OK) {
        close_device();
        return ret;
    }

    wait_for_response();

    close_device();

    return ret;
}

int flash_class::write(const QString &devname, uint32_t addr, uint32_t length, QString filename)
{
    int ret = OK;
    char cmd_str[32] = {0};
    char data_buff[990] = {0};

    if (length <= 0) {
        std::cout << "invalid length" << std::endl;
        return ERR_ARG;
    }

    QFile fd(filename);
    if (!fd.open(QIODevice::ReadOnly)) {
        std::cout << "could not open file" << std::endl;
        return ERR_FILE;
    }

    if (open_device(devname)) {
        fd.close();
        return ERR_DEVICE;
    }

    // send command
    snprintf(cmd_str, sizeof(cmd_str), CMD_FMT_WRITE"\r\n", addr, length);
    std::cout << ">> " << cmd_str;

    clear_response();

    ret = send_data(cmd_str, static_cast<uint32_t>(strlen(cmd_str)));
    if (ret != OK) {
        fd.close();
        close_device();
        return ret;
    }

    if (wait_for_response() == RSP_IDX_FALSE) {
        fd.close();
        close_device();
        return ERR_REMOTE;
    }

    // send data
    rw_in_progress = 1;
    std::cout << ">> SENT:" << "0%\r";

    clear_response();

    uint32_t pkt = 0;
    for (pkt=0; pkt<length/990; pkt++) {
        if (fd.read(data_buff, 990) != 990) {
            std::cout << std::endl << ">> ERROR" << std::endl;
            fd.close();
            close_device();
            return ERR_FILE;
        }

        ret = send_data(data_buff, 990);
        if (ret != OK) {
            std::cout << std::endl << ">> ERROR" << std::endl;
            fd.close();
            close_device();
            return ret;
        }

        if (m_device_rsp == RSP_IDX_FALSE) {
            fd.close();
            close_device();
            return ERR_REMOTE;
        }

        std::cout << ">> SENT:" << pkt*990*100/length << "%\r";
    }

    uint32_t data_remain = length - pkt * 990;
    if (data_remain != 0 && data_remain < 990) {
        if (fd.read(data_buff, data_remain) != data_remain) {
            std::cout << std::endl << ">> ERROR" << std::endl;
            fd.close();
            close_device();
            return ERR_FILE;
        }

        ret = send_data(data_buff, data_remain);
        if (ret != OK) {
            std::cout << std::endl << ">> ERROR" << std::endl;
            fd.close();
            close_device();
            return ret;
        }

        if (m_device_rsp == RSP_IDX_FALSE) {
            fd.close();
            close_device();
            return ERR_REMOTE;
        }

        std::cout << ">> SENT:" << (data_remain+pkt*990)*100/length << "%\r";
    }

    std::cout << std::endl;
    rw_in_progress = 0;

    wait_for_response();

    fd.close();
    close_device();

    return ret;
}

int flash_class::read(const QString &devname, uint32_t addr, uint32_t length, QString filename)
{
    int ret = OK;
    char cmd_str[32] = {0};

    QFile fd(filename);
    if (!fd.open(QIODevice::WriteOnly)) {
        std::cout << "could not open file" << std::endl;
        return ERR_FILE;
    }

    if (open_device(devname)) {
        fd.close();
        return ERR_DEVICE;
    }

    // send command
    snprintf(cmd_str, sizeof(cmd_str), CMD_FMT_READ"\r\n", addr, length);
    std::cout << ">> " << cmd_str;

    data_fd = &fd; data_need = length; data_recv = 0;

    clear_response();

    ret = send_data(cmd_str, static_cast<uint32_t>(strlen(cmd_str)));
    if (ret != OK) {
        fd.close();
        close_device();
        return ret;
    }

    if (wait_for_response() == RSP_IDX_FALSE) {
        fd.close();
        close_device();
        return ERR_REMOTE;
    }

    // receive data
    rw_in_progress = 1;
    std::cout << "<< RECV:" << data_recv*100/data_need << "%\r";

    clear_response();

    do {
        if (check_response() == RSP_IDX_FALSE) {
            fd.close();
            close_device();
            return ERR_REMOTE;
        }

        std::cout << "<< RECV:" << data_recv*100/data_need << "%\r";
    } while (data_recv != data_need);

    std::cout << std::endl << "<< DONE" << std::endl;
    rw_in_progress = 0;

    data_fd = nullptr; data_need = 0; data_recv = 0;

    fd.close();
    close_device();

    return ret;
}

int flash_class::info(const QString &devname)
{
    int ret = OK;
    char cmd_str[32] = {0};

    if (open_device(devname)) {
        return ERR_DEVICE;
    }

    // send command
    snprintf(cmd_str, sizeof(cmd_str), CMD_FMT_INFO"\r\n");
    std::cout << ">> " << cmd_str;

    clear_response();

    ret = send_data(cmd_str, static_cast<uint32_t>(strlen(cmd_str)));
    if (ret != OK) {
        close_device();
        return ret;
    }

    wait_for_response();

    close_device();

    return ret;
}

void flash_class::print_usage(void)
{
    std::cout << "Usage:" << std::endl;
    std::cout << "    spp-flash-programmer /dev/rfcommX [OPTIONS]\n" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "    erase_all\t\t\terase full flash chip" << std::endl;
    std::cout << "    erase addr length\t\terase flash start from [addr] for [length] bytes" << std::endl;
    std::cout << "    write addr length data.bin\twrite [data.bin] to flash from [addr] for [length] bytes" << std::endl;
    std::cout << "    read  addr length data.bin\tread flash from [addr] for [length] bytes to [data.bin]" << std::endl;
    std::cout << "    info\t\t\tread flash info" << std::endl;
}

void flash_class::exit(void)
{
    if (rw_in_progress) {
        std::cout << std::endl;
    }

    m_device_rsp = RSP_IDX_FALSE;
}

int flash_class::exec(int argc, char *argv[])
{
    int ret = OK;

    if (argc < 3) {
        print_usage();
        return ERR_ARG;
    }

    QString devname = QString(argv[1]);
    QString options = QString(argv[2]);

    std::cout << std::unitbuf;

    m_device = new QSerialPort(this);
    connect(m_device, &QSerialPort::readyRead, this, &flash_class::process_data);

    if (options == "erase_all" && argc == 3) {
        ret = erase_all(devname);
    } else if (options == "erase" && argc == 5) {
        uint32_t addr = static_cast<uint32_t>(std::stoul(argv[3], nullptr, 16));
        uint32_t length = static_cast<uint32_t>(std::stoul(argv[4], nullptr, 16));

        if (length <= 0) {
            std::cout << "invalid length" << std::endl;
            return ERR_ARG;
        }

        ret = erase(devname, addr, length);
    } else if (options == "write" && argc == 6) {
        uint32_t addr = static_cast<uint32_t>(std::stoul(argv[3], nullptr, 16));
        uint32_t length = static_cast<uint32_t>(std::stoul(argv[4], nullptr, 16));
        QString filename = QString(argv[5]);

        if (length <= 0) {
            std::cout << "invalid length" << std::endl;
            return ERR_ARG;
        }

        ret = write(devname, addr, length, filename);
    } else if (options == "read" && argc == 6) {
        uint32_t addr = static_cast<uint32_t>(std::stoul(argv[3], nullptr, 16));
        uint32_t length = static_cast<uint32_t>(std::stoul(argv[4], nullptr, 16));
        QString filename = QString(argv[5]);

        if (length <= 0) {
            std::cout << "invalid length" << std::endl;
            return ERR_ARG;
        }

        ret = read(devname, addr, length, filename);
    } else if (options == "info" && argc == 3) {
        ret = info(devname);
    } else {
        print_usage();

        return ERR_ARG;
    }

    return ret;
}
