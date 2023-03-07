#include "ppu.h"

uint32_t LUT[] = { 0, 0xA0402000, 0xD0A05000, 0xF0F08000 };
uint8_t* OAM = (uint8_t*)malloc(64 * 4 * sizeof(uint8_t));

void resetPixels(uint32_t* pixels) {

	for (uint64_t i = 0; i < (0xFFFF); i++) {
		//printf("%5X\n", i);
		pixels[i] = 0;
	}
}

void checkers(uint32_t* pixels) {


	uint32_t i2 = 0;
	for (int y = 0; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			int index = y * SCREEN_WIDTH + x;
			i2 = (i2 + 1) % (SCREEN_WIDTH * SCREEN_HEIGHT);

			if (index == 0xC000) {
				pixels[index] = 0xFF000000;
			}
			if (index < 0x1FFF) {

				//pixels[index] = 0xFFFFFF00;
				pixels[index] = (Uint32)(chrrom[i2] << 8 | chrrom[i2] << 16);
			}


			//pixels[index] = (Uint32)((i2 < 0xFF) ? ram[i2] << 24 : (ram[i2] << 8 | ram[i2] << 16));

		}
	}
}

void showNameTable(uint32_t* pixels) {

}

void updateOam() { // OAMDMA (0x4014) processing routine
	if (V_BLANK) {
		for (uint8_t i = 0; i < 0xFF; i++) {
			OAM[i] = ram[(ram[0x4014] << 8) | i];
		}
	}
}

void drawSprite(uint32_t* pixels, uint8_t yy, uint8_t id,  uint8_t xx, uint8_t params=0, uint8_t palette=0) {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			uint16_t cur = (yy + j) * SCREEN_WIDTH + xx + i;
			pixels[cur] = 0xFF00FF00;

			int y = (yy + j);
			int x = xx + i;
			uint16_t baseAd = (id & 1) ? (0x1000 | (id >> 1)) : (id >> 1);
			pixels[cur] = LUT[((chrrom[(16* baseAd)+i] >> ((7 - j)))&1) + ((chrrom[(16 * baseAd) + i+8] >> ((7 - j))) &1)];
		}
	}
}

void showOam(uint32_t* pixels) {
	uint32_t i2 = 0;
	
	resetPixels(pixels);

	for (uint8_t sprite = 0; sprite < 64; sprite++) { // sprite is OAM iterator
		if ((OAM[sprite * 4] < 0xEF) && (OAM[(3 + sprite) * 4]<0xF9))
			drawSprite(pixels, OAM[sprite * 4], OAM[(1 + sprite) * 4], OAM[(3 + sprite) * 4]);
		//pixels[SCREEN_WIDTH  * OAM[sprite * 4] +   OAM[(3+sprite) * 4]] = 0xFFFF0000;
	}


}

void showTiles(uint32_t* pixels) {
	uint32_t i2 = 0;
	for (int y = 0; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			int index = y * SCREEN_WIDTH + x;
			i2 = (i2 + 1) % (SCREEN_WIDTH * SCREEN_HEIGHT);
			if (1) {
				pixels[y * SCREEN_WIDTH + x] = LUT[((chrrom[8 * (y / 8) + y + (x / 8) * 2 * SCREEN_HEIGHT] >> ((7 - (x % 8)))) & 1) | (((chrrom[8 * (y / 8) + y + 8 + (x / 8) * 2 * SCREEN_HEIGHT] >> ((7 - (x % 8)))) & 1) << 1)];
			}
		}
	}
}

void finalRender(uint32_t* pixels) {
	uint32_t i2 = 0;
	if (0)checkers(pixels);
	if (0)showTiles(pixels);
	if (0)showNameTable(pixels);
	if (1)updateOam();
	if (1)showOam(pixels);
	//if (ram[0x4014])printf("OAMDMA, %2X", ram[0x4014]);
	if (ram[0x2006])printf("%2X\n", ram[0x2006]);
	//printf("\n\n%X:%X:%X:%X:%X:%X:%X\n", ram[0x2000] , ram[0x2001], ram[0x2003], ram[0x2004], ram[0x2005], ram[0x2006], ram[0x2007], ram[0x4014]);
}

