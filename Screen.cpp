#include "ScreenTools.h"

u32 *pixels = (u32*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));

void Screen::initSDLScreen() {

	window = SDL_CreateWindow("PPU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * ScreenScaleFactor, SCREEN_HEIGHT * ScreenScaleFactor, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	// Create SDL texture
	
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
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

u8 keyIsDown = 0;

u8 Screen::listener() {
	SDL_Event event;
	/* handle your event here */
	int kkk;
	kkk = SDL_PollEvent(&event);
	switch (event.type) {
	case SDL_KEYDOWN:
		if (!keyIsDown) {
			keyIsDown = 1;
			printf("KEYDOWNNEUHH\n");

		}
		break;
	case SDL_KEYUP:
		keyIsDown = 0;
		printf("KEYRELEASE\n");
	default:
		break;
	}

	//ram[0x4016] = 

	SDL_UpdateTexture(texture, NULL,pixels, SCREEN_WIDTH * sizeof(Uint32));
	
	// Render texture and present on screen

	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	Sleep(10);
	//User requests quit
	//if (event.type == SDL_QUIT)
	return 0;
}

void Screen::writePixel(u32 where, u32 what) {
	pixels[where] = what;
}

Screen::Screen() {

}