#ifndef _CHEATS_H
#define _CHEATS_H

#define CHEAT_COUNT 10

enum CHEAT_TYPE
{
	FLY,
	ANIM_RAND,
	POWERUP_CYCLE,
	SLOW_FALL,
	STARMAN_SWITCH,
	KILL_SWITCH,
	FIREBALL_ABUSE,
	FAST_MOVE,
	WALL_CLIP,
	SIZE_CHANGER,
};

extern bool cheatsEnabled[2][CHEAT_COUNT];

#endif //_CHEATS_H
