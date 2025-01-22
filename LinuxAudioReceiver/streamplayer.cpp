#include <iostream>
#include <opus/opus.h>
#include <portaudio.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define FRAMES_PER_BUFFER 960
#define SERVER_PORT 8888
#define BUFFER_SIZE 4096

struct AudioData {
    unsigned char compressedData[BUFFER_SIZE];
};

void audioCallback(PaStream *stream, int serverSocket, sockaddr_in serverAddr) {
    socklen_t addrLen = sizeof(serverAddr);
    AudioData audioData;
    int16_t decodedBuffer[FRAMES_PER_BUFFER * CHANNELS];

    int opusError;
    OpusDecoder *decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &opusError);
    if (opusError != OPUS_OK) {
        std::cerr << "Failed to create Opus decoder: " << opus_strerror(opusError) << std::endl;
        return;
    }

    while (true) {
        ssize_t receivedBytes = recvfrom(serverSocket, audioData.compressedData, BUFFER_SIZE, 0, (sockaddr *)&serverAddr, &addrLen);
        if (receivedBytes < 0) {
            std::cerr << "Error receiving audio data" << std::endl;
            break;
        }

        int decodedSamples = opus_decode(decoder, audioData.compressedData, receivedBytes, decodedBuffer, FRAMES_PER_BUFFER, 0);
        if (decodedSamples < 0) {
            std::cerr << "Opus decoding failed: " << opus_strerror(decodedSamples) << std::endl;
            break;
        }

        if (Pa_WriteStream(stream, decodedBuffer, FRAMES_PER_BUFFER) != paNoError) {
            std::cerr << "Error writing to audio stream" << std::endl;
            break;
        }
    }

    opus_decoder_destroy(decoder);
}

int main() {
    if (Pa_Initialize() != paNoError) {
        std::cerr << "Failed to initialize PortAudio" << std::endl;
        return 1;
    }

    PaStream *stream;
    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice) {
        std::cerr << "No default output device available" << std::endl;
        Pa_Terminate();
        return 1;
    }

    outputParams.channelCount = CHANNELS;
    outputParams.sampleFormat = paInt16;
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    if (Pa_OpenStream(&stream, nullptr, &outputParams, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, nullptr, nullptr) != paNoError) {
        std::cerr << "Failed to open audio stream" << std::endl;
        Pa_Terminate();
        return 1;
    }

    if (Pa_StartStream(stream) != paNoError) {
        std::cerr << "Failed to start audio stream" << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(serverSocket);
        return 1;
    }

    audioCallback(stream, serverSocket, serverAddr);

    close(serverSocket);
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    return 0;
}
