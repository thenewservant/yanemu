#include "ScreenTools.h"
#include "ppu.h"
u32 *pixels = (u32*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));

void Screen::initSDLScreen() {

	window = SDL_CreateWindow("PPU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * ScreenScaleFactor, SCREEN_HEIGHT * ScreenScaleFactor, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	// Create SDL texture
	
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		exit(1);
	}
}

void Screen::endSDLApplication() {
	free(pixels);

	// Cleanup SDL
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}



Screen::Screen(u8 scaleFact) {
	//pixels = (u32*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u32));
	status = 0;
	ScreenScaleFactor = scaleFact;
	initSDLScreen();
}

u32* Screen::getPixelsPointer() {
	return pixels;
}

void Screen::updateScreen() {
	SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(Uint32));

	// Render texture and present on screen

	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}



void Screen::checkPressKey(SDL_Event event) {
	switch (event.key.keysym.sym) {
	case SDLK_LEFT:
		status |= 0b01000000;
		break;
	case SDLK_RIGHT:
		status |= 0b10000000;
		break;
	case SDLK_UP:
		status |= 0b00010000;
		break;
	case SDLK_DOWN:
		status |= 0b00100000;
		break;
	case SDLK_RETURN:
		status |= 0b00001000;
		break;
	case SDLK_r:
		status |= 0b00000001;
		break;
	case SDLK_t:
		status |= 0b00000010;
		break;
	case SDLK_SPACE:
		status |= 0b00000100;
		break;
	default:
		break;
	}
}

void Screen::checkRaiseKey( SDL_Event event) {
	switch (event.key.keysym.sym) {
	case SDLK_LEFT:
		status &= ~0b01000000;
		break;
	case SDLK_RIGHT:
		status &= ~0b10000000;
		break;
	case SDLK_UP:
		status &= ~0b00010000;
		break;
	case SDLK_DOWN:
		status &= ~0b00100000;
		break;
	case SDLK_RETURN:
		status &= ~0b00001000;
		break;
	case SDLK_r:
		status &= ~0b00000001;
		break;
	case SDLK_t:
		status &= ~0b00000010;
		break;
	case SDLK_SPACE:
		status &= ~0b00000100;
		break;
	default:
		break;
	}
}

u8 Screen::listener() {

	while (SDL_PollEvent(&keyEvent)) {
		switch (keyEvent.type) {
		case SDL_KEYDOWN:
				checkPressKey( keyEvent);
				pressKey(status, 0);
			break;

		case SDL_KEYUP:
			checkRaiseKey( keyEvent);
			pressKey(status, 0);
		case SDL_WINDOWEVENT:
			switch (keyEvent.window.event) {
			case SDL_WINDOWEVENT_CLOSE: 
				return 1;
			default:
				break;
			}
		default:
			break;
		}
	}

	
	updateScreen();
	SDL_Delay(2);
	//Sleep(10);
	return 0;
}

void Screen::writePixel(u32 where, u32 what) {
	pixels[where] = what;
}

Screen::Screen() {

}