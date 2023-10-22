#pragma once

#include <portaudio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <array>

#include "pa_ringbuffer.h"
#include "kiss_fftr.h"

/* #define SAMPLE_RATE  (17932) // Test failure to open with this value. */
#define SAMPLE_RATE  (44100)
#define FRAMES_PER_BUFFER (512)
#define OCT_N           (8)
#define NUM_SECONDS     (5)
#define NUM_CHANNELS    (1)
/* #define DITHER_FLAG     (paDitherOff) */
#define DITHER_FLAG     (0) /**/

/* Select sample format. */
#if 1
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 1
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif

typedef struct
{
	SAMPLE* recordedSamples;
	PaUtilRingBuffer    ringBuffer;
}
paTestData;

class AudioProc {
public:
	AudioProc() {
		rWindow = window_hamming();
		aWeights = weight_table();
	}
	~AudioProc() {
		Pa_Terminate();
		if (data.recordedSamples)       /* Sure it is NULL or valid. */
			free(data.recordedSamples);
	}

	int init();
	int startStream();
	int stopStream();
	int streamRunning();
	int ringBufferFramesAvail();
	void readSamples();
	void performFFT(float output[]);
private:
	SAMPLE				recordedSamples[FRAMES_PER_BUFFER];
	PaStreamParameters  inputParameters;
	PaStream*			stream;
	PaError             err = paNoError;
	paTestData          data;

	std::array<float, FRAMES_PER_BUFFER> rWindow;
	std::array<float, FRAMES_PER_BUFFER/2> aWeights;

	constexpr std::array<float, FRAMES_PER_BUFFER> window_hamming();
	constexpr std::array<float, FRAMES_PER_BUFFER/2> weight_table();
	kiss_fftr_cfg mycfg;

	static int recordCallback(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void* userData);
};

// Hamming window creation function
constexpr std::array<float, FRAMES_PER_BUFFER> AudioProc::window_hamming()
{
	std::array<float, FRAMES_PER_BUFFER> a{0};
	float alpha = 25.0F / (float)46;
	float factor = 2.0F * (float)M_PI / (float)FRAMES_PER_BUFFER;

	for (int i = 0; i < FRAMES_PER_BUFFER; i++)
		a[i] = alpha - (1.0F - alpha) * cos(factor * i);
	return a;
}

constexpr std::array<float, FRAMES_PER_BUFFER/2> AudioProc::weight_table()
{
	const float f_bin = SAMPLE_RATE / FRAMES_PER_BUFFER;
	std::array<float, FRAMES_PER_BUFFER/2> A{0};
	for (int i = 0; i < FRAMES_PER_BUFFER/2; i++) {
		float a = powf(12194, 2) * powf(f_bin * i, 2);
		float b = powf(f_bin * i, 2) + powf(20.6, 2);
		//float c = sqrtf( ( pow(f_bin * i, 2) + powf(107.7, 2) ) * ( pow(f_bin * i, 2) + powf(737.9, 2) ) );
		float d = powf(f_bin * i, 2) + powf(12194, 2);
		//float Ra = a / (b * c * d);
		float Ra = a / (b * d);
		//A[i] = 20 * log10f(Ra) + 2;
		A[i] = 20 * log10f(Ra) + 0.06;
		if (i > 11) A[i] = -0.005;
		A[i] = powf(2, A[i]);
	}
	return A;
}