#pragma once
#ifndef PPU_H
#define PPU_H
#include "simpleCPU.hpp"
#include <SDL.h>

#define WINDOW_WIDTH 1000
#define V_BLANK ram[0x2000] & N_FLAG

const int SCREEN_WIDTH = 260;
const int SCREEN_HEIGHT = 260;


void finalRender(uint32_t* pixels);

#endif