#pragma once
#ifndef SCREENTOOLS_H
#define SCREENTOOLS_H
#include <SDL.h>
#include <cstdio>
#include "types.h"
#include "simpleCPU.h"
#include "Rom.h"

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 224

#define FILE_DROPPED 184

extern u32* pixels; // real screen

class Screen {
private:
	char title[150];
	Rom* rom;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	u8 ScreenScaleFactor;
	u16 status;
	bool needFullScreenToggle = false;
	SDL_Event keyEvent;
	u16 screenW, screenH;
private:
	void initSDLScreen();
	char* newFilePath;
public:
	Screen(u8 scaleFact);
	u32* getPixels();
	void updateScreen();
	void checkPressKey(SDL_Event event);
	void checkRaiseKey(SDL_Event event);
	void endSDLApplication();
	u8 listener();
	SDL_Window* getWindow();
	void writePixel(u32 where, u32 what);
	char* getFilePath();
	void setRom(Rom* romPtr){
		this->rom = romPtr;
		strcpy(title, "Yanemu - ");
		strcat(title, rom->getfileName());
		SDL_SetWindowTitle(window, title);
	};
};

#endif