#pragma once

#include "audio-processing.h"
#include "ixwebsocket/IXWebSocket.h"

#define DECAY_RATE_MAX 3
#define DECAY_RATE_MIN 2

#define NUM_LEDS 300

typedef struct ledstruc {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char abslt;
} ledstruc;

class VisualObject {
public:
	~VisualObject() {
		done = true;
		sendThread.join();
		if (webSocket.getReadyState() == ix::ReadyState::Open)
			webSocket.stop();
	}

	int timerLength = 20;

	void startAudioVis();
	void updateAudioValues();
private:
	float maxPeak = 1;
	float octBins[OCT_N] = { 0 };
	int routineRunning = 0;

	unsigned int ledProps[OCT_N];

	AudioProc audioHandle;
	ledstruc leds[NUM_LEDS];

	ix::WebSocket webSocket;

	std::thread sendThread;
	std::atomic<bool> done;
	std::atomic<bool> sendData{true};

	float decayFun(float val);
	void mapWidthx2(unsigned char widthArr[], unsigned int widthLen, float input);
	void distributeProportions();
	void visAudioRoutine1(unsigned char col, unsigned char colCoeff);
	void abslToRGB(unsigned char col, unsigned char colCoeff);
	float mapLogCol(unsigned int colCoeff);
	unsigned int sumP(unsigned int arr[]);
};