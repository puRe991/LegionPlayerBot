
#ifndef _BOT_MONK_AI_H_
#define _BOT_MONK_AI_H_

#include "ScriptSystem.h"
#include "BotAI.h"
#include "AIWaypointsMgr.h"

class BotMonkAI : public BotBGAI
{
public:
	BotMonkAI(Player* player) :
		BotBGAI(player)
	{}
	~BotMonkAI() {}

	void ResetBotAI() override;

protected:
	void InitializeSpells();
	bool ProcessNormalSpell() override;
	void ProcessMeleeSpell(Unit* pTarget) override;
	void ProcessRangeSpell(Unit* pTarget) override;
	void ProcessFlee() override;

private:
	uint32 MonkCommon_TigerPalm;
	uint32 MonkCommon_BlackoutKick;
	uint32 MonkCommon_RisingSunKick;
	uint32 MonkCommon_SpinningCraneKick;
	uint32 MonkCommon_FistsOfFury;
	uint32 MonkCommon_TouchOfDeath;
	uint32 MonkCommon_FlyingSerpentKick;
	uint32 MonkCommon_Roll;
	uint32 MonkCommon_FortifyingBrew;
	uint32 MonkCommon_TouchOfKarma;
	uint32 MonkCommon_SpearHandStrike;
};

#endif // !_BOT_MONK_AI_H_
