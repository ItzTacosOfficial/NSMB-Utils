#include "nsmb.h"
#include "Console.h"
#include "Cheats.h"

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

enum class MenuType
{
	Main,
	Cheats,
	CheatsInfo,
	AddrViewer,
	AddrViewerAdd,
	RamViewer
};

struct MenuProps
{
	inline MenuProps() : selectedMenu(0), selectedCheat(0), menuOpened(false), disableMenu(false),
		AVtempAddress(0x02000000), AVcurDigit(6), AVcurAddress(0), AVdigitMask(0),
		AVdigitOne(0), AVadrNoDgt(0), AVadrDgt(0), RVaddress(0x02000000),
		RVreadMode(0), RVcurDigit(0), RVcolor(false), RVinvalid(false), AVaddressCount(0), menuType(MenuType::Main)
	{
		for (int i = 0; i < 14; i++)
		{
			AVaddressValues[i] = 0;
		}
	}

	int selectedMenu;
	int selectedCheat; //selected cheat int for the menu
	bool menuOpened; //console activated bool
	bool disableMenu;
	
	u32 AVtempAddress;
	int AVcurDigit;
	int AVcurAddress;
	u32 AVdigitMask;
	u32 AVdigitOne;
	u32 AVadrNoDgt;
	u32 AVadrDgt;
	
	u32 RVaddress = 0x02000000;
	u32 RVreadMode = 0; // 0 = 8bit, 1 = 16bit, 2 = 32bit
	u32 RVcurDigit = 0;
	bool RVcolor = false;
	bool RVinvalid = false;
	
	int AVaddressCount = 0;
	int AVaddressValues[14];

	MenuType menuType = MenuType::Main;
};

static MenuProps menuProps[2];

typedef void(*TextUpdater)();
typedef void(*GetControls)();

