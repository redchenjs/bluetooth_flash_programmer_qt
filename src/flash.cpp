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
#include <QThread>
#include <QtSerialPort/QSerialPort>

#include "flash.h"

bool flash_class::send_byte(const char c)
{
    if (!m_device->isOpen()) {
        return false;
    }

    if ((m_device->write(&c, 1)) < 1) {
        return false;
    }

    return true;
}

bool flash_class::send_string(QString *s)
{
    QByteArray bytes;
    bytes.reserve(s->size());
    for (auto &c : *s) {
        bytes.append(static_cast<char>(c.unicode()));
    }

    for (int i=0; i<bytes.length(); i++) {
        if (!send_byte(bytes[i])) {
            return false;
        }
    }

    if (!send_byte('\r') || !send_byte('\n')) {
        return false;
    }

    return true;
}

void flash_class::process_data(void)
{
    static uint8_t read_in_progress = 0;
    QByteArray data = m_device->readAll();

    if (!read_in_progress) {
        if (strncmp(data.data(), "OK\r\n", 4) == 0) {
            m_device_rsp = 1;
            std::cout << "<< " << data.data();
            return;
        } else if (strncmp(data.data(), "DONE\r\n", 6) == 0) {
            m_device_rsp = 1;
            std::cout << "<< " << data.data();
            return;
        } else if (strncmp(data.data(), "FAIL\r\n", 6) == 0) {
            m_device_rsp = 2;
            std::cout << "<< " << data.data();
            return;
        } else if (strncmp(data.data(), "ERROR\r\n", 7) == 0) {
            m_device_rsp = 2;
            std::cout << "<< " << data.data();
            return;
        } else if (strncmp(data.data(), "INFO:", 5) == 0) {
            m_device_rsp = 1;
            std::cout << "<< " << data.data();
            return;
        }
        m_device_rsp = 1;
        read_in_progress = 1;
    }
    if (data.size() > 0 && data_fd != nullptr && data_recv != data_need) {
        if ((data_need - data_recv) < static_cast<uint32_t>(data.size())) {
            data_fd->write(data, data_need - data_recv);
            data_recv += data_need - data_recv;
        } else {
            data_fd->write(data, data.size());
            data_recv += static_cast<uint32_t>(data.size());
        }
        if (data_recv == data_need) {
            read_in_progress = 0;
        }
    }
}

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
        return -1;
    }

    return 0;
}

int flash_class::close_device(void)
{
    m_device->clearError();
    m_device->close();

    return 0;
}

int flash_class::erase_all(const QString &devname)
{
    // open serial device
    if (open_device(devname)) {
        return -1;
    }

    // send chip erase command
    char cmd_str[32] = {0};
    snprintf(cmd_str, sizeof(cmd_str), "MTD+ERASE:ALL!");
    std::cout << ">> " << cmd_str << std::endl;
    QString cmd = cmd_str;
    send_string(&cmd);
    m_device->waitForBytesWritten();
    while (m_device_rsp == 0) {
        QThread::msleep(20);
        m_device->waitForReadyRead();
    }
    // error
    if (m_device_rsp == 2) {
        // close serial device
        close_device();
        return -3;
    }
    m_device_rsp = 0;

    // close serial device
    return close_device();
}

int flash_class::erase(const QString &devname, uint32_t addr, uint32_t length)
{
    if (length <= 0) {
        std::cout << "invalid length" << std::endl;
        return -2;
    }

    // open serial device
    if (open_device(devname)) {
        return -1;
    }

    // send erase command
    char cmd_str[32] = {0};
    snprintf(cmd_str, sizeof(cmd_str), "MTD+ERASE:0x%x+0x%x", addr, length);
    std::cout << ">> " << cmd_str << std::endl;
    QString cmd = cmd_str;
    send_string(&cmd);
    m_device->waitForBytesWritten();
    while (m_device_rsp == 0) {
        QThread::msleep(20);
        m_device->waitForReadyRead();
    }
    // error
    if (m_device_rsp == 2) {
        // close serial device
        close_device();
        return -3;
    }
    m_device_rsp = 0;

    // close serial device
    return close_device();
}

