#include "nsmb.h"
#include "Console.h"
#include "Cheats.h"

#ifdef __INTELLISENSE__
#define __attribute__(x)
#endif

#define LONG_LINE "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90" //Overblade likes this element
#define KEY_A "\xE0"
#define KEY_B "\xE1"
#define KEY_X "\xE2"
#define KEY_Y "\xE3"
#define KEY_SELECT "\xE7\xE8\xE9\xEA"
#define KEY_L "\xE4"
#define KEY_R "\xE5"
#define CURSOR1 "\xF1"
#define CURSOR2 "\xF2"
#define SWITCHON "\xEB\xEC"
#define SWITCHOFF "\xED\xEE"
#define WRENCH "\xEF\xF0"

static int& playerNumber = *(int*)0x02085A7C;

static int selectedMenu[2] = { 0 };
static int selectedCheat[2] = { 0 }; //selected cheat int for the menu
static bool menuOpened[2] = { false }; //console activated bool
static bool disableMenu[2] = { false };

static u32 AVtempAddress = 0x0000000;
static int AVcurDigit = 6;
static int AVcurAddress = 0;
static u32 AVdigitMask = 0xF << (AVcurDigit * 4);
static u32 AVdigitOne = 1 << (AVcurDigit * 4);
static u32 AVadrNoDgt = AVtempAddress & ~AVdigitMask;
static u32 AVadrDgt = AVtempAddress & AVdigitMask;

static u32 RVaddress = 0x00000000;
static u32 RVreadMode = 0; // 0 = 8bit, 1 = 16bit, 2 = 32bit
static u32 RVcurDigit = 0;
static bool RVColor = false;

typedef void(*TextUpdater)();
typedef void(*GetControls)();

enum class MenuType
{
	Main,
	Cheats,
	CheatsInfo,
	AddrViewer,
	AddrViewerAdd,
	RamViewer
};

MenuType menuType[2] = { MenuType::Main };

static const char* cheatNames[CHEAT_COUNT] = {
	"Fly",
	"Anim Random",
	"Powerup Cycle",
	"Slow Fall",
	"Starman Switch",
	"Kill Switch",
	"Fireball Abuse",
	"Fast Move",
	"Wall Clip",
	"Size Changer",
};

static const char* cheatDescriptions[CHEAT_COUNT] = {
	"Hold " KEY_A " or " KEY_B " to fly",
	"Sets a random animation every\nframe, if you get stuck press " KEY_L,
	"Hold SELECT and press UP to\nchange the inventory powerup",
	"Hold " KEY_X " or " KEY_Y " to fall slowly",
	"(WIP) Hold " KEY_SELECT " and press\nDOWN to toggle the starman\neffect",
	"Hold " KEY_SELECT " and press RIGHT to\ninstant kill the other player\n(Only Mario vs Luigi), hold\n" KEY_SELECT " and press LEFT to\ninstantkill yourself",
	"Disable the limit to throw\nfireballs",
	"Hold " KEY_R " to move really fast\n(you can clip into blocks if you have the Mini Mushroom)",
	"Disable lateral collision with\ntiles",
	"Hold " KEY_SELECT " and " KEY_R " to make the\nplayer bigger, hold " KEY_SELECT " and " KEY_L " to make the player smaller\n(it changes the model dimension,\nnot the hitbox)",
};

static int AVaddressCount = 0;
static int AVaddressValues[14];

static const char* switchGfx[2] = { SWITCHOFF, SWITCHON };

void RVReadBytes(u32 address, u8 out[4])
{
	for (int i = 0; i < 4; i++)
	{
		out[i] = *reinterpret_cast<u8*>(address + i);
	}
}

void RVReadShorts(u32 address, u8 out[4])
{
	u16 s[2];
	for (int i = 0; i < 2; i++)
	{
		s[i] = *reinterpret_cast<u16*>(address + i * 2);
	}
	out[0] = s[0];
	out[1] = s[0] >> 8;
	out[2] = s[1];
	out[3] = s[1] >> 8;
}

void RVReadWord(u32 address, u8 out[4])
{
	u32 w = *reinterpret_cast<u32*>(address);
	out[0] = w;
	out[1] = w >> 8;
	out[2] = w >> 16;
	out[3] = w >> 24;
}

