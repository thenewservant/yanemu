
#include "ppu.h"
#include "simpleCPU.hpp"

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
void initSDL();

extern int prop2 = 3;
int main(int argc, char* argv[]) {

	SDL_Window* window = SDL_CreateWindow("PPU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * prop2, SCREEN_HEIGHT * prop2, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	// Create SDL texture

	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		exit(1);
	}

	// Create SDL window and renderer
	// Create pixel buffer
	Uint32* pixels = (Uint32*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Uint32));
	uint32_t cnt = 0;
	// Fill pixel buffer with red and black checkers
	
	std::thread tcpu(mainCPU);

	auto start = std::chrono::high_resolution_clock::now();
	int zzz = 0;
	SDL_Event event;
	Sleep(3);
	while (1) {

		zzz += 1;
		int kkk;
		finalRender(pixels);
		{
			kkk = SDL_PollEvent(&event);
			SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(Uint32));
			Sleep(10);
			// Render texture and present on screen

			SDL_RenderCopy(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);

			if (ram[0x2000] & N_FLAG) NULL;
			if (0)
			{
				if (((zzz % 800) == 0) && (ram[0x2000] & N_FLAG)) {
					_nmi();
					ram[0x2002] |= 0x80;
				}
				if (((zzz % 800) == 5) && (ram[0x2000] & N_FLAG)) {
					ram[0x2002] &= 0b01111111;
				}

			}
			//ram[0x4000] = 0b11001011;
		}
		//Sleep(0);
	}

	
	// Free memory
	free(pixels);

	// Cleanup SDL
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}