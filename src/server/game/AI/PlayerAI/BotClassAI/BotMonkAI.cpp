
#include "BotMonkAI.h"

void BotMonkAI::InitializeSpells()
{
	MonkCommon_TigerPalm = FindMaxRankSpellByExist(100780);
	MonkCommon_BlackoutKick = FindMaxRankSpellByExist(100784);
	MonkCommon_RisingSunKick = FindMaxRankSpellByExist(107428);
	MonkCommon_SpinningCraneKick = FindMaxRankSpellByExist(101546);
	MonkCommon_FistsOfFury = FindMaxRankSpellByExist(113656);
	MonkCommon_TouchOfDeath = FindMaxRankSpellByExist(115080);
	MonkCommon_FlyingSerpentKick = FindMaxRankSpellByExist(101545);
	MonkCommon_Roll = FindMaxRankSpellByExist(109132);
	MonkCommon_FortifyingBrew = FindMaxRankSpellByExist(120954);
	MonkCommon_TouchOfKarma = FindMaxRankSpellByExist(122470);
	MonkCommon_SpearHandStrike = FindMaxRankSpellByExist(116705);
}

void BotMonkAI::ResetBotAI()
{
	BotBGAI::ResetBotAI();
	InitializeSpells();
}

bool BotMonkAI::IsHealerBotAI()
{
	// Brewmaster and Windwalker are not healers; only Mistweaver is.
	return me->FindTalentType() == 1;
}

bool BotMonkAI::ProcessNormalSpell()
{
	if (me->HasUnitState(UNIT_STATE_CASTING))
		return true;
	return TryUpMount();
}

void BotMonkAI::ProcessMeleeSpell(Unit* pTarget)
{
	if (!pTarget)
		return;

	uint32 chi = me->GetPower(POWER_CHI);
	uint32 targetMeCount = RangeEnemyListByTargetIsMe(NEEDFLEE_CHECKRANGE).size();

	if (pTarget->GetHealthPct() <= 10.0f && BotUtility::SpellHasReady(me, MonkCommon_TouchOfDeath) && TryCastSpell(MonkCommon_TouchOfDeath, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	if (targetMeCount >= 3)
	{
		if (chi >= 3 && TryCastSpell(MonkCommon_SpinningCraneKick, pTarget) == SpellCastResult::SPELL_CAST_OK)
			return;
		if (BotUtility::SpellHasReady(me, MonkCommon_FistsOfFury) && TryCastSpell(MonkCommon_FistsOfFury, pTarget) == SpellCastResult::SPELL_CAST_OK)
			return;
	}

	if (BotUtility::SpellHasReady(me, MonkCommon_RisingSunKick) && chi >= 2 && TryCastSpell(MonkCommon_RisingSunKick, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	if (chi >= 2 && TryCastSpell(MonkCommon_BlackoutKick, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	TryCastSpell(MonkCommon_TigerPalm, pTarget);
}

void BotMonkAI::ProcessRangeSpell(Unit* pTarget)
{
	if (!pTarget)
		return;
	if (me->GetDistance(pTarget) > 8.0f && BotUtility::SpellHasReady(me, MonkCommon_FlyingSerpentKick))
	{
		if (TryCastSpell(MonkCommon_FlyingSerpentKick, pTarget) == SpellCastResult::SPELL_CAST_OK)
			return;
	}
	if (BotUtility::SpellHasReady(me, MonkCommon_Roll))
		TryCastSpell(MonkCommon_Roll, me);
}

void BotMonkAI::ProcessFlee()
{
	if (me->GetHealthPct() <= 30.0f && TryCastSpell(MonkCommon_FortifyingBrew, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	if (TryCastSpell(MonkCommon_Roll, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	FleeMovement();
}
