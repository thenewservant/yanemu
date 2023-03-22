
#include "sound.h"
#include "ScreenTools.h"

// Define your audio settings
const int sampleRate = 44100;
const int numChannels = 1;
const int bitsPerSample = 8;
const uint64_t bufferSize = 5 * sampleRate * numChannels * bitsPerSample / 8;

// Define your sine wave parameters
const double frequency = 440.0;
const double amplitude = 0.1;
#include <stdio.h>
#include <stdbool.h>
#include <math.h>


#define PI 3.14159265359

float duration = 0;
float freq = 440;

void audio_callback(void* userdata, Uint8* stream, int len)
{
    // Cast the stream to a short pointer
    //Sint16* snd = (Sint16*)stream;

    // Calculate the increment of the phase for each sample
    //float phase_inc = freq * PI * 2 / 48000;

    // Generate a sine wave for each sample in the stream
    workSound(stream, len);
}

int soundmain()
{
    // Initialize SDL audio


    // Set the audio callback function
    SDL_AudioSpec desired_spec, obtained_spec;
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = 1;
    desired_spec.samples = 4096;
    desired_spec.callback = audio_callback;
    desired_spec.userdata = NULL;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &obtained_spec, 0);
    if (dev < 0)
    {
        fprintf(stderr, "SDL_OpenAudio error: %s\n", SDL_GetError());
        return -1;
    }

    // Start the audio playback
    SDL_PauseAudioDevice(dev, 0);

    // Loop forever
    while (true)
    {
        // Generate a sine wave with a frequency of 440 Hz
        freq = 440;
        SDL_Delay(1);
    }

    // Clean up SDL audio
    SDL_CloseAudioDevice(dev);
    SDL_Quit();

    return 0;
}