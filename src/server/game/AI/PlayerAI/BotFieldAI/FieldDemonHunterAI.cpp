
#include "BotFieldClassAI.h"
#include "BotBGAIMovement.h"

uint32 FieldDemonHunterAI::GetFuryPowerPer()
{
	float per = (float)me->GetPower(POWER_FURY) / (float)me->GetMaxPower(POWER_FURY);
	return (uint32)(per * 100);
}

void FieldDemonHunterAI::ResetBotAI()
{
	BotFieldAI::ResetBotAI();
	InitializeSpells(me);
}

void FieldDemonHunterAI::OnLevelUp(uint32 talentType)
{
	InitializeSpells(me);
}

bool FieldDemonHunterAI::ProcessNormalSpell()
{
	if (me->HasUnitState(UNIT_STATE_CASTING))
		return true;
	return TryUpMount();
}

void FieldDemonHunterAI::ProcessMeleeSpell(Unit* pTarget)
{
	if (!pTarget)
		return;

	bool inMeta = me->HasAura(DemonHunterHavoc_Metamorphosis);
	uint32 furyPer = GetFuryPowerPer();
	uint32 targetMeCount = RangeEnemyListByTargetIsMe(NEEDFLEE_CHECKRANGE).size();

	if (!me->HasAura(DemonHunterCommon_ImmolationAura) && TryCastSpell(DemonHunterCommon_ImmolationAura, me) == SpellCastResult::SPELL_CAST_OK)
		return;

	if (targetMeCount >= 3 && BotUtility::SpellHasReady(me, DemonHunterHavoc_EyeBeam) && TryCastSpell(DemonHunterHavoc_EyeBeam, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	if (targetMeCount >= 2)
	{
		uint32 danceSpell = inMeta ? DemonHunterHavoc_DeathSweep : DemonHunterHavoc_BladeDance;
		if (BotUtility::SpellHasReady(me, danceSpell) && TryCastSpell(danceSpell, pTarget) == SpellCastResult::SPELL_CAST_OK)
			return;
		if (TryCastSpell(DemonHunterCommon_ChaosNova, me) == SpellCastResult::SPELL_CAST_OK)
			return;
	}

	if (!inMeta && furyPer >= 90 && BotUtility::SpellHasReady(me, DemonHunterHavoc_Metamorphosis) && TryCastSpell(DemonHunterHavoc_Metamorphosis, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	uint32 spendSpell = inMeta ? DemonHunterHavoc_Annihilation : DemonHunterHavoc_ChaosStrike;
	if (furyPer >= 40 && TryCastSpell(spendSpell, pTarget) == SpellCastResult::SPELL_CAST_OK)
		return;

	TryCastSpell(DemonHunterHavoc_DemonsBite, pTarget);
}

void FieldDemonHunterAI::ProcessRangeSpell(Unit* pTarget)
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

void FieldDemonHunterAI::ProcessFlee()
{
	if (me->GetHealthPct() <= 30.0f && TryCastSpell(DemonHunterCommon_Netherwalk, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	if (TryCastSpell(DemonHunterCommon_VengefulRetreat, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	FleeMovement();
}
