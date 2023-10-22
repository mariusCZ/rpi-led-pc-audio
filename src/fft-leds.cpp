// fft-leds.cpp : Defines the entry point for the application.
//

#include "../include/fft-leds.h"
#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include "../include/mainwindow.h"

int main(int argc, char* argv[])
{
	//VisualObject vis;
	//vis.startAudioVis();
	QApplication app(argc, argv);

	MainWindow main_window;
	main_window.show();

	return app.exec();
}
