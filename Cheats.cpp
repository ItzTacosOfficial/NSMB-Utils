#include "nsmb.h"
#include "Console.h"
#include "Cheats.h"

#ifdef __INTELLISENSE__
#define __attribute__(x)
#endif

bool cheatsEnabled[2][CHEAT_COUNT] = { { false } };

static bool flagpoleScene = false;
static int powerupCycleIdx[2] = { 0 };
static bool isStarman[2] = { false };

void hook_020a1a1c() { flagpoleScene = true; }
void hook_020bb6f0() { flagpoleScene = false; }

void hook_020FD1D4_ov_0A(PlayerActor* player)
{
	int playerNo = player->actor.playerNumber;
	bool* cheatsEnabled_ = cheatsEnabled[playerNo];

	if ((cheatsEnabled_[CHEAT_TYPE::FLY]) && (flagpoleScene == false))
	{
		if (player->P.ButtonsHeld & PAD_BUTTON_B || player->P.ButtonsHeld & PAD_BUTTON_A)
		{
			player->actor.velocity.y = 0x3000;
		}
	}

	if ((cheatsEnabled_[CHEAT_TYPE::ANIM_RAND]) && (flagpoleScene == false))
	{
		if (player->P.ButtonsPressed & PAD_BUTTON_L)
		{
			int args = 0;
			PlayerActor_setMovementState(player, 0x02115AAC, 0, &args);
		}
	}

	if ((cheatsEnabled_[CHEAT_TYPE::POWERUP_CYCLE]) && (flagpoleScene == false))
	{
		if (player->P.ButtonsHeld & PAD_BUTTON_SELECT && player->P.ButtonsPressed & PAD_KEY_UP)
		{
			int& powerupCycleIdx_ = powerupCycleIdx[playerNo];

			powerupCycleIdx_++;
			if (powerupCycleIdx_ > 5)
				powerupCycleIdx_ = 0;

			SetStoredPowerupForPlayer(player->actor.playerNumber, powerupCycleIdx_);
		}
	}

	if ((cheatsEnabled_[CHEAT_TYPE::SLOW_FALL]) && (flagpoleScene == false))
	{
		if ((player->P.ButtonsHeld & PAD_BUTTON_X || player->P.ButtonsHeld & PAD_BUTTON_Y) && (player->actor.velocity.y < 0))
			player->actor.velocity.y = 0;
	}

	if ((cheatsEnabled_[CHEAT_TYPE::STARMAN_SWITCH]) && (flagpoleScene == false))
	{
		bool& isStarman_ = isStarman[playerNo];
		if (player->P.ButtonsHeld & PAD_BUTTON_SELECT && player->P.ButtonsPressed & PAD_KEY_DOWN)
		{
			isStarman_ = !isStarman_;
			if (isStarman_)
			{
				SetStarmanTimeForPlayer(playerNo, 601);
			}
		}
		else if (isStarman_)
		{
			SetStarmanTimeForPlayer(playerNo, 62);
		}
	}

	if ((cheatsEnabled_[CHEAT_TYPE::KILL_SWITCH]) && (flagpoleScene == false))
	{
		if (player->P.ButtonsHeld & PAD_BUTTON_SELECT)
		{
			if (player->P.ButtonsPressed & PAD_KEY_LEFT)
			{
				PlayerActor_setEntranceState(player, 0x02119B24, 0, 1);
			}
			else if ((player->P.ButtonsPressed & PAD_KEY_RIGHT) && GetPlayerCount() == 2)
			{
				PlayerActor* otherPlayer = GetPtrToPlayerActorByID(!player->actor.playerNumber);
				PlayerActor_setEntranceState(otherPlayer, 0x02119B24, 0, 1);
			}
		}
	}

	if ((cheatsEnabled_[CHEAT_TYPE::FIREBALL_ABUSE]) && (flagpoleScene == false))
	{
		u8* fireballsForPlayer = (u8*)0x02129480;
		fireballsForPlayer[playerNo] = 0;
	}

	if ((cheatsEnabled_[CHEAT_TYPE::FAST_MOVE]) && (flagpoleScene == false))
	{
		if (player->P.ButtonsHeld & PAD_BUTTON_R)
			player->actor.position.x += player->info.direction ? -FX32(8) : FX32(8);
	}

	if ((cheatsEnabled_[CHEAT_TYPE::WALL_CLIP]) && (flagpoleScene == false))
	{
		int btnsPressed = player->P.ButtonsPressed;
		if ((btnsPressed & PAD_KEY_LEFT) || (btnsPressed & PAD_KEY_RIGHT))
		{
			int x = FX_Whole(player->actor.position.x);
			int y = FX_Whole(-player->actor.position.y) - 16;

			int x_off = player->info.direction ? -16 : 16;
			x += x_off;

			if ((GetTileBehaviorAtPos(x, y) & 0x790000) != 0)
			{
				player->actor.position.x += FX32(x_off);
			}
		}
	}

	if ((cheatsEnabled_[CHEAT_TYPE::SIZE_CHANGER]) && (flagpoleScene == false))
	{
		if (player->P.ButtonsHeld & PAD_BUTTON_L && player->P.ButtonsHeld & PAD_BUTTON_SELECT)
			player->actor.scale = player->actor.scale - 100;

		if (player->P.ButtonsHeld & PAD_BUTTON_R && player->P.ButtonsHeld & PAD_BUTTON_SELECT)
			player->actor.scale = player->actor.scale + 100;
	}
}

__attribute__((naked)) static void PlayerActor_setAnimationBAK(PlayerActor* player, int animationNo, int startFrame, int unk1, int updateSpeed, int unk2) { asm("STMFD SP!, {R4-R8,LR}\nB 0x02120BBC"); }
void nsub_02120BB8_ov_0A(PlayerActor* player, int animationNo, int startFrame, int unk1, int updateSpeed, int unk2)
{
	int playerNo = player->actor.playerNumber;

	if ((cheatsEnabled[playerNo][CHEAT_TYPE::ANIM_RAND]) && (flagpoleScene == false))
	{
		animationNo = (u32)RNG() % 148;
	}

	PlayerActor_setAnimationBAK(player, animationNo, startFrame, unk1, updateSpeed, unk2);
}

//Re-sync cheats
void hook_02157A20_ov_34()
{
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < CHEAT_COUNT; j++)
		{
			cheatsEnabled[i][j] = false;
			powerupCycleIdx[i] = 0;
			isStarman[i] = false;
		}
}
