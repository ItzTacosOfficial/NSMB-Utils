#include "Console.h"

static GXBg01Control bg0ctr;
static int visiblePlane;
static u8* vramBackup;

namespace Console
{
	namespace {

		const int maxW = 32;
		const int maxH = 32;

		struct ConsoleContext {
			u16 textBuf[maxW * maxH];
			u8 col;
			u8 row;
			Color color;
			Alignment alignment;

			u8 tabSize = 2;

			inline u32 getCursorPosition() {
				return col + row * maxW;
			}

			void checkCursorOutOfBounds_w() {
				if (col > maxW) {
					col = 0;
					row++;

					checkCursorOutOfBounds_h();
				}
			}
			void checkCursorOutOfBounds_h() {
				if (row > maxH) {
					row = maxH;
					scrollTextUp();
				}
			}

			// scrolls the cursor right
			void updateCursor() {
				col++;
				checkCursorOutOfBounds_w();
			}

			// scrolls the cursor down
			void newLine() {
				col = 0;
				row++;
				checkCursorOutOfBounds_h();
			}

			// scrolls the cursor right by tabSize
			void tab() {
				col += tabSize;
				checkCursorOutOfBounds_w();
			}

			void scrollTextUp() {
			}
		};

		ConsoleContext console;

		// private
		void _vaprint(const char* fmt, va_list va) {
			char dst[32 * 24];

			OS_VSPrintf(dst, fmt, va);

			int len = strlen(dst);

			s8 x = console.col;

			// set alignment
			switch (console.alignment) {
			case ALIGN_LEFT:
				break;

			case ALIGN_CENTER:
				x = 16 - (len / 2);
				if (len & 1) x--;
				if (x < 0) x = 0;
				break;

			case ALIGN_RIGHT:
				x = 31 - len - 1;
				if (x < 0) x = 0;
				break;
			}

			console.col = x;

			int i = 0;

			while (dst[i] != '\x0') {
				char c = dst[i++];

				// insert char into console buffer
				switch (c) {
				case '\n':
					console.newLine();
					break;
				case '\t':
					console.tab();
					break;
				default:
				{
					int j = console.getCursorPosition();
					console.textBuf[j] = c + console.color;
					console.updateCursor();
					break;
				}
				}
			}
		}
	}

	// public
	void gotox(u8 x) {
		if (x >= 32) {
			return;
		}

		console.col = x;
	}
	void gotoy(u8 y) {
		if (y >= 24) {
			return;
		}

		console.row = y;
	}
	void gotoxy(u8 x, u8 y) {
		gotox(x);
		gotoy(y);
	}

	void print(const char* fmt, ...) {
		va_list va;
		va_start(va, fmt);
		_vaprint(fmt, va);
		va_end(va);
	}
	void printxy(u8 x, u8 y, const char* fmt, ...) {
		gotoxy(x, y);

		va_list va;
		va_start(va, fmt);
		_vaprint(fmt, va);
		va_end(va);
	}

	void align(Alignment alignment) {
		console.alignment = alignment;

		switch (console.alignment) {
		case ALIGN_LEFT:
			console.col = 0;
			break;
		default:
			break;
		}
	}
	void setTextColor(Color color) {
		console.color = color;
	}
	void setBackdropColor(GXRgb color) {
		*(u16*)HW_DB_BG_PLTT = color;
	}

	void clear(u16 c) {
		MI_CpuFill16(console.textBuf, c, 32 * 24 * sizeof(u16));

		gotoxy(0, 0);
		align(ALIGN_LEFT);
	}
	void update() {
		u16* bg0s = (u16*)G2S_GetBG0ScrPtr();

		for (int i = 0; i < 32 * 24; i++) {
			bg0s[i] = console.textBuf[i];
		}
	}

	void init()
	{
		// initalize graphics for sub screen
		G2S_SetBG0Control(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x2000, GX_BG_CHARBASE_0x00000, GX_BG_EXTPLTT_01);
		GXS_SetVisiblePlane(GX_PLANEMASK_BG0);

		u16* bg0c = (u16*)G2S_GetBG0CharPtr();

		// clear bg0 char data
		//MI_CpuClearFast(bg0c, 0x800 * 32);

#if CONSOLE_USE_BIN_GFX
		// resources
		void* gfx = (void*)0x2039218;
		void* pltt = (void*)0x2039838;

		// load resources
		MI_UncompressLZ16(gfx, bg0c + 0x200);
		GXS_LoadBGPltt(pltt, 0x1A0, 0x60);
#else
		int dma = FS_GetDefaultDMA();
		FS_SetDefaultDMA(-1);

		// resources
		void* pltt = nFS_LoadFileByID(2090 - 131);

		// load resources
		nFS_LoadFileByIDToDest(2089 - 131, bg0c + 0x200);
		GXS_LoadBGPltt(pltt, 0x1A0, 0x60);

		// free palette
		NSMB_FreeToGameHeap(pltt);

		FS_SetDefaultDMA(dma);
#endif
		// reset console params
		console.row = 0;
		console.col = 0;
		console.alignment = ALIGN_LEFT;
		console.color = COLOR_WHITE;
		setBackdropColor(0);

		// clear console buffer
		clear(0);

		// copy buffer to VRAM
		update();
	}

	void backupVram()
	{
		bg0ctr = G2S_GetBG0Control(); //Saves the background control
		visiblePlane = GXS_GetVisiblePlane(); //Saves the graphic visible plane settings

		vramBackup = (u8*)NSMB_AllocFromGameHeap(0x1C00 + 0x2600 + 0x200);
		MI_CpuCopy32((void*)0x6200400, vramBackup, 0x1C00);
		MI_CpuCopy32((void*)0x6202000, vramBackup + 0x1C00, 0x2600);
		MI_CpuCopy32((void*)0x50005A0, vramBackup + 0x1C00 + 0x2600, 0x200);
	}

	void restoreVram()
	{
		G2S_SetBG0Control(
			static_cast<GXBGScrSizeText>(bg0ctr.screenSize),
			static_cast<GXBGColorMode>(bg0ctr.colorMode),
			static_cast<GXBGScrBase>(bg0ctr.screenBase),
			static_cast<GXBGCharBase>(bg0ctr.charBase),
			static_cast<GXBGExtPltt>(bg0ctr.bgExtPltt)
		);
		GXS_SetVisiblePlane(visiblePlane);

		MI_CpuCopy32(vramBackup, (void*)0x6200400, 0x1C00);
		MI_CpuCopy32(vramBackup + 0x1C00, (void*)0x6202000, 0x2600);
		MI_CpuCopy32(vramBackup + 0x1C00 + 0x2600, (void*)0x50005A0, 0x200);
		NSMB_FreeToGameHeap(vramBackup);
	}
}
