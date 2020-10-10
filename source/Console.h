#pragma once
#include "nsmb.h"

extern "C" int strlen(const char* __s);

#define CONSOLE_USE_BIN_GFX 0

namespace Console {

	enum Alignment {
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT,
	};

	enum Color {
		COLOR_WHITE,
		COLOR_RED,
		COLOR_GREEN,
		COLOR_YELLOW,
		COLOR_BLUE,
		COLOR_PURPLE,
		COLOR_LIGHT_BLUE,
		COLOR_GRAY,
		COLOR_UNUSED_1,
		COLOR_UNUSED_2,
		COLOR_UNUSED_3,
		COLOR_UNUSED_4,
		COLOR_UNUSED_5,
		COLOR_UNUSED_6,
		COLOR_UNUSED_7,
		COLOR_UNUSED_8,
	};

	void gotox(u8 x);
	void gotoy(u8 y);
	void gotoxy(u8 x, u8 y);

	void print(const char* fmt, ...);
	void printxy(u8 x, u8 y, const char* fmt, ...);
	void printxyc(u8 x, u8 y, Color col, const char* fmt, ...);

	void align(Alignment alignment);
	void setTextColor(Color color);
	void setBackdropColor(GXRgb color);

	void clear(u16 c = ' ');
	void update();
	void init();

	void backupVram();
	void restoreVram();
}
#pragma once