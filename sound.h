#pragma once
#ifndef SOUND_H
#define SOUND_H
#include "simpleCPU.h"
#include "firstAPU.h"
#include <stdio.h>

int soundmain();
extern SDL_AudioDeviceID dev;
SDL_AudioDeviceID getDev();
#endif