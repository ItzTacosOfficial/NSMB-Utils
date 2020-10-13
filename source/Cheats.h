#ifndef _CHEATS_H
#define _CHEATS_H

enum class CheatType
{
	Fly,
	AnimRand,
	PowerupCycle,
	SlowFall,
	StarmanSwitch,
	KillSwitch,
	FireballAbuse, //Overblade fucking god please finish fireflower
	FastMove,
	WallClip,
	SizeChanger,
};

constexpr int cheatsCount = 10;

extern bool cheatsEnabled[2][cheatsCount];

inline bool isCheatEnabled(CheatType type, int playerNo)
{
    return cheatsEnabled[playerNo][static_cast<u32>(type)];
}

#endif //_CHEATS_H