static void updateTextMain()
{
	Console::clear();

	Console::setTextColor(Console::COLOR_WHITE);

	Console::printxy(0, 0, LONG_LINE);
	Console::printxy(4, 1, "NSMB Utils - Main Menu");
	Console::printxy(0, 2, LONG_LINE);

	Console::printxy(0, 21, LONG_LINE);
	Console::printxy(0, 22, KEY_L " & " KEY_R " to choose and " KEY_X " to select");
	Console::printxy(0, 23, LONG_LINE);

	Console::printxy(4, 4, "Cheats Menu");
	Console::printxy(4, 5, "Debugger Menu");
	Console::printxy(4, 6, "RamViewer Menu");

	u32 i = 4;
	if (selectedMenu[playerNumber] >= 0 && selectedMenu[playerNumber] < 3)
		i += selectedMenu[playerNumber];
	Console::printxy(2, i, CURSOR1);
}

static void updateTextCheats()
{
	Console::clear();

	Console::setTextColor(Console::COLOR_WHITE);

	Console::printxy(0, 0, LONG_LINE);
	Console::printxy(3, 1, "NSMB Utils - Cheats Menu");
	Console::printxy(0, 2, LONG_LINE);

	Console::printxy(0, 20, LONG_LINE);
	Console::printxy(0, 21, KEY_L " & " KEY_R " to choose and " KEY_X " to switch");
	Console::printxy(0, 22, KEY_Y " for info and " KEY_SELECT " to go back");
	Console::printxy(0, 23, LONG_LINE);

	for (int i = 0; i < CHEAT_COUNT; i++)
	{
		Console::printxy(4, 4 + i, switchGfx[cheatsEnabled[playerNumber][i]]);
		Console::printxy(7, 4 + i, cheatNames[i]);
	}

	u32 i = 4; // selected cheat arrow
	if (selectedCheat[playerNumber] >= 0 && selectedCheat[playerNumber] < CHEAT_COUNT)
		i += selectedCheat[playerNumber];
	Console::printxy(2, i, CURSOR1);
}

static void updateTextCheatsInfo()
{
	Console::clear();

	Console::setTextColor(Console::COLOR_WHITE);

	Console::printxy(0, 0, LONG_LINE);
	Console::printxy(3, 1, "NSMB Utils - Cheat Menu");
	Console::printxy(0, 2, LONG_LINE);

	const char* cheatName = cheatNames[selectedCheat[playerNumber]];
	const char* cheatDesc = cheatDescriptions[selectedCheat[playerNumber]];

	Console::printxy(0, 4, "Info for");
	Console::printxy(9, 4, cheatName);
	Console::printxy(9 + strlen(cheatName), 4, ":");

	Console::printxy(0, 6, cheatDesc);

	Console::printxy(0, 21, LONG_LINE);
	Console::printxy(10, 22, KEY_Y " to go back");
	Console::printxy(0, 23, LONG_LINE);
}

static void updateTextAddrViewer()
{
	Console::clear();

	Console::setTextColor(Console::COLOR_WHITE);

	Console::printxy(0, 0, LONG_LINE);
	Console::printxy(2, 1, "NSMB Utils - Address Viewer");
	Console::printxy(0, 2, LONG_LINE);

	Console::printxy(0, 20, LONG_LINE);
	Console::printxy(0, 23, LONG_LINE);

	if (disableMenu[playerNumber])
	{
		Console::printxy(1, 21, "All the controls are disabled");
		Console::printxy(4, 22, "because you are playing");
	}
	else
	{
		Console::printxy(0, 21, KEY_Y " & " KEY_X " to add & remove an address");
		Console::printxy(7, 22, KEY_SELECT " to go back");
	}

	if (AVaddressCount != 0)
	{
		for (int i = 0; i < AVaddressCount; i++)
		{
			int val = *reinterpret_cast<int*>(AVaddressValues[i]);
			Console::printxy(4, 4 + i, "0x%08x = %08x", AVaddressValues[i], val);
		}
	}
}

static void updateTextAddrViewerAdd()
{
	Console::clear();

	Console::setTextColor(Console::COLOR_WHITE);

	Console::printxy(0, 0, LONG_LINE);
	Console::printxy(2, 1, "NSMB Utils - Address Viewer");
	Console::printxy(0, 2, LONG_LINE);

	Console::printxy(0, 19, LONG_LINE);
	Console::printxy(1, 20, KEY_L " & " KEY_R " to scroll between digits");
	Console::printxy(6, 21, KEY_X " & " KEY_Y " to change value");
	Console::printxy(3, 22, KEY_SELECT " to save the address");
	Console::printxy(0, 23, LONG_LINE);

	Console::printxy(1, 4, "Add a new address:");

	Console::printxy(1, 6, "0x%08x", AVtempAddress);
	Console::printxy(1 + 9 - AVcurDigit, 7, CURSOR2);
}