static const char* cheatNames[cheatsCount] = {
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

static const char* cheatDescriptions[cheatsCount] = {
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

static const char* switchGfx[2] = { SWITCHOFF, SWITCHON };

static void updateTextMain();
static void updateTextCheats();
static void updateTextCheatsInfo();
static void updateTextAddrViewer();
static void updateTextAddrViewerAdd();
static void updateTextRamViewer();
static void controlsMain();
static void controlsCheats();
static void controlsCheatsInfo();
static void controlsAddrViewer();
static void controlsAddrViewerAdd();
static void controlsRamViewer();

static TextUpdater updaterForMenu[6] = {
	updateTextMain,
	updateTextCheats,
	updateTextCheatsInfo,
	updateTextAddrViewer,
	updateTextAddrViewerAdd,
	updateTextRamViewer
};

static GetControls controlsForMenu[6] = {
	controlsMain,
	controlsCheats,
	controlsCheatsInfo,
	controlsAddrViewer,
	controlsAddrViewerAdd,
	controlsRamViewer
};

namespace RV
{
	void ReadBytes(u32 address, u8 out[4])
	{
		for (int i = 0; i < 4; i++)
		{
			out[i] = *reinterpret_cast<u8*>(address + i);
		}
	}

	void ReadShorts(u32 address, u8 out[4])
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

	void ReadWord(u32 address, u8 out[4])
	{
		u32 w = *reinterpret_cast<u32*>(address);
		out[0] = w;
		out[1] = w >> 8;
		out[2] = w >> 16;
		out[3] = w >> 24;
	}
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
	if (menuProps[playerNumber].selectedMenu >= 0 && menuProps[playerNumber].selectedMenu < 3)
		i += menuProps[playerNumber].selectedMenu;
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

	for (int i = 0; i < cheatsCount; i++)
	{
		Console::printxy(4, 4 + i, switchGfx[cheatsEnabled[playerNumber][i]]);
		Console::printxy(7, 4 + i, cheatNames[i]);
	}

	u32 i = 4; // selected cheat arrow
	if (menuProps[playerNumber].selectedCheat >= 0 && menuProps[playerNumber].selectedCheat < cheatsCount)
		i += menuProps[playerNumber].selectedCheat;
	Console::printxy(2, i, CURSOR1);
}

static void updateTextCheatsInfo()
{
	Console::clear();

	Console::setTextColor(Console::COLOR_WHITE);

	Console::printxy(0, 0, LONG_LINE);
	Console::printxy(3, 1, "NSMB Utils - Cheat Menu");
	Console::printxy(0, 2, LONG_LINE);

	const char* cheatName = cheatNames[menuProps[playerNumber].selectedCheat];
	const char* cheatDesc = cheatDescriptions[menuProps[playerNumber].selectedCheat];

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

	if (menuProps[playerNumber].disableMenu)
	{
		Console::printxy(1, 21, "All the controls are disabled");
		Console::printxy(4, 22, "because you are playing");
	}
	else
	{
		Console::printxy(0, 21, KEY_Y " & " KEY_X " to add & remove an address");
		Console::printxy(7, 22, KEY_SELECT " to go back");
	}

	if (menuProps[playerNumber].AVaddressCount != 0)
	{
		for (int i = 0; i < menuProps[playerNumber].AVaddressCount; i++)
		{
			int val = *reinterpret_cast<int*>(menuProps[playerNumber].AVaddressValues[i]);
			Console::printxy(4, 4 + i, "0x%08x = %08x", menuProps[playerNumber].AVaddressValues[i], val);
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

	Console::printxy(1, 6, "0x%08x", menuProps[playerNumber].AVtempAddress);
	Console::printxy(1 + 9 - menuProps[playerNumber].AVcurDigit, 7, CURSOR2);

	Console::printxy(1, 9, "Range: 0x03810000 - 0x01ff8000");
}

static void updateTextRamViewer()
{
	Console::clear();

	Console::setTextColor(Console::COLOR_WHITE);

	Console::printxy(0, 0, LONG_LINE);
	Console::printxy(4, 1, "NSMB Utils - Ram Viewer");
	Console::printxy(0, 2, LONG_LINE);

	if (menuProps[playerNumber].disableMenu)
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

	Console::printxy(3, 4, "%2d-bit", 8 << menuProps[playerNumber].RVreadMode);

	Console::printxy(17, 4, "0x%08x", menuProps[playerNumber].RVaddress);
	if (!menuProps[playerNumber].disableMenu)
		Console::printxy(17 + 9 - menuProps[playerNumber].RVcurDigit, 5, CURSOR2);

	Console::align(Console::ALIGN_LEFT);

	if ((0x03810000 < menuProps[playerNumber].RVaddress) || (menuProps[playerNumber].RVaddress < 0x01ff8000))
	{
		if (menuProps[playerNumber].RVinvalid == false)
		{
			PlaySNDEffect(238, nullptr);
		}

		menuProps[playerNumber].RVinvalid = true;

		Console::printxy(7, 6, "Invalid Address");
	}
	else
	{
		for (int y = 0; y < 12; y++)
		{
			Console::setTextColor(Console::COLOR_WHITE);
			Console::printxy(2, 6 + y, "%08X ", menuProps[playerNumber].RVaddress + y * 8);

			for (int x = 0; x < 8; x += 4)
			{
				u8 bytes[4];

				switch (menuProps[playerNumber].RVreadMode)
				{
				case 0:
					RV::ReadBytes(menuProps[playerNumber].RVaddress + y * 8 + x * 4, bytes);
					break;
				case 1:
					RV::ReadShorts(menuProps[playerNumber].RVaddress + y * 8 + x * 4, bytes);
					break;
				case 2:
					RV::ReadWord(menuProps[playerNumber].RVaddress + y * 8 + x * 4, bytes);
					break;
				default:
					//kekw
					return;
				}

				for (int i = 0; i < 4; i++)
				{
					if (menuProps[playerNumber].RVcolor == false)
						Console::setTextColor(Console::COLOR_WHITE);
					else
						Console::setTextColor(Console::COLOR_LIGHT_BLUE);

					menuProps[playerNumber].RVcolor = !menuProps[playerNumber].RVcolor;

					Console::printxy(2 + 12 + x * 8 + i * 2, 6 + y, "%02X", bytes[i]);
				}
			}
		}
	}
}

static void controlsMain()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	menuProps[playerNumber].RVcurDigit = 0;
	menuProps[playerNumber].AVcurDigit = 6;

	if (buttonsPressed_ & PAD_BUTTON_R)
	{
		if (menuProps[playerNumber].selectedMenu != 2)
			menuProps[playerNumber].selectedMenu++;
	}

	else if (buttonsPressed_ & PAD_BUTTON_L)
	{
		if (menuProps[playerNumber].selectedMenu != 0)
			menuProps[playerNumber].selectedMenu--;
	}

	if (buttonsPressed_ & PAD_BUTTON_X)
		switch (menuProps[playerNumber].selectedMenu)
		{
		case 0:
			menuProps[playerNumber].menuType = MenuType::Cheats;
			break;
		case 1:
			menuProps[playerNumber].menuType = MenuType::AddrViewer;
			break;
		case 2:
			menuProps[playerNumber].menuType = MenuType::RamViewer;
			break;
		}
}

static void controlsCheats()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	if (buttonsPressed_ & PAD_BUTTON_R)
	{
		if (menuProps[playerNumber].selectedCheat != cheatsCount - 1)
			menuProps[playerNumber].selectedCheat++;
	}

	else if (buttonsPressed_ & PAD_BUTTON_L)
	{
		if (menuProps[playerNumber].selectedCheat != 0)
			menuProps[playerNumber].selectedCheat--;
	}

	if (buttonsPressed_ & PAD_BUTTON_X)
		cheatsEnabled[playerNumber][menuProps[playerNumber].selectedCheat] ^= 1;

	else if (buttonsPressed_ & PAD_BUTTON_Y)
		menuProps[playerNumber].menuType = MenuType::CheatsInfo;

	else if (buttonsPressed_ & PAD_BUTTON_SELECT)
		menuProps[playerNumber].menuType = MenuType::Main;
}

static void controlsCheatsInfo()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	if (buttonsPressed_ & PAD_BUTTON_Y)
		menuProps[playerNumber].menuType = MenuType::Cheats;
}

static void controlsAddrViewer()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	if (buttonsPressed_ & PAD_BUTTON_SELECT)
		menuProps[playerNumber].menuType = MenuType::Main;

	if (buttonsPressed_ & PAD_BUTTON_Y)
		menuProps[playerNumber].menuType = MenuType::AddrViewerAdd;

	if (buttonsPressed_ & PAD_BUTTON_X)
	{
		menuProps[playerNumber].AVaddressCount--;
		menuProps[playerNumber].AVcurAddress--;
	}
}

static void controlsAddrViewerAdd()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	menuProps[playerNumber].AVdigitMask = 0xF << (menuProps[playerNumber].AVcurDigit * 4);
	menuProps[playerNumber].AVdigitOne = 1 << (menuProps[playerNumber].AVcurDigit * 4);
	menuProps[playerNumber].AVadrNoDgt = menuProps[playerNumber].AVtempAddress & ~menuProps[playerNumber].AVdigitMask;
	menuProps[playerNumber].AVadrDgt = menuProps[playerNumber].AVtempAddress & menuProps[playerNumber].AVdigitMask;

	if (PAD_BUTTON_R & buttonsPressed_)
		menuProps[playerNumber].AVcurDigit--;
	else if (PAD_BUTTON_L & buttonsPressed_)
		menuProps[playerNumber].AVcurDigit++;

	if (menuProps[playerNumber].AVcurDigit == 7)
		menuProps[playerNumber].AVcurDigit = 6;

	if (menuProps[playerNumber].AVcurDigit < 0)
		menuProps[playerNumber].AVcurDigit = 0;

	if (PAD_BUTTON_X & buttonsPressed_)
	{
		menuProps[playerNumber].AVadrDgt = (menuProps[playerNumber].AVadrDgt + menuProps[playerNumber].AVdigitOne) & menuProps[playerNumber].AVdigitMask;
		menuProps[playerNumber].AVtempAddress = menuProps[playerNumber].AVadrNoDgt | menuProps[playerNumber].AVadrDgt;
	}
	else if (PAD_BUTTON_Y & buttonsPressed_)
	{
		menuProps[playerNumber].AVadrDgt = (menuProps[playerNumber].AVadrDgt - menuProps[playerNumber].AVdigitOne) & menuProps[playerNumber].AVdigitMask;
		menuProps[playerNumber].AVtempAddress = menuProps[playerNumber].AVadrNoDgt | menuProps[playerNumber].AVadrDgt;
	}

	if (PAD_BUTTON_SELECT & buttonsPressed_)
	{
		if ((0x03810000 < menuProps[playerNumber].AVtempAddress) || (menuProps[playerNumber].AVtempAddress < 0x01ff8000))
			PlaySNDEffect(238, nullptr);
		else
		{
			menuProps[playerNumber].AVaddressValues[menuProps[playerNumber].AVcurAddress] = menuProps[playerNumber].AVtempAddress;
			if (++menuProps[playerNumber].AVcurAddress > 15)
				menuProps[playerNumber].AVcurAddress = 0;
			if (++menuProps[playerNumber].AVaddressCount > 15)
				menuProps[playerNumber].AVaddressCount = 14;

			menuProps[playerNumber].AVtempAddress = 0x02000000;
			menuProps[playerNumber].AVcurDigit = 6;

			menuProps[playerNumber].menuType = MenuType::AddrViewer;
		}
	}

	//238
}

