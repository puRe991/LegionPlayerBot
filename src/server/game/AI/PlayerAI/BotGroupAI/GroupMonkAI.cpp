
#include "BotGroupClassAI.h"
#include "BotBGAIMovement.h"

void GroupMonkAI::ResetBotAI()
{
	BotGroupAI::ResetBotAI();
	InitializeSpells(me);
}

uint32 GroupMonkAI::GetSeducePriority()
{
	if (!me->IsAlive())
		return 0;
	return 4;
}

void GroupMonkAI::OnLevelUp(uint32 talentType)
{
	BotGroupAI::OnLevelUp(talentType);
	InitializeSpells(me);
}

void GroupMonkAI::ProcessSeduceSpell(Unit* pTarget)
{
	if (!pTarget)
		return;
	ProcessMeleeSpell(pTarget);
}

bool GroupMonkAI::ProcessNormalSpell()
{
	if (me->HasUnitState(UNIT_STATE_CASTING))
		return true;
	return TryUpMount();
}

void GroupMonkAI::ProcessMeleeSpell(Unit* pTarget)
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

void GroupMonkAI::ProcessRangeSpell(Unit* pTarget)
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

void GroupMonkAI::ProcessFlee()
{
	if (me->GetHealthPct() <= 30.0f && TryCastSpell(MonkCommon_FortifyingBrew, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	if (TryCastSpell(MonkCommon_Roll, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	FleeMovement();
}

bool GroupMonkAI::TryBlockCastingByTarget(Unit* pTarget)
{
	if (!pTarget)
		return false;
	if (TryCastSpell(MonkCommon_SpearHandStrike, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return true;
	return false;
}
