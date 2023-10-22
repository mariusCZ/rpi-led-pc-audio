#include "../include/vis.h"
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

/*
    TODO: perhaps different decay for different frequency bins.
    Faster for low freq and slower for higher. May look good while
    not being too jumpy as now.
*/

void VisualObject::startAudioVis() {
    std::string url("ws://raspberrypi:8008/");
    webSocket.setUrl(url);

    webSocket.setPingInterval(45);

    // Per message deflate connection is enabled by default. You can tweak its parameters or disable it
    webSocket.disablePerMessageDeflate();

    // Setup a callback to be fired when a message or an event (open, close, error) is received
    webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg)
        {
            if (msg->type == ix::WebSocketMessageType::Message)
            {
                std::cout << msg->str << std::endl;
                std::string tmpMsg1 = "STP";
                std::string tmpMsg2 = "STR";
                if (!msg->str.compare(tmpMsg1)) sendData = false;
                else if (!msg->str.compare(tmpMsg2)) sendData = true;
            }
        }
    );

    // Now that our callback is setup, we can start our background thread and receive messages
    webSocket.start();
    
    while (webSocket.getReadyState() != ix::ReadyState::Open)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    PaError err = paNoError;
    audioHandle.init();
    audioHandle.startStream();
    distributeProportions();
    
    done = false;
    std::string dev = "AUD,PCAudio";
    webSocket.sendText(dev);
    sendThread = std::thread(&VisualObject::updateAudioValues, this);
}

