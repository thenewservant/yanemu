#include "ScreenTools.h"

u32 *pixels = (u32*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));

void Screen::initSDLScreen() {
	screenW = SCREEN_WIDTH * ScreenScaleFactor;
	screenH = SCREEN_HEIGHT * ScreenScaleFactor;
	window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenW, screenH, SDL_WINDOW_SHOWN | 0 * SDL_WINDOW_RESIZABLE);
	
	//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	
	// Create SDL texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	//int pitch = SCREEN_WIDTH * SCREEN_HEIGHT * 4;
	//SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		exit(1);
	}
}

void Screen::endSDLApplication() {
	// Cleanup SDL
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

Screen::Screen(u8 scaleFact) {
	status = 0;
	ScreenScaleFactor = scaleFact;
	initSDLScreen();
}

u32* Screen::getPixels() {
	return pixels;
}

void Screen::updateScreen() {
	//SDL_UnlockTexture(texture);
	SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(Uint32));
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	//int pitch = SCREEN_WIDTH * SCREEN_HEIGHT * 4;
	//SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
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
				rom->saveWorkRam();
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

char* Screen::getFilePath() {
	return newFilePath;
}
