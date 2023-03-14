#pragma once
#ifndef SCREENTOOLS_H
#define SCREENTOOLS_H
#include <SDL.h>
#include <cstdio>
#include "simpleCPU.hpp"

#define SCREEN_WIDTH 260
#define SCREEN_HEIGHT 240

extern u32* pixels; // real screen

class Screen {
private:
	//uint32_t* pixels;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	u8 ScreenScaleFactor;
	void initSDLScreen();
	//void threadWindowOpen();

public:
	Screen();
	Screen(u8 scaleFact);
	u32* getPixelsPointer();
	void updateScreen();
	void endSDLApplication();
	u8 listener();
	void writePixel(u32 where, u32 what);
};

#endif