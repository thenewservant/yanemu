#pragma once
#ifdef _WIN32
	#include <windows.h>
	#include <mmsystem.h>
	#include <dsound.h>
#endif

#include <math.h>
#include "firstAPU.h"
#include <stdio.h>

void createSineWaveBuffer();
int soundmain();