static void updateTextRamViewer()
{
	Console::clear();

	Console::setTextColor(Console::COLOR_WHITE);

	Console::printxy(0, 0, LONG_LINE);
	Console::printxy(4, 1, "NSMB Utils - Ram Viewer");
	Console::printxy(0, 2, LONG_LINE);

	if (disableMenu[playerNumber])
	{
		Console::printxy(0, 20, LONG_LINE);
		Console::printxy(1, 21, "All the controls are disabled");
		Console::printxy(4, 22, "because you are playing");
		Console::printxy(0, 23, LONG_LINE);
	}
	else
	{
		Console::printxy(0, 19, LONG_LINE);
		Console::printxy(1, 20, KEY_L " & " KEY_R " to scroll between digits");
		Console::printxy(1, 21, KEY_X " & " KEY_Y " to change value & bitmode");
		Console::printxy(8, 22, KEY_SELECT " to go back");
		Console::printxy(0, 23, LONG_LINE);
	}

	Console::printxy(3, 4, "%2d-bit", 8 << RVreadMode);

	Console::printxy(17, 4, "0x%08x", RVaddress);
	if (!disableMenu[playerNumber])
		Console::printxy(17 + 9 - RVcurDigit, 5, CURSOR2);

	Console::align(Console::ALIGN_LEFT);
	for (int y = 0; y < 12; y++)
	{
		Console::setTextColor(Console::COLOR_WHITE);
		Console::printxy(2, 6 + y, "%08X ", RVaddress + y * 8);

		for (int x = 0; x < 8; x += 4)
		{
			u8 bytes[4];

			switch (RVreadMode)
			{
			case 0:
				RVReadBytes(RVaddress + y * 8 + x * 4, bytes);
				break;
			case 1:
				RVReadShorts(RVaddress + y * 8 + x * 4, bytes);
				break;
			case 2:
				RVReadWord(RVaddress + y * 8 + x * 4, bytes);
				break;
			}

			for (int i = 0; i < 4; i++)
			{
				if (RVColor == false)
					Console::setTextColor(Console::COLOR_WHITE);
				else
					Console::setTextColor(Console::COLOR_LIGHT_BLUE);

				RVColor = !RVColor;

				Console::printxy(2 + 12 + x * 8 + i * 2, 6 + y, "%02X", bytes[i]);
			}
		}
	}
}

TextUpdater updaterForMenu[6] = {
	updateTextMain,
	updateTextCheats,
	updateTextCheatsInfo,
	updateTextAddrViewer,
	updateTextAddrViewerAdd,
	updateTextRamViewer
};

static void controlsMain()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	RVcurDigit = 0;
	AVcurDigit = 6;

	if (buttonsPressed_ & PAD_BUTTON_R)
	{
		if (selectedMenu[playerNumber] != 2)
			selectedMenu[playerNumber]++;
	}

	else if (buttonsPressed_ & PAD_BUTTON_L)
	{
		if (selectedMenu[playerNumber] != 0)
			selectedMenu[playerNumber]--;
	}

	if (buttonsPressed_ & PAD_BUTTON_X)
		switch (selectedMenu[playerNumber])
		{
		case 0:
			menuType[playerNumber] = MenuType::Cheats;
			break;
		case 1:
			menuType[playerNumber] = MenuType::AddrViewer;
			break;
		case 2:
			menuType[playerNumber] = MenuType::RamViewer;
			break;
		}
}

static void controlsCheats()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	if (buttonsPressed_ & PAD_BUTTON_R)
	{
		if (selectedCheat[playerNumber] != CHEAT_COUNT - 1)
			selectedCheat[playerNumber]++;
	}

	else if (buttonsPressed_ & PAD_BUTTON_L)
	{
		if (selectedCheat[playerNumber] != 0)
			selectedCheat[playerNumber]--;
	}

	if (buttonsPressed_ & PAD_BUTTON_X)
		cheatsEnabled[playerNumber][selectedCheat[playerNumber]] ^= 1;

	else if (buttonsPressed_ & PAD_BUTTON_Y)
		menuType[playerNumber] = MenuType::CheatsInfo;

	else if (buttonsPressed_ & PAD_BUTTON_SELECT)
		menuType[playerNumber] = MenuType::Main;
}

static void controlsCheatsInfo()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	if (buttonsPressed_ & PAD_BUTTON_Y)
		menuType[playerNumber] = MenuType::Cheats;
}

static void controlsAddrViewer()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	if (buttonsPressed_ & PAD_BUTTON_SELECT)
		menuType[playerNumber] = MenuType::Main;

	if (buttonsPressed_ & PAD_BUTTON_Y)
		menuType[playerNumber] = MenuType::AddrViewerAdd;

	if (buttonsPressed_ & PAD_BUTTON_X)
		AVaddressCount--;
}

static void controlsAddrViewerAdd()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	AVdigitMask = 0xF << (AVcurDigit * 4);
	AVdigitOne = 1 << (AVcurDigit * 4);
	AVadrNoDgt = AVtempAddress & ~AVdigitMask;
	AVadrDgt = AVtempAddress & AVdigitMask;

	if (PAD_BUTTON_R & buttonsPressed_)
		AVcurDigit--;
	else if (PAD_BUTTON_L & buttonsPressed_)
		AVcurDigit++;

	if (AVcurDigit == 7)
		AVcurDigit = 6;

	if (AVcurDigit < 0)
		AVcurDigit = 0;

	if (PAD_BUTTON_X & buttonsPressed_)
	{
		AVadrDgt = (AVadrDgt + AVdigitOne) & AVdigitMask;
		AVtempAddress = AVadrNoDgt | AVadrDgt;
	}
	else if (PAD_BUTTON_Y & buttonsPressed_)
	{
		AVadrDgt = (AVadrDgt - AVdigitOne) & AVdigitMask;
		AVtempAddress = AVadrNoDgt | AVadrDgt;
	}

	if (PAD_BUTTON_SELECT & buttonsPressed_)
	{
		AVaddressValues[AVcurAddress] = AVtempAddress;
		if (++AVcurAddress > 15)
			AVcurAddress = 0;
		if (++AVaddressCount > 15)
			AVaddressCount = 14;

		AVtempAddress = 0x0000000;
		AVcurDigit = 6;

		menuType[playerNumber] = MenuType::AddrViewer;
	}
}

static void controlsRamViewer()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	u32 digitMask = 0xF << (RVcurDigit * 4);

	if (PAD_BUTTON_X & buttonsPressed_)
		RVaddress = (RVaddress & ~digitMask) | (((RVaddress & digitMask) + (1 << (RVcurDigit * 4))) & digitMask);

	if (PAD_BUTTON_Y & buttonsPressed_)
		RVreadMode++;

	if (buttonsPressed_ & PAD_BUTTON_L)
	{
		if (RVcurDigit != 6)
			RVcurDigit++;
	}

	if (buttonsPressed_ & PAD_BUTTON_R)
	{
		if (RVcurDigit != 0)
			RVcurDigit--;
	}

	if (buttonsPressed_ & PAD_BUTTON_SELECT)
		menuType[playerNumber] = MenuType::Main;

	if (RVreadMode >= 3)
		RVreadMode = 0;
}

GetControls controlsForMenu[6] = {
	controlsMain,
	controlsCheats,
	controlsCheatsInfo,
	controlsAddrViewer,
	controlsAddrViewerAdd,
	controlsRamViewer
};

static void UpdateCheatMenuForPlayer(int playerNo)
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	if (!menuOpened[playerNo] && !disableMenu[playerNo])
	{
		if (buttonsPressed_ & PAD_BUTTON_SELECT)
		{
			if (playerNo == playerNumber)
			{
				Console::backupVram();
				Console::init();
			}
			menuOpened[playerNo] = true;
		}
	}
	else if (playerNo == playerNumber)
	{
		if (!disableMenu[playerNo])
			controlsForMenu[(int)menuType[playerNo]]();
		updaterForMenu[(int)menuType[playerNo]]();
		Console::update();
	}
}

void hook_020a2c80_ov_00(void* stageScene)
{
	for (int i = 0; i < GetPlayerCount(); i++)
		UpdateCheatMenuForPlayer(i);
}

// PauseMenu::onOpen hook
int repl_020A2900_ov_00()
{
	disableMenu[playerNumber] = false;
	return 0x020CA850; //Keep replaced instruction
}

//Restores the VRAM backup and closes the console
static void restoreAndClose()
{
	if (menuOpened[playerNumber])
		Console::restoreVram();

	menuOpened[0] = false;
	menuOpened[1] = false;
}

void hook_020A2518_ov_00() //pause menu close
{
	if ((menuType[playerNumber] == MenuType::AddrViewer) || (menuType[playerNumber] == MenuType::RamViewer))
	{
		disableMenu[playerNumber] = false;
	}
	else
	{
		disableMenu[playerNumber] = true;
		restoreAndClose();
	}
}

void hook_020A189C_ov_00() //level exit
{
	disableMenu[playerNumber] = false;
	restoreAndClose();
}

void hook_020baab8_ov_00() //playing level
{
	disableMenu[playerNumber] = true;
}