
#include "BotDuelClassAI.h"
#include "BotBGAIMovement.h"

uint32 DuelDemonHunterAI::GetFuryPowerPer()
{
	float per = (float)me->GetPower(POWER_FURY) / (float)me->GetMaxPower(POWER_FURY);
	return (uint32)(per * 100);
}

void DuelDemonHunterAI::ResetBotAI()
{
	BotDuelAI::ResetBotAI();
	InitializeSpells(me);
}

void DuelDemonHunterAI::ProcessMeleeSpell(Unit* pTarget)
{
	if (!pTarget)
		return;

	bool inMeta = me->HasAura(DemonHunterHavoc_Metamorphosis);
	uint32 furyPer = GetFuryPowerPer();

	if (!me->HasAura(DemonHunterCommon_ImmolationAura) && TryCastSpell(DemonHunterCommon_ImmolationAura, me) == SpellCastResult::SPELL_CAST_OK)
		return;

	if (pTarget->HasUnitState(UNIT_STATE_CASTING) && BotUtility::SpellHasReady(me, DemonHunterCommon_Disrupt) && TryCastSpell(DemonHunterCommon_Disrupt, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	if (!inMeta && furyPer >= 90 && BotUtility::SpellHasReady(me, DemonHunterHavoc_Metamorphosis) && TryCastSpell(DemonHunterHavoc_Metamorphosis, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	uint32 spendSpell = inMeta ? DemonHunterHavoc_Annihilation : DemonHunterHavoc_ChaosStrike;
	if (furyPer >= 40 && TryCastSpell(spendSpell, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	TryCastSpell(DemonHunterHavoc_DemonsBite, pTarget);
}

void DuelDemonHunterAI::ProcessRangeSpell(Unit* pTarget)
{
	if (!pTarget)
		return;
	if (me->GetDistance(pTarget) > 8.0f && BotUtility::SpellHasReady(me, DemonHunterHavoc_FelRush))
	{
		if (TryCastSpell(DemonHunterHavoc_FelRush, pTarget) == SpellCastResult::SPELL_CAST_OK)
			return;
	}
	TryCastSpell(DemonHunterCommon_ThrowGlaive, pTarget);
}

void DuelDemonHunterAI::ProcessFlee()
{
	if (me->GetHealthPct() <= 30.0f && TryCastSpell(DemonHunterCommon_Netherwalk, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	if (TryCastSpell(DemonHunterCommon_VengefulRetreat, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	FleeMovement();
}