void VisualObject::updateAudioValues() {
    while (!done) {
        if (audioHandle.streamRunning() == 1) {
            float output[OCT_N] = { 0 };
            if (audioHandle.ringBufferFramesAvail() >= FRAMES_PER_BUFFER) {
                audioHandle.readSamples();
                audioHandle.performFFT(output);
                std::string data = "OCT";
                for (int i = 0; i < OCT_N; i++) {
                    data += "," + std::to_string((int)output[i]);
                    //std::cout << output[i] << " ";
                }
                //std::cout << std::endl;
                std::cout << data << std::endl;
                if(sendData)
                    webSocket.sendText(data);
            }

            //for (int i = 0; i < OCT_N; i++)
            //    // Make sure decay does not overflow.
            //    octBins[i] = (octBins[i] < decayFun(octBins[i])) ? 0 : (octBins[i] - decayFun(octBins[i]));
            // 
            //for (int i = 0; i < OCT_N; i++) {
            //    if (output[i] > maxPeak) maxPeak = output[i];
            //    // Change decay array value only if current FFT value is larger
            //    octBins[i] = (output[i] > octBins[i]) ? output[i] : octBins[i];
            //}
            //visAudioRoutine1('C', 0.1);
            //for (int i = 0; i < NUM_LEDS; i++) {
            //    ledVals[i * 3] = leds[i].r;
            //    ledVals[i * 3 + 1] = leds[i].g;
            //    ledVals[i * 3 + 2] = leds[i].b;
            //}
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

void VisualObject::visAudioRoutine1(unsigned char col, unsigned char colCoeff) {
    unsigned char* widthArr = new unsigned char[ledProps[0]];
    // Map the magnitude of frequency to amoount of LEDs to turn on and their color.
    mapWidthx2(widthArr, ledProps[0], octBins[0]);
    // Values stored in abslt, which is then converted to RGB.
    for (unsigned int i = NUM_LEDS / 2 - 1; i > (NUM_LEDS / 2 - 1) - ledProps[0]; i--)
        leds[i].abslt = widthArr[(NUM_LEDS / 2 - 1) - i];
    // Do the same for the rest of the strip.
    int pos = (NUM_LEDS / 2 - 1) - ledProps[0];
    for (unsigned int i = 1; i < OCT_N; i++) {
        mapWidthx2(widthArr, ledProps[i], octBins[i]);
        for (unsigned int j = 0; j < ledProps[i]; j++) {
            if ((pos - ledProps[i] + 1 + j) >= 0)
                leds[pos - ledProps[i] + 1 + j].abslt = widthArr[j];
            if (pos - ledProps[i] - j >= 0)
                leds[pos - ledProps[i] - j].abslt = widthArr[j];
        }
        // Keep track of position where LEDs are turned on.
        pos = ((pos - ledProps[i] * 2) >= 0) ? pos - ledProps[i] * 2 : 0;
    }

    //if (col == 'M') state = !state;
    //if (state)
        //reverseOrder(leds);

    // Symmetrically show the FFT and convert abslt to RGB.
    for (uint16_t i = 0; i < NUM_LEDS / 2; i++)
        leds[NUM_LEDS - 1 - i].abslt = leds[i].abslt;
    abslToRGB(col, colCoeff);
    delete[] widthArr;
}

float VisualObject::decayFun(float val) {
	// Decay mapped to a 2^x function. Makes decay look smooth.
	const float a = logf((float)DECAY_RATE_MAX) / (maxPeak * logf((float)2));
    float b = (DECAY_RATE_MAX + DECAY_RATE_MIN*2) - (powf(2, a * val) + DECAY_RATE_MIN - 1);
	//return (powf(2, a * val) + DECAY_RATE_MIN - 1);
    return (DECAY_RATE_MAX + DECAY_RATE_MIN * 2) - (powf(2, a * val) + DECAY_RATE_MIN);
}

void VisualObject::mapWidthx2(unsigned char widthArr[], unsigned int widthLen, float input)
{
    input = (input > maxPeak) ? maxPeak : input;
    // Map magnitude to a 2^x function.
    float a = logf(255 * (float)widthLen) / (maxPeak * logf(2));
    uint32_t width = (uint32_t)powf(2, a * input) - 1;

    for (uint16_t i = 0; i < widthLen; i++) {
        volatile float prop = (widthLen - (float)i) / widthLen;
        // Set the width array. prop^2 done because WS2812 brightness is weird.
        widthArr[i] = (width * prop > 255) ? 255 : width * prop * prop;
    }
}

void VisualObject::abslToRGB(unsigned char col, unsigned char colCoeff) {
    static float RCOEFF = 0.0F, GCOEFF = 0.99F, BCOEFF = 0.9F;

    // Set the color coefficients from Bluetooth received values
    switch (col) {
    case 'B':
        RCOEFF = 0;
        GCOEFF = mapLogCol(colCoeff);
        BCOEFF = 1.7F - GCOEFF;
        break;
    case 'C':
        RCOEFF = mapLogCol(colCoeff);
        GCOEFF = 0;
        BCOEFF = 1.7F - RCOEFF;
        break;
    case 'D':
        RCOEFF = mapLogCol(colCoeff);
        GCOEFF = 1.7F - RCOEFF;
        BCOEFF = 0;
        break;
    default:
        break;
    }

    // Make sure main color turns on first and only then other colors follow.
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
        leds[i].r = (leds[i].abslt > 255 * RCOEFF) ? leds[i].abslt - 255 * RCOEFF : 0;
        leds[i].g = (leds[i].abslt > 255 * GCOEFF) ? leds[i].abslt - 255 * GCOEFF : 0;
        leds[i].b = (leds[i].abslt > 255 * BCOEFF) ? leds[i].abslt - 255 * BCOEFF : 0;
    }
}

unsigned int VisualObject::sumP(unsigned int arr[])
{
    unsigned int sum = 0;
    for (unsigned int i = 0; i < OCT_N; i++) {
        if (!i)
            sum += arr[i];
        else sum += arr[i] * 2;
    }

    return sum;
}

float VisualObject::mapLogCol(unsigned int colCoeff) {
    const float a = 0.3F / logf(255);
    return a * logf(colCoeff + 1.0F) + 0.7F;
}

void VisualObject::distributeProportions() {
    unsigned int sum = 0;
    // Distribute LEDs equally.
    for (int i = 0; i < OCT_N; i++)
        ledProps[i] = NUM_LEDS / (4 * OCT_N);
    sum = sumP(ledProps);

    /*
    * LEDs can't always be equally distributed, so redistribute
    * first and last elements, to ensure distribution has the same
    * amount of LEDs as defined.
    */
    if (sum > NUM_LEDS / 2) {
        if ((NUM_LEDS / 2) % 2 ^ ledProps[0] % 2) {
            ledProps[OCT_N - 1] -= (sum - NUM_LEDS / 2) / 2;
            ledProps[0] -= 1;
        }
        else ledProps[OCT_N - 1] -= (sum - NUM_LEDS / 2) / 2;
    }
    else if (sum < NUM_LEDS / 2) {
        ledProps[0] += (NUM_LEDS / 2 - sum);
    }

    // Distribute LEDs again, giving largest significance to low frequencies.
    for (unsigned int i = 0; i < OCT_N - 1; i++) {
        unsigned int intm = (OCT_N / 2 - 1 + i * 2 > OCT_N - 1) ? OCT_N : OCT_N / 2 - 1 + i * 2;
        for (unsigned int j = i + 1; j < intm; j++) {
            if (!i && ledProps[j] > 1) {
                ledProps[i] += 2;
                ledProps[j] -= 1;
            }
            else if (ledProps[j] > 1) {
                ledProps[i] += 1;
                ledProps[j] -= 1;
            }
        }
    }
}