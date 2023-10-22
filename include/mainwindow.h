#pragma once

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#define NOMINMAX

#include <QMainWindow>
#include <QWidget.h>
#include <QtSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <ixwebsocket/IXNetSystem.h>
#include "vis.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow() override;

protected:
    Ui::MainWindow* ui;

private slots:
    void onSerialDataAvailable();
    void on_connectButton_clicked();
    void on_sendButton_clicked();

private:
    //void getAvailableSerialDevices();
    void serialRead();
    void serialWrite(unsigned char message[900]);

    qint32 baudrate;
    QSerialPort* usbDevice;
    std::vector<QSerialPortInfo> serialComPortList; //A list of the available ports for the dropdownmenue in the GUI

    QString deviceDescription;
    QString serialBuffer;

    bool serialDeviceIsConnected;

    VisualObject visObj;

    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point stop;
};

#endif