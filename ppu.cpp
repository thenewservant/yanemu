#include "ppu.h"

uint32_t LUT[][4] = { {0, 0xA0402000, 0xD0A05000, 0xF0F08000} ,
						{0, 0xA0002F00, 0xF0FF1000, 0x4444FF00} ,
						{0, 0x50402000, 0x52115000, 0x55F08000} ,
						{0, 0xA0002000, 0xD0AF9000, 0xF0028000} };
int32_t scanLine = 0, cycle = 0;
u8* ppuRam;// = (u8*)calloc(1 << 15, sizeof(u8));
u16 ppuADDR = 0;
u16 oamADDR = 0;

void copyChrRom(u8* chr) {
	for (u16 t = 0; t < (1 << 13); t++) {
		ppuRam[t] = chr[t];
	}
}

void PPU::resetPixels() {
	for (uint64_t i = 0; i < (0xFFFF); i++) {
		//printf("%5X\n", i);
		pixels[i] = 0;
	}
}

void PPU::showNameTable() {
}

void PPU::updateOam() { // OAMDMA (0x4014) processing routine
	if (V_BLANK) {
		for (uint8_t i = 0; i < 0xFF; i++) {
			OAM[i] = ram[(ram[0x4014] << 8) | i];
		}
	}
}

void PPU::drawSprite(uint8_t yy, uint8_t id, uint8_t xx, uint8_t byte2, uint8_t palette) {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			uint16_t cur = (yy + j) * SCREEN_WIDTH + xx + i;
			pixels[cur] = 0xFF00FF00;

			int y = (yy + j);
			int x = xx + i;
			uint16_t baseAd = (id & 1) ? (0x1000 | (id >> 1)) : (id >> 1);
			pixels[cur] = LUT[byte2 & 3][((chrrom[(16 * baseAd) + i] >> ((7 - j))) & 1) + ((chrrom[(16 * baseAd) + i + 8] >> ((7 - j))) & 1)];
		}
	}
}

void PPU::showOam() {
	uint32_t i2 = 0;

	//resetPixels();

	for (uint8_t sprite = 0; sprite < 64; sprite++) { // sprite is OAM iterator
		if ((OAM[sprite * 4] < 0xEF) && (OAM[(3 + sprite) * 4] < 0xF9))
			drawSprite(OAM[sprite * 4], OAM[(1 + sprite) * 4], OAM[(3 + sprite) * 4], OAM[(2 + sprite) * 4]);
		//pixels[SCREEN_WIDTH  * OAM[sprite * 4] +   OAM[(3+sprite) * 4]] = 0xFFFF0000;
	}

}

void PPU::cac() {
	for (u16 t = 0; t < 100; t++) {
		if (t % 2) {
			pixels[t] = 0xFF00FF00;
		}
		else {
			pixels[t] = 0xFFFF0000;
		}
	}
}

void PPU::finalRender() {
	if (0)updateOam();
	if (0)showNameTable();
	if (0)showOam();
	if (0)cac();
	//scr.updateScreen();
	//if (ram[0x4014])printf("OAMDMA, %2X", ram[0x4014]);
	//if (ram[0x2006])printf("%2X\n", ram[0x2006]);
	//printf("\n\n%X:%X:%X:%X:%X:%X:%X\n", ram[0x2000] , ram[0x2001], ram[0x2003], ram[0x2004], ram[0x2005], ram[0x2006], ram[0x2007], ram[0x4014]);
}

void TELLNAMETABLE() {
	printf("\n\n\nNAMETABLE\n\n\n");
	for (int i = 0; i < 32 * 30; i++) {
		printf("%X:%X\n", i, ppuRam[0x2000 + i]);
	}
}

//stands for NameTable Prefix -- add that to needed ppuRAM address suffix
#define NT_PREFIX (0x2000 + (ram[0x2000] & 3) * 0x400)
#define PAT_TB_PREFIX (((ram[0x2000] & 0b00010000) > 0) ? 0x1000 : 0)
#define EXAMPLE 0 // flex

