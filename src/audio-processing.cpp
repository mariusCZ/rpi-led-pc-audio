#include "../include/audio-processing.h"

int AudioProc::init() {
	err = Pa_Initialize();
	if (err != paNoError) return 1;

    data.recordedSamples = (SAMPLE*)malloc(FRAMES_PER_BUFFER * NUM_CHANNELS * sizeof(SAMPLE) * 2); /* From now on, recordedSamples is initialised. */
    if (data.recordedSamples == NULL)
    {
        printf("Could not allocate record array.\n");
        return 1; // Add error handling, no gotos;
    }
    for (int i = 0; i < FRAMES_PER_BUFFER * NUM_CHANNELS; i++) data.recordedSamples[i] = 0;

    if (PaUtil_InitializeRingBuffer(&data.ringBuffer, sizeof(SAMPLE), FRAMES_PER_BUFFER * NUM_CHANNELS * 2, data.recordedSamples) < 0)
    {
        printf("Could not allocate ringbuffer array.\n");
        return 1; // Add error handling, no gotos;
    }

	mycfg = kiss_fftr_alloc(FRAMES_PER_BUFFER, 0, NULL, NULL);
    return 0;
}

int AudioProc::recordCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    paTestData* data = (paTestData*)userData;
    const SAMPLE* rptr = (const SAMPLE*)inputBuffer;
    ring_buffer_size_t elementsWriteable = PaUtil_GetRingBufferWriteAvailable(&data->ringBuffer);
    //SAMPLE* wptr = &data->recordedSamples[0]; // take care with this
    int finished;

    (void)outputBuffer; /* Prevent unused variable warnings. */
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;

    if (elementsWriteable >= FRAMES_PER_BUFFER) PaUtil_WriteRingBuffer(&data->ringBuffer, rptr, FRAMES_PER_BUFFER);
    else {
        PaUtil_FlushRingBuffer(&data->ringBuffer);
        PaUtil_WriteRingBuffer(&data->ringBuffer, rptr, FRAMES_PER_BUFFER);
    }

    finished = paContinue; // Make sure to handle stopping in some way. Will be either done with destructor or/and button press.
    return finished;
}

int AudioProc::ringBufferFramesAvail() {
    paTestData* pData = (paTestData*)&data;
    return PaUtil_GetRingBufferReadAvailable(&pData->ringBuffer);
}

void AudioProc::readSamples() {
    paTestData* pData = (paTestData*)&data;
    PaUtil_ReadRingBuffer(&pData->ringBuffer, recordedSamples, FRAMES_PER_BUFFER);
}

int AudioProc::startStream() {
    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    if (inputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default input device.\n");
        return 1;
    }
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    /* Record some audio. -------------------------------------------- */
    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        NULL,                  /* &outputParameters, */
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
        recordCallback,
        &data);
    if (err != paNoError) return 1;

    err = Pa_StartStream(stream);
    if (err != paNoError) return 1;
    printf("\n=== Now recording!! Please speak into the microphone. ===\n"); fflush(stdout);
    printf(Pa_GetDeviceInfo(inputParameters.device)->name);

    return 0;
}

int AudioProc::stopStream() {
    err = Pa_CloseStream(stream);
    return err;
}

int AudioProc::streamRunning() {
    return Pa_IsStreamActive(stream);
}

void AudioProc::performFFT(float output[]) {
    float windowSamples[FRAMES_PER_BUFFER];
    kiss_fft_cpx outBufCpx[FRAMES_PER_BUFFER];
    float outBuf[FRAMES_PER_BUFFER];
    float inputDebug[FRAMES_PER_BUFFER];

    for (int i = 0; i < FRAMES_PER_BUFFER; i++)
        inputDebug[i] = recordedSamples[i];

    for (int i = 0; i < FRAMES_PER_BUFFER; i++)
        windowSamples[i] = recordedSamples[i] * rWindow[i];

    kiss_fftr(mycfg, windowSamples, outBufCpx);
    for (int i = 0; i < FRAMES_PER_BUFFER; i++) {
        if (i != 0 && i < FRAMES_PER_BUFFER/2) {
            outBufCpx[i].r *= aWeights[i];
            outBufCpx[i].i *= aWeights[i];
        }
        outBuf[i] = sqrtf((pow(outBufCpx[i].r, 2) + pow(outBufCpx[i].i, 2)) / 8);
    }

    float intm = 0;
    for (int i = 0; i < OCT_N - 2; i++)
        output[i] = 32 * log2f(outBuf[i + 1]);

    const int freq5khz = 5000 / (SAMPLE_RATE / FRAMES_PER_BUFFER);
    const int freq2k5hz = 2500 / (SAMPLE_RATE / FRAMES_PER_BUFFER);
    for (int i = OCT_N - 2; i < freq5khz / 2; i++)
        intm += (outBuf[i]) * (outBuf[i]);
    output[OCT_N - 2] = 32 * log2f(sqrtf(intm));
    intm = 0;

    for (int i = freq5khz / 2; i < freq5khz; i++)
        intm += (outBuf[i]) * (outBuf[i]);
    output[OCT_N - 1] = 32 * log2f(sqrtf(intm));

    for (int i = 0; i < OCT_N; i++)
        if (output[i] < 0) output[i] = 0;
}