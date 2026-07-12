
#include "BotDemonHunterAI.h"

uint32 BotDemonHunterAI::GetFuryPowerPer()
{
	float per = (float)me->GetPower(POWER_FURY) / (float)me->GetMaxPower(POWER_FURY);
	return (uint32)(per * 100);
}

void BotDemonHunterAI::InitializeSpells()
{
	DemonHunterCommon_ThrowGlaive = FindMaxRankSpellByExist(185123);
	DemonHunterCommon_ImmolationAura = FindMaxRankSpellByExist(178740);
	DemonHunterCommon_ChaosNova = FindMaxRankSpellByExist(179057);
	DemonHunterCommon_Netherwalk = FindMaxRankSpellByExist(196555);
	DemonHunterCommon_Disrupt = FindMaxRankSpellByExist(183752);
	DemonHunterCommon_VengefulRetreat = FindMaxRankSpellByExist(198793);

	DemonHunterHavoc_DemonsBite = FindMaxRankSpellByExist(162243);
	DemonHunterHavoc_ChaosStrike = FindMaxRankSpellByExist(162794);
	DemonHunterHavoc_Annihilation = FindMaxRankSpellByExist(201427);
	DemonHunterHavoc_BladeDance = FindMaxRankSpellByExist(188499);
	DemonHunterHavoc_DeathSweep = FindMaxRankSpellByExist(210152);
	DemonHunterHavoc_EyeBeam = FindMaxRankSpellByExist(198013);
	DemonHunterHavoc_FelRush = FindMaxRankSpellByExist(195072);
	DemonHunterHavoc_Metamorphosis = FindMaxRankSpellByExist(191427);
}

void BotDemonHunterAI::ResetBotAI()
{
	BotBGAI::ResetBotAI();
	InitializeSpells();
}

bool BotDemonHunterAI::ProcessNormalSpell()
{
	if (me->HasUnitState(UNIT_STATE_CASTING))
		return true;
	return TryUpMount();
}

void BotDemonHunterAI::ProcessMeleeSpell(Unit* pTarget)
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

void BotDemonHunterAI::ProcessRangeSpell(Unit* pTarget)
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

void BotDemonHunterAI::ProcessFlee()
{
	if (me->GetHealthPct() <= 30.0f && TryCastSpell(DemonHunterCommon_Netherwalk, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	if (TryCastSpell(DemonHunterCommon_VengefulRetreat, me) == SpellCastResult::SPELL_CAST_OK)
		return;
	FleeMovement();
}
