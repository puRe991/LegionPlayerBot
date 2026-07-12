
#include "BotDuelClassAI.h"
#include "BotBGAIMovement.h"

void DuelMonkAI::ResetBotAI()
{
	BotDuelAI::ResetBotAI();
	InitializeSpells(me);
}

void DuelMonkAI::ProcessMeleeSpell(Unit* pTarget)
{
	if (!pTarget)
		return;

	uint32 chi = me->GetPower(POWER_CHI);

	if (pTarget->GetHealthPct() <= 10.0f && BotUtility::SpellHasReady(me, MonkCommon_TouchOfDeath) && TryCastSpell(MonkCommon_TouchOfDeath, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	if (pTarget->HasUnitState(UNIT_STATE_CASTING) && BotUtility::SpellHasReady(me, MonkCommon_SpearHandStrike) && TryCastSpell(MonkCommon_SpearHandStrike, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	if (BotUtility::SpellHasReady(me, MonkCommon_RisingSunKick) && chi >= 2 && TryCastSpell(MonkCommon_RisingSunKick, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	if (chi >= 2 && TryCastSpell(MonkCommon_BlackoutKick, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	TryCastSpell(MonkCommon_TigerPalm, pTarget);
}

void DuelMonkAI::ProcessRangeSpell(Unit* pTarget)
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

void DuelMonkAI::ProcessFlee()
{
	if (me->GetHealthPct() <= 30.0f && TryCastSpell(MonkCommon_FortifyingBrew, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	if (TryCastSpell(MonkCommon_Roll, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	FleeMovement();
}
