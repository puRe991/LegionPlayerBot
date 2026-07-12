
#ifndef _BOT_DEMONHUNTER_AI_H_
#define _BOT_DEMONHUNTER_AI_H_

#include "ScriptSystem.h"
#include "BotAI.h"
#include "AIWaypointsMgr.h"

class BotDemonHunterAI : public BotBGAI
{
public:
	BotDemonHunterAI(Player* player) :
		BotBGAI(player)
	{}
	~BotDemonHunterAI() {}

	void ResetBotAI() override;

protected:
	uint32 GetFuryPowerPer();
	void InitializeSpells();
	bool ProcessNormalSpell() override;
	void ProcessMeleeSpell(Unit* pTarget) override;
	void ProcessRangeSpell(Unit* pTarget) override;
	void ProcessFlee() override;

private:
	uint32 DemonHunterCommon_ThrowGlaive;
	uint32 DemonHunterCommon_ImmolationAura;
	uint32 DemonHunterCommon_ChaosNova;
	uint32 DemonHunterCommon_Netherwalk;
	uint32 DemonHunterCommon_Disrupt;
	uint32 DemonHunterCommon_VengefulRetreat;

	uint32 DemonHunterHavoc_DemonsBite;
	uint32 DemonHunterHavoc_ChaosStrike;
	uint32 DemonHunterHavoc_Annihilation;
	uint32 DemonHunterHavoc_BladeDance;
	uint32 DemonHunterHavoc_DeathSweep;
	uint32 DemonHunterHavoc_EyeBeam;
	uint32 DemonHunterHavoc_FelRush;
	uint32 DemonHunterHavoc_Metamorphosis;
};

#endif // !_BOT_DEMONHUNTER_AI_H_
