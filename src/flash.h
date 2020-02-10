/*
 * flash.h
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef FLASH_H
#define FLASH_H

#include <QtCore>
#include <QtSerialPort/QSerialPort>

class flash_class : public QObject
{
    Q_OBJECT

public:
    int exec(int argc, char *argv[]);

private:
    QSerialPort *m_device = nullptr;
    size_t m_device_rsp = 0;

    QFile *data_fd = nullptr;
    uint32_t data_need = 0;
    uint32_t data_recv = 0;

    bool send_byte(const char c);
    bool send_string(QString *s);

    void process_data(void);

    int open_device(const QString &devname);
    int close_device(void);

    int erase_all(const QString &devname);
    int erase(const QString &devname, uint32_t addr, uint32_t length);
    int write(const QString &devname, uint32_t addr, uint32_t length, QString filename);
    int read(const QString &devname, uint32_t addr, uint32_t length, QString filename);
    int info(const QString &devname);

    void print_usage(void);
};

#endif // FLASH_H
