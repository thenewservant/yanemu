#include "Mapper.h"

u8 Mapper::rdNT(u16 where) {
	u8 ntID = (where >> 10) & 3;
	switch (mirror) {
	case HORIZONTAL:
		return nameTables[(ntID >> 1)][where & 0x03FF];
		break;
	case VERTICAL:
		return nameTables[(ntID & 1)][where & 0x03FF];
		break;
	default:
		break;
	}
	return 0;
}

void Mapper::wrNT(u16 where, u8 what) {
	u8 ntID = (where >> 10) & 3;
	switch (mirror) {
	case HORIZONTAL:
		nameTables[(ntID >> 1)][where & 0x03FF] = what;
		break;
	case VERTICAL:
		nameTables[(ntID & 1)][where & 0x03FF] = what;
		break;
	default:
		break;
	}
}
void Mapper::setMirroring(u8 mir) {
	mirror = mir;
}