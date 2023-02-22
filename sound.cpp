#include "sound.h"
#pragma comment(lib, "dsound.lib")

// Define your audio settings
const int sampleRate = 44100;
const int numChannels = 1;
const int bitsPerSample = 8;
const uint64_t bufferSize = 5*sampleRate * numChannels * bitsPerSample / 8;

// Define your sine wave parameters
const double frequency = 440.0;
const double amplitude = 0.1;

// Define the DirectAudio objects
LPDIRECTSOUND directSound;
LPDIRECTSOUNDBUFFER primaryBuffer;
LPDIRECTSOUNDBUFFER secondaryBuffer;

// Create a sine wave buffer
void createSineWaveBuffer()
{
    // Allocate memory for the buffer
    char* buffer = new char[bufferSize ];
    /*
    

    // Fll the buffer with the sine wave
    
    for (int i = 0; i < bufferSize; i += 2) {
        double time = (double)i / (double)sampleRate;
        short sample = (short)(amplitude * 32767.0 * (2*(sin(2 * 3.14159 * frequency * time))>0)-1);
        buffer[i] = (char)sample;
        
        buffer[i + 1] = (char)(sample >> 8);

    }*/

    //uint64_t dur;
    workSound(buffer, bufferSize);

    

    // Create a secondary sound buffer
    DSBUFFERDESC bufferDesc = {};
    bufferDesc.dwSize = sizeof(DSBUFFERDESC);
    bufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME;
    bufferDesc.dwBufferBytes = bufferSize;
    bufferDesc.lpwfxFormat = new WAVEFORMATEX();
    bufferDesc.lpwfxFormat->wFormatTag = WAVE_FORMAT_PCM;
    bufferDesc.lpwfxFormat->nChannels = numChannels;
    bufferDesc.lpwfxFormat->nSamplesPerSec = sampleRate;
    bufferDesc.lpwfxFormat->wBitsPerSample = bitsPerSample;
    bufferDesc.lpwfxFormat->nBlockAlign = bufferDesc.lpwfxFormat->nChannels * bufferDesc.lpwfxFormat->wBitsPerSample / 8;
    bufferDesc.lpwfxFormat->nAvgBytesPerSec = bufferDesc.lpwfxFormat->nSamplesPerSec * bufferDesc.lpwfxFormat->nBlockAlign;
    directSound->CreateSoundBuffer(&bufferDesc, &secondaryBuffer, NULL);

    // Lock the buffer and copy the audio data to it
    LPVOID audioPtr1 = nullptr, audioPtr2 = nullptr;
    DWORD audioBytes1 = 0, audioBytes2 = 0;
    secondaryBuffer->Lock(0, bufferSize, &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, 0);
    memcpy(audioPtr1, buffer, audioBytes1);
    if (audioPtr2 != nullptr) {
        memcpy(audioPtr2, buffer + audioBytes1, audioBytes2);
    }
    secondaryBuffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2);

    // Play the buffer
    secondaryBuffer->SetCurrentPosition(0);
    secondaryBuffer->Play(0, 0, 0);
}

int soundmain()
{
    // Create the DirectSound object
    HRESULT x = DirectSoundCreate(NULL, &directSound, NULL);

    // Set the cooperative level
    directSound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_NORMAL);

    // Create the primary sound buffer
    DSBUFFERDESC bufferDesc = {};
    bufferDesc.dwSize = sizeof(DSBUFFERDESC);
    bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, NULL);

    // Set the primary buffer format
    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = numChannels;
    waveFormat.nSamplesPerSec = sampleRate;
    waveFormat.wBitsPerSample = bitsPerSample;
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    primaryBuffer->SetFormat(&waveFormat);

    // Create the sine wave buffer
    createSineWaveBuffer();

    
    // Wait for the sound to finish playing
    
    
    secondaryBuffer->SetCurrentPosition(0);
    while (1) {
        
        Sleep(300);
        secondaryBuffer->Release();
        createSineWaveBuffer();
        secondaryBuffer->SetCurrentPosition(0);
    }

    // Release the objects
    secondaryBuffer->Release();
    primaryBuffer->Release();
    directSound->Release();

    return 0;
}