#include "ScreenTools.h"

u32 *pixels = (u32*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));

void Screen::initSDLScreen() {
	char tt[100];
	strcpy(tt, "Yanemu: ");
	strcat(tt, title);
	screenW = SCREEN_WIDTH * ScreenScaleFactor;
	screenH = SCREEN_HEIGHT * ScreenScaleFactor;
	window = SDL_CreateWindow(tt, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenW, screenH, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	
	//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	
	// Create SDL texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	
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

Screen::Screen(u8 scaleFact, const char* title) {
	//pixels = (u32*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u32));
	status = 0;
	ScreenScaleFactor = scaleFact;
	this->title = title;
	initSDLScreen();
}

u32* Screen::getPixels() {
	return pixels;
}

void Screen::updateScreen() {
	static bool fullScreenState = false;
	if (needFullScreenToggle) {
		printf("Toggle fullscreen\n");
		needFullScreenToggle = false;
		SDL_DestroyRenderer(renderer);
		SDL_DestroyTexture(texture);
		fullScreenState = !fullScreenState;
		SDL_SetWindowFullscreen(window, fullScreenState && SDL_WINDOW_FULLSCREEN_DESKTOP);

		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	}
	SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(Uint32));
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}


void Screen::checkPressKey(SDL_Event event) {
	switch (event.key.keysym.sym) {
	case SDLK_y:
		status |= 0b100000000;
		break;
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
	case SDLK_f:
		status |= 0b00000001;
		break;
	case SDLK_d:
		status |= 0b00000010;
		break;
	case SDLK_SPACE:
		status |= 0b00000100;
		break;
	case SDLK_i:
		specialCom();
		break;
	case SDLK_m:
		_nmi();
		break;
	case SDLK_c:
		changeMirror();
		break;
	case SDLK_h:
		rstCtrl();
		break;
	case SDLK_q:
		needFullScreenToggle = true;
		break;
	case SDLK_r:
		_rst();
		break;
	default:
		break;
	}
}

void Screen::checkRaiseKey( SDL_Event event) {
	switch (event.key.keysym.sym) {
		
	case SDLK_y:
		status &= ~0b100000000;
		break;
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
	case SDLK_f:
		status &= ~0b00000001;
		break;
	case SDLK_d:
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
			checkPressKey(keyEvent);
			pressKey(status);
			break;
		case SDL_KEYUP:
			checkRaiseKey(keyEvent);
			pressKey(status);
			break;
		case SDL_DROPFILE:
			printf("File dropped: %s\n", keyEvent.drop.file);
			newFilePath = keyEvent.drop.file;
			return 10;
			break;
		case SDL_WINDOWEVENT:
			switch (keyEvent.window.event) {
			case SDL_WINDOWEVENT_CLOSE:
				printf("Window closed\n");
				exit(0);
			case SDL_WINDOWEVENT_RESIZED:
				printf("Window resized\n");
				//resizeWindow();
				break;
			default:
				break;
			}
		default:
			break;
		}
	}

	SDL_Delay(2);
	return 0;
}

SDL_Window* Screen::getWindow() {
	return window;
}

void Screen::writePixel(u32 where, u32 what) {
	pixels[where] = what;
}

Screen::Screen() {

}

char* Screen::getFilePath() {
	return newFilePath;
}


void Screen::resizeWindow() {

}