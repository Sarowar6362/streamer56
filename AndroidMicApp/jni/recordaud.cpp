#include <iostream>
#include <portaudio.h>
#include <sndfile.h>
#include <cstring>

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define FRAMES_PER_BUFFER 256
#define RECORD_SECONDS 5

struct AudioData {
    float* buffer;
    size_t maxFrames;
    size_t currentFrame;
};

static int RecordCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData) {
    AudioData* data = (AudioData*)userData;
    const float* input = (const float*)inputBuffer;

    if (inputBuffer == nullptr) {
        return paContinue;
    }

    size_t framesToCopy = std::min((size_t)framesPerBuffer, data->maxFrames - data->currentFrame);
    memcpy(data->buffer + data->currentFrame * NUM_CHANNELS, input, framesToCopy * NUM_CHANNELS * sizeof(float));
    data->currentFrame += framesToCopy;

    return (data->currentFrame >= data->maxFrames) ? paComplete : paContinue;
}

int main() {
    PaError err;
    SNDFILE* outputFile;
    SF_INFO sfInfo;

    size_t totalFrames = SAMPLE_RATE * RECORD_SECONDS;
    AudioData data;
    data.buffer = new float[totalFrames * NUM_CHANNELS];
    data.maxFrames = totalFrames;
    data.currentFrame = 0;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Failed to initialize PortAudio: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    // Open audio stream
    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream, NUM_CHANNELS, 0, paFloat32, SAMPLE_RATE,
                               FRAMES_PER_BUFFER, RecordCallback, &data);
    if (err != paNoError) {
        std::cerr << "Failed to open stream: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    // Start recording
    std::cout << "Recording for " << RECORD_SECONDS << " seconds..." << std::endl;
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "Failed to start stream: " << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    while ((err = Pa_IsStreamActive(stream)) == 1) {
        Pa_Sleep(100);
    }
    if (err < 0) {
        std::cerr << "Stream error: " << Pa_GetErrorText(err) << std::endl;
    }

    // Stop and close stream
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    std::cout << "Recording finished. Writing to file..." << std::endl;

    // Save to WAV file
    sfInfo.samplerate = SAMPLE_RATE;
    sfInfo.channels = NUM_CHANNELS;
    sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    outputFile = sf_open("output.wav", SFM_WRITE, &sfInfo);
    if (!outputFile) {
        std::cerr << "Failed to open output file: " << sf_strerror(nullptr) << std::endl;
        delete[] data.buffer;
        return -1;
    }

    sf_writef_float(outputFile, data.buffer, data.currentFrame);
    sf_close(outputFile);

    std::cout << "Audio written to 'output.wav' successfully!" << std::endl;

    delete[] data.buffer;
    return 0;
}
