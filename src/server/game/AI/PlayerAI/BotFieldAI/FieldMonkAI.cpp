
#include "BotFieldClassAI.h"
#include "BotBGAIMovement.h"

void FieldMonkAI::ResetBotAI()
{
	BotFieldAI::ResetBotAI();
	InitializeSpells(me);
}

void FieldMonkAI::OnLevelUp(uint32 talentType)
{
	InitializeSpells(me);
}

bool FieldMonkAI::ProcessNormalSpell()
{
	if (me->HasUnitState(UNIT_STATE_CASTING))
		return true;
	return TryUpMount();
}

void FieldMonkAI::ProcessMeleeSpell(Unit* pTarget)
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

void FieldMonkAI::ProcessRangeSpell(Unit* pTarget)
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

void FieldMonkAI::ProcessFlee()
{
	if (me->GetHealthPct() <= 30.0f && TryCastSpell(MonkCommon_FortifyingBrew, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	if (TryCastSpell(MonkCommon_Roll, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	FleeMovement();
}
