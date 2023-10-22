#include "../include/mainwindow.h"
#include "../ui/ui_mainwindow.h"
#include <chrono>

// Main window function
MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    usbDevice = new QSerialPort(this);
    connect(usbDevice, SIGNAL(readyRead()), this, SLOT(onSerialDataAvailable()));

    baudrate = 2000000;

    serialDeviceIsConnected = false;

    ix::initNetSystem();
}

// Main window destructor.
MainWindow::~MainWindow()
{
    ix::uninitNetSystem();
    delete ui;
    delete usbDevice;
}

void MainWindow::serialWrite(unsigned char message[900])
{
    if (serialDeviceIsConnected == true)
    {
        start = std::chrono::high_resolution_clock::now();
        int a = usbDevice->write((char *)message, 900); // Send the message to the device
    }
}
void MainWindow::serialRead()
{
    if (serialDeviceIsConnected == true)
    {
        serialBuffer += usbDevice->readAll(); // Read the available data
    }
}
void MainWindow::onSerialDataAvailable()
{
    if (serialDeviceIsConnected == true)
    {
        serialRead(); // Read a chunk of the Message
        stop = std::chrono::high_resolution_clock::now();
        auto duration = duration_cast<std::chrono::milliseconds>(stop - start);
        qDebug() << "Write time: " << duration.count();
        unsigned char ledBuf[900];
        visObj.updateAudioValues();
        serialWrite(ledBuf);
    }
}

void MainWindow::on_sendButton_clicked() {
    unsigned char buf[900] = { 0 };
    int j = 0;
    for (int i = 0; i < 900; i++) {
        if (j > 255) j = 0;
        buf[i] = j;
        j++;
    }
    serialWrite(buf);
}

void MainWindow::on_connectButton_clicked()
{
    visObj.startAudioVis();
}