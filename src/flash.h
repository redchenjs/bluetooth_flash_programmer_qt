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

class FlashProgrammer : public QObject
{
    Q_OBJECT

public:
    void stop(void);
    void start(int argc, char *argv[]);

private:
    QSerialPort *m_device = nullptr;
    size_t m_device_rsp = 0;

    QFile *data_fd = nullptr;
    uint32_t data_need = 0;
    uint32_t data_recv = 0;

    size_t rw_in_progress = 0;

    int open_device(const QString &devname);
    int close_device(void);

    void clear_response(void);
    size_t check_response(void);
    size_t wait_for_response(void);

    void process_data(void);
    int send_data(const char *data, uint32_t length);

    int erase_all(const QString &devname);
    int erase(const QString &devname, uint32_t addr, uint32_t length);
    int write(const QString &devname, uint32_t addr, uint32_t length, QString filename);
    int read(const QString &devname, uint32_t addr, uint32_t length, QString filename);
    int info(const QString &devname);

    void print_usage(void);

signals:
    void finished(int err = 0);
};

#endif // FLASH_H
