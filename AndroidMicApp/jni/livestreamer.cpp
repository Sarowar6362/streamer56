#include <iostream>
#include <opus/opus.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define FRAMES_PER_BUFFER 960
#define OPUS_BITRATE 64000
#define SERVER_PORT 8888
#define CLIENT_IP "127.0.0.1"

struct AudioData {
    unsigned char compressedData[4096];
};

int main() {
    // PulseAudio configuration
    pa_simple *paStream = nullptr;
    pa_sample_spec sampleSpec = {PA_SAMPLE_S16LE, SAMPLE_RATE, CHANNELS};
    int paError;

    paStream = pa_simple_new(nullptr, "AudioStreamer", PA_STREAM_RECORD, nullptr, "Audio Capture", &sampleSpec, nullptr, nullptr, &paError);
    if (!paStream) {
        std::cerr << "PulseAudio initialization failed: " << pa_strerror(paError) << std::endl;
        return 1;
    }

    // Opus encoder setup
    int opusError;
    OpusEncoder *encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_AUDIO, &opusError);
    if (opusError != OPUS_OK) {
        std::cerr << "Failed to create Opus encoder: " << opus_strerror(opusError) << std::endl;
        return 1;
    }
    opus_encoder_ctl(encoder, OPUS_SET_BITRATE(OPUS_BITRATE));

    // Setup UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, CLIENT_IP, &clientAddr.sin_addr);

    AudioData audioData;
    int16_t buffer[FRAMES_PER_BUFFER * CHANNELS];

    std::cout << "Streaming audio to " << CLIENT_IP << ":" << SERVER_PORT << std::endl;

    while (true) {
        if (pa_simple_read(paStream, buffer, sizeof(buffer), &paError) < 0) {
            std::cerr << "Error reading from PulseAudio: " << pa_strerror(paError) << std::endl;
            break;
        }

        int compressedSize = opus_encode(encoder, buffer, FRAMES_PER_BUFFER, audioData.compressedData, sizeof(audioData.compressedData));
        if (compressedSize < 0) {
            std::cerr << "Opus encoding failed: " << opus_strerror(compressedSize) << std::endl;
            break;
        }

        ssize_t sentBytes = sendto(sock, audioData.compressedData, compressedSize, 0, (sockaddr *)&clientAddr, sizeof(clientAddr));
        if (sentBytes < 0) {
            std::cerr << "Error sending audio data" << std::endl;
            break;
        }
    }

    close(sock);
    opus_encoder_destroy(encoder);
    pa_simple_free(paStream);

    return 0;
}