int flash_class::write(const QString &devname, uint32_t addr, uint32_t length, QString filename)
{
    if (length <= 0) {
        std::cout << "invalid length" << std::endl;
        return -2;
    }

    // open data file
    QFile fd(filename);
    if (!fd.open(QIODevice::ReadOnly)) {
        std::cout << "could not open file" << std::endl;
        return -2;
    }

    uint32_t filesize = static_cast<uint32_t>(fd.size());
    if (filesize < length) {
        std::cout << "length should not be larger than actual file size" << std::endl;
        // close data file
        fd.close();
        return -2;
    }

    // open serial device
    if (open_device(devname)) {
        // close data file
        fd.close();
        return -1;
    }

    // send write command
    char cmd_str[32] = {0};
    snprintf(cmd_str, sizeof(cmd_str), "MTD+WRITE:0x%x+0x%x", addr, length);
    std::cout << ">> " << cmd_str << std::endl;
    QString cmd = cmd_str;
    send_string(&cmd);
    m_device->waitForBytesWritten();
    while (m_device_rsp == 0) {
        QThread::msleep(20);
        m_device->waitForReadyRead();
    }
    // error
    if (m_device_rsp == 2) {
        // close data file
        fd.close();
        // close serial device
        close_device();
        return -3;
    }
    m_device_rsp = 0;

    // send data
    QByteArray filedata = fd.readAll();
    for (size_t i=0; i<length; i++) {
        bool rc = send_byte(filedata.at(static_cast<int>(i)));
        if (!rc) {
            std::cout << "write failed" << std::endl;
            // close data file
            fd.close();
            // close serial device
            close_device();
            return -4;
        }
        // flush every 990 bytes
        if ((i+1) % 990 == 0) {
            std::cout << ">> SENT:" << i*100/length << "%\r";
            m_device->waitForBytesWritten();
            QThread::msleep(20);
        }
    }
    std::cout << std::endl;
    m_device->waitForBytesWritten();
    while (m_device_rsp == 0) {
        QThread::msleep(20);
        m_device->waitForReadyRead();
    }
    // error
    if (m_device_rsp == 2) {
        // close data file
        fd.close();
        // close serial device
        close_device();
        return -3;
    }
    m_device_rsp = 0;

    // close data file
    fd.close();

    // close serial device
    return close_device();
}

int flash_class::read(const QString &devname, uint32_t addr, uint32_t length, QString filename)
{
    if (length <= 0) {
        std::cout << "invalid length" << std::endl;
        return -2;
    }

    // open data file
    QFile fd(filename);
    if (!fd.open(QIODevice::WriteOnly)) {
        std::cout << "could not open file" << std::endl;
        return -2;
    }

    // open serial device
    if (open_device(devname)) {
        // close data file
        fd.close();
        return -1;
    }

    // send read command
    data_fd = &fd;
    data_need = length;
    data_recv = 0;
    char cmd_str[32] = {0};
    snprintf(cmd_str, sizeof(cmd_str), "MTD+READ:0x%x+0x%x", addr, length);
    std::cout << ">> " << cmd_str << std::endl;
    QString cmd = cmd_str;
    send_string(&cmd);
    m_device->waitForBytesWritten();
    while (m_device_rsp == 0) {
        QThread::msleep(20);
        m_device->waitForReadyRead();
    }
    // error
    if (m_device_rsp == 2) {
        // close data file
        fd.close();
        // close serial device
        close_device();
        return -3;
    }
    m_device_rsp = 0;

    // receive data
    do {
        QThread::msleep(20);
        m_device->waitForReadyRead();
        std::cout << "<< RECV:" << (data_recv)*100/data_need << "%\r";
    } while (data_recv != data_need);
    std::cout << std::endl;
    std::cout << "<< DONE" << std::endl;
    data_fd = nullptr;
    data_need = 0;
    data_recv = 0;

    // close data file
    fd.close();

    // close serial device
    return close_device();
}

int flash_class::info(const QString &devname)
{
    // open serial device
    if (open_device(devname)) {
        return -1;
    }

    // send info command
    char cmd_str[32] = {0};
    snprintf(cmd_str, sizeof(cmd_str), "MTD+INFO?");
    std::cout << ">> " << cmd_str << std::endl;
    QString cmd = cmd_str;
    send_string(&cmd);
    m_device->waitForBytesWritten();
    while (m_device_rsp == 0) {
        QThread::msleep(20);
        m_device->waitForReadyRead();
    }
    // error
    if (m_device_rsp == 2) {
        // close serial device
        close_device();
        return -3;
    }
    m_device_rsp = 0;

    // close serial device
    return close_device();
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

int flash_class::exec(int argc, char *argv[])
{
    int res = 0;

    if (argc < 3) {
        print_usage();
        return -1;
    }

    std::cout << std::unitbuf;

    m_device = new QSerialPort(this);
    connect(m_device, &QSerialPort::readyRead, this, &flash_class::process_data);

    QString devname = QString(argv[1]);
    QString options = QString(argv[2]);

    if (options == "erase_all" && argc == 3) {
        res = erase_all(devname);
    } else if (options == "erase" && argc == 5) {
        uint32_t addr = static_cast<uint32_t>(std::stoul(argv[3], nullptr, 16));
        uint32_t length = static_cast<uint32_t>(std::stoul(argv[4], nullptr, 16));
        res = erase(devname, addr, length);
    } else if (options == "write" && argc == 6) {
        uint32_t addr = static_cast<uint32_t>(std::stoul(argv[3], nullptr, 16));
        uint32_t length = static_cast<uint32_t>(std::stoul(argv[4], nullptr, 16));
        QString filename = QString(argv[5]);
        res = write(devname, addr, length, filename);
    } else if (options == "read" && argc == 6) {
        uint32_t addr = static_cast<uint32_t>(std::stoul(argv[3], nullptr, 16));
        uint32_t length = static_cast<uint32_t>(std::stoul(argv[4], nullptr, 16));
        QString filename = QString(argv[5]);
        res = read(devname, addr, length, filename);
    } else if (options == "info" && argc == 3) {
        res = info(devname);
    } else {
        print_usage();
        return -1;
    }

    return res;
}