void drawNameTable(u16 cycle, u8 scanl) {
	if (EXAMPLE) {
		if (cycle < 256) {
			if (scanl - 1) {
				for (int k = 0; k < 8; k++) {

					pixels[(scanl - 1) * SCREEN_WIDTH + cycle + k] = 0x00111100;
				}
			}
			for (int k = 0; k < 8; k++) {

				pixels[scanl * SCREEN_WIDTH + cycle + k] = 0xFF00FF00;
			}
		}
	}
	else {
		for (int k = 0; k < 8; k++) {
			if (((scanl * SCREEN_WIDTH + cycle + k) < SCREEN_WIDTH * SCREEN_HEIGHT) && (ram[0x2001] & 0b00001000)) {
				//printf("%X\n", NT_PREFIX + 8 * (cycle / 8) + 32 * scanl);
				pixels[scanl * SCREEN_WIDTH + cycle + k] = ((ppuRam[(scanl % 8) + PAT_TB_PREFIX + 16 * ppuRam[NT_PREFIX + (cycle / 8) + 32 * (scanl / 8)]] >> (7 - k)) & 1) * 1100000 +




																 ((ppuRam[(scanl % 8) + 8 + PAT_TB_PREFIX + 16 * ppuRam[NT_PREFIX + (cycle / 8) + 32 * (scanl / 8)]] >> (7 - k)) & 1) * 1111101000;

				//if (k == 7) pixels[scanl * SCREEN_WIDTH + cycle] = 0x83894000;
			}
		}
	}
}

int imgcnt = 0;

void PPU::sequencer() {

	if (((cycle % 8) == 0) && (scanLine < 240) && (cycle < 256)) {
		drawNameTable(cycle, scanLine);
		//updateOam();
		//printf("here");

	}
	if ((cycle == 1) && (scanLine == 241)) {
		//TELLNAMETABLE();
		int donmi = 0;
		ram[0x2002] |= 0b10000000;
		/*if (!donmi) {
			donmi++;
		}
		else {

		}*/

		_nmi();




	}

	if ((cycle == 1) && (scanLine == 261)) {
		//printf("wut");
		ram[0x2002] &= ~0b11100000;
	}
}


void PPU::tick() {
	cycle++;

	if (cycle / 341) {
		//printf("\n\ncycle %d SCANLINE %d ppuADDR %2X", cycle, scanLine, ppuADDR);
		scanLine++;
		cycle = 0;

	}

	scanLine %= 262;
	sequencer();
	//printf("\n%d, %d\n", cycle, scanLine);

	//finalRender();
}


PPU::PPU(u32* px, Screen sc, Rom rom) {
	ram[0x2002] = 0b10000000;
	scanLine = -1;
	cycle = 0;
	scr = sc;
	ppuADDR = 0;
	ppuRam = (u8*)calloc(1 << 16, sizeof(u8));
	pixels = px;
	copyChrRom(rom.getChrRom());
	printf("ok!\n\n");
	Sleep(300);
}

void whatInTheHellIsThat() {
	/*
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
			if (1)
			{
				if (((zzz % 100) == 0) && (ram[0x2000] & N_FLAG)) {
					_nmi();
					ram[0x2002] |= 0x80;
				}
				if (((zzz % 100) == 5) && (ram[0x2000] & N_FLAG)) {
					ram[0x2002] &= 0b01111111;
				}

			}
			//ram[0x4000] = 0b11001011;
		}
		//Sleep(0);
	}
	*/
}
/*
void PPU::showTiles() {
	uint32_t i2 = 0;
	for (int y = 0; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			int index = y * SCREEN_WIDTH + x;
			i2 = (i2 + 1) % (SCREEN_WIDTH * SCREEN_HEIGHT);
			if (1) {
				pixels[y * SCREEN_WIDTH + x] = LUT[0][((chrrom[8 * (y / 8) + y + (x / 8) * 2 * SCREEN_HEIGHT] >> ((7 - (x % 8)))) & 1) | (((chrrom[8 * (y / 8) + y + 8 + (x / 8) * 2 * SCREEN_HEIGHT] >> ((7 - (x % 8)))) & 1) << 1)];
			}
		}
	}
}

*/


/*

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
			if (1)
			{
				if (((zzz % 100) == 0) && (ram[0x2000] & N_FLAG)) {
					_nmi();
					ram[0x2002] |= 0x80;
				}
				if (((zzz % 100) == 5) && (ram[0x2000] & N_FLAG)) {
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
*/

void writePPU(u8 what) {
	ppuRam[ppuADDR] = what;
	//printf("what?: %X actually : %X\n", what, ppuRam[ppuADDR]);
	incPPUADDR();
	//printf("\nppuADDR:%X", ppuADDR);
}

void incPPUADDR() {
	if (ram[0x2000] & 4) {
		ppuADDR += 32;
	}
	else {
		ppuADDR++;
	}
}