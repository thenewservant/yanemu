#include "sound.h"

SDL_AudioDeviceID dev;

int soundmain() {

    // Set the audio callback function
    SDL_AudioSpec desired_spec, obtained_spec;
    desired_spec.freq = 44700;
    desired_spec.format = AUDIO_F32SYS;
    desired_spec.channels = 1;
    desired_spec.callback = NULL;// (SDL_AudioCallback)audio_callback;
    desired_spec.userdata = NULL;

     dev = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &obtained_spec, 0);
    if (dev < 0)
    {
        fprintf(stderr, "SDL_OpenAudio error: %s\n", SDL_GetError());
        return -1;
    }
    // Start the audio playback
    SDL_PauseAudioDevice(dev, 0);
    printf("\nAPU started.");
    return 0;
}