static void controlsRamViewer()
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	u32 digitMask = 0xF << (menuProps[playerNumber].RVcurDigit * 4);

	if (PAD_BUTTON_X & buttonsPressed_)
		menuProps[playerNumber].RVaddress = (menuProps[playerNumber].RVaddress & ~digitMask) | (((menuProps[playerNumber].RVaddress & digitMask) + (1 << (menuProps[playerNumber].RVcurDigit * 4))) & digitMask);

	if (PAD_BUTTON_Y & buttonsPressed_)
		menuProps[playerNumber].RVreadMode++;

	if (buttonsPressed_ & PAD_BUTTON_L)
	{
		if (menuProps[playerNumber].RVcurDigit != 6)
			menuProps[playerNumber].RVcurDigit++;
	}

	if (buttonsPressed_ & PAD_BUTTON_R)
	{
		if (menuProps[playerNumber].RVcurDigit != 0)
			menuProps[playerNumber].RVcurDigit--;
	}

	if (buttonsPressed_ & PAD_BUTTON_SELECT)
		menuProps[playerNumber].menuType = MenuType::Main;

	if (menuProps[playerNumber].RVreadMode >= 3)
		menuProps[playerNumber].RVreadMode = 0;
}

static void UpdateCheatMenuForPlayer(int playerNo)
{
	int buttonsPressed_ = buttonsPressedAddr[playerNumber * 2];

	if (!menuProps[playerNo].menuOpened && !menuProps[playerNo].disableMenu)
	{
		if (buttonsPressed_ & PAD_BUTTON_SELECT)
		{
			if (playerNo == playerNumber)
			{
				Console::backupVram();
				Console::init();
			}
			menuProps[playerNo].menuOpened = true;
		}
	}
	else if (playerNo == playerNumber)
	{
		if (!menuProps[playerNo].disableMenu)
			controlsForMenu[(int)menuProps[playerNo].menuType]();
		updaterForMenu[(int)menuProps[playerNo].menuType]();
		Console::update();
	}
}

void hook_020a2c80_ov_00(void* stageScene)
{
	for (int i = 0; i < GetPlayerCount(); i++)
		UpdateCheatMenuForPlayer(i);
}

int repl_020A2900_ov_00() // PauseMenu::onOpen hook
{
	menuProps[playerNumber].disableMenu = false;
	return 0x020CA850; //Keep replaced instruction
}

static void restoreAndClose() //Restores the VRAM backup and closes the console
{
	if (menuProps[playerNumber].menuOpened)
		Console::restoreVram();

	menuProps[0].menuOpened = false;
	menuProps[1].menuOpened = false;
}

void hook_020A2518_ov_00() //pause menu close
{
	if ((menuProps[playerNumber].menuType == MenuType::AddrViewer) || (menuProps[playerNumber].menuType == MenuType::RamViewer))
	{
		menuProps[playerNumber].disableMenu = false;
	}
	else
	{
		menuProps[playerNumber].disableMenu = true;
		restoreAndClose();
	}
}

void hook_020A189C_ov_00() //level exit
{
	menuProps[playerNumber].disableMenu = false;
	restoreAndClose();
}

void hook_020baab8_ov_00() //playing level
{
	menuProps[playerNumber].disableMenu = true;
}