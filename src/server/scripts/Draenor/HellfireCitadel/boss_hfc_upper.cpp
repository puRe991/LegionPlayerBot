/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
SDName: Hellfire_Citadel_Upper
SD%Complete: 65
SDComment: Iskar, Socrethar, Velhari, Zakuun, Xhul'horac, Mannoroth and
           Archimonde. Spell ids come from the Warlords of Draenor encounter
           journal on Wowhead.

           Iskar and Socrethar were previously frame-only because the journal
           query came back empty; both now carry their verified rotations.
SDCategory: Hellfire Citadel
EndScriptData */

#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "hellfire_citadel.h"

enum HFCUpperTexts
{
    SAY_AGGRO       = 0,
    SAY_SLAY        = 1,
    SAY_DEATH       = 2,
    SAY_SPECIAL     = 3
};

/*######
## Shadow-Lord Iskar
##
## Spell ids from the Warlords of Draenor encounter journal. The Eye of Anzu
## (179202) is a raid-carried item and lives in the world database, so the
## script casts what Iskar himself casts and leaves the Eye to the players.
######*/

enum IskarSpells
{
    SPELL_ISKAR_FOCUSED_BLAST       = 181912,
    SPELL_ISKAR_PHANTASMAL_WINDS    = 181956,
    SPELL_ISKAR_FEL_CHAKRAM         = 182173,
    SPELL_ISKAR_PHANTASMAL_WOUNDS   = 182323,
    SPELL_ISKAR_FEL_INCINERATION    = 182582,
    SPELL_ISKAR_SHADOW_RIPOSTE      = 185343  // mythisch
};

enum IskarEvents
{
    EVENT_ISKAR_FOCUSED_BLAST = 1,
    EVENT_ISKAR_PHANTASMAL_WINDS,
    EVENT_ISKAR_FEL_CHAKRAM,
    EVENT_ISKAR_PHANTASMAL_WOUNDS,
    EVENT_ISKAR_FEL_INCINERATION,
    EVENT_ISKAR_SHADOW_RIPOSTE
};

class boss_shadow_lord_iskar : public CreatureScript
{
    public:
        boss_shadow_lord_iskar() : CreatureScript("boss_shadow_lord_iskar") { }

        struct boss_shadow_lord_iskarAI : public BossAI
        {
            boss_shadow_lord_iskarAI(Creature* creature) : BossAI(creature, DATA_ISKAR) { }

            void Reset()
            {
                _Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_ISKAR_PHANTASMAL_WOUNDS, 8000);
                events.ScheduleEvent(EVENT_ISKAR_FEL_CHAKRAM, 16000);
                events.ScheduleEvent(EVENT_ISKAR_FOCUSED_BLAST, 24000);
                events.ScheduleEvent(EVENT_ISKAR_FEL_INCINERATION, 32000);
                events.ScheduleEvent(EVENT_ISKAR_PHANTASMAL_WINDS, 45000);

                if (IsMythicRaid())
                    events.ScheduleEvent(EVENT_ISKAR_SHADOW_RIPOSTE, 30000);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _JustDied();
                summons.DespawnAll();
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ISKAR_PHANTASMAL_WOUNDS:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 60.0f, true))
                                DoCast(target, SPELL_ISKAR_PHANTASMAL_WOUNDS);
                            events.ScheduleEvent(EVENT_ISKAR_PHANTASMAL_WOUNDS, 25000);
                            break;
                        case EVENT_ISKAR_FEL_CHAKRAM:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 60.0f, true))
                                DoCast(target, SPELL_ISKAR_FEL_CHAKRAM);
                            events.ScheduleEvent(EVENT_ISKAR_FEL_CHAKRAM, 30000);
                            break;
                        case EVENT_ISKAR_FOCUSED_BLAST:
                            // Der Schaden wird laut Journal auf die getroffenen Ziele
                            // aufgeteilt, deshalb bewusst auf das Kampfziel.
                            DoCastVictim(SPELL_ISKAR_FOCUSED_BLAST);
                            events.ScheduleEvent(EVENT_ISKAR_FOCUSED_BLAST, 35000);
                            break;
                        case EVENT_ISKAR_FEL_INCINERATION:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 60.0f, true))
                                DoCast(target, SPELL_ISKAR_FEL_INCINERATION);
                            events.ScheduleEvent(EVENT_ISKAR_FEL_INCINERATION, 40000);
                            break;
                        case EVENT_ISKAR_PHANTASMAL_WINDS:
                            DoCastAOE(SPELL_ISKAR_PHANTASMAL_WINDS);
                            events.ScheduleEvent(EVENT_ISKAR_PHANTASMAL_WINDS, 60000);
                            break;
                        case EVENT_ISKAR_SHADOW_RIPOSTE:
                            DoCastAOE(SPELL_ISKAR_SHADOW_RIPOSTE);
                            events.ScheduleEvent(EVENT_ISKAR_SHADOW_RIPOSTE, 45000);
                            break;
                        default:
                            break;
                    }

                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        return;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_shadow_lord_iskarAI(creature);
        }
};

/*######
## Socrethar the Eternal
##
## Die Begegnung laeuft ueber den Seelengebundenen Konstrukt (90296); Socrethar
## selbst (bzw. seine Seele, 92330) steuert von der Seite bei. Der Konstrukt
## wechselt zwischen Socrethars und Spielerkontrolle -- dieser Wechsel haengt an
## einem Fahrzeug-/Kontrollzauber aus der Weltdatenbank und wird hier nicht
## nachgebaut. Umgesetzt ist die Fähigkeitenrotation des Konstrukts.
######*/

enum SocretharSpells
{
    SPELL_SOC_REVERBERATING_BLOW    = 182635,
    SPELL_SOC_SHATTERED_DEFENSES    = 182038,
    SPELL_SOC_FEL_PRISON            = 181288,
    SPELL_SOC_VOLATILE_FEL_ORB      = 180221,
    SPELL_SOC_FELBLAZE_CHARGE       = 182051,
    SPELL_SOC_APOCALYPTIC_FELBURST  = 188693,   // mythisch
    SPELL_SOC_EXERT_DOMINANCE       = 183331,
    SPELL_SOC_APOCALYPSE            = 183329
};

enum SocretharEvents
{
    EVENT_SOC_REVERBERATING_BLOW = 1,
    EVENT_SOC_FEL_PRISON,
    EVENT_SOC_VOLATILE_FEL_ORB,
    EVENT_SOC_FELBLAZE_CHARGE,
    EVENT_SOC_APOCALYPTIC_FELBURST,
    EVENT_SOC_EXERT_DOMINANCE
};

class boss_socrethar_the_eternal : public CreatureScript
{
    public:
        boss_socrethar_the_eternal() : CreatureScript("boss_socrethar_the_eternal") { }

        struct boss_socrethar_the_eternalAI : public BossAI
        {
            boss_socrethar_the_eternalAI(Creature* creature) : BossAI(creature, DATA_SOCRETHAR_ETERNAL) { }

            void Reset()
            {
                _Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_SOC_REVERBERATING_BLOW, 10000);
                events.ScheduleEvent(EVENT_SOC_VOLATILE_FEL_ORB, 15000);
                events.ScheduleEvent(EVENT_SOC_FEL_PRISON, 22000);
                events.ScheduleEvent(EVENT_SOC_FELBLAZE_CHARGE, 30000);
                events.ScheduleEvent(EVENT_SOC_EXERT_DOMINANCE, 60000);

                if (IsMythicRaid())
                    events.ScheduleEvent(EVENT_SOC_APOCALYPTIC_FELBURST, 40000);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _JustDied();
                summons.DespawnAll();
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SOC_REVERBERATING_BLOW:
                            // Legt beim Tank Zersplitterte Verteidigung (182038) an --
                            // der Zauber selbst traegt den Nachfolgeeffekt.
                            DoCastVictim(SPELL_SOC_REVERBERATING_BLOW);
                            events.ScheduleEvent(EVENT_SOC_REVERBERATING_BLOW, 20000);
                            break;
                        case EVENT_SOC_VOLATILE_FEL_ORB:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 60.0f, true))
                                DoCast(target, SPELL_SOC_VOLATILE_FEL_ORB);
                            events.ScheduleEvent(EVENT_SOC_VOLATILE_FEL_ORB, 25000);
                            break;
                        case EVENT_SOC_FEL_PRISON:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 60.0f, true))
                                DoCast(target, SPELL_SOC_FEL_PRISON);
                            events.ScheduleEvent(EVENT_SOC_FEL_PRISON, 35000);
                            break;
                        case EVENT_SOC_FELBLAZE_CHARGE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 60.0f, true))
                                DoCast(target, SPELL_SOC_FELBLAZE_CHARGE);
                            events.ScheduleEvent(EVENT_SOC_FELBLAZE_CHARGE, 30000);
                            break;
                        case EVENT_SOC_APOCALYPTIC_FELBURST:
                            DoCastAOE(SPELL_SOC_APOCALYPTIC_FELBURST);
                            events.ScheduleEvent(EVENT_SOC_APOCALYPTIC_FELBURST, 45000);
                            break;
                        case EVENT_SOC_EXERT_DOMINANCE:
                            // Socrethar reisst den Konstrukt an sich; im Journal die
                            // Klammer um Apokalypse (183329).
                            DoCast(me, SPELL_SOC_EXERT_DOMINANCE, true);
                            DoCastAOE(SPELL_SOC_APOCALYPSE);
                            events.ScheduleEvent(EVENT_SOC_EXERT_DOMINANCE, 70000);
                            break;
                        default:
                            break;
                    }

                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        return;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_socrethar_the_eternalAI(creature);
        }
};

/*######
## Tyrant Velhari -- three stages, each with its own aura over the whole raid
######*/

enum VelhariSpells
{
    SPELL_AURA_OF_OPPRESSION    = 181718,
    SPELL_AURA_OF_CONTEMPT      = 179986,
    SPELL_AURA_OF_MALICE        = 179991,
    SPELL_EDICT_OF_CONDEMNATION = 180128,
    SPELL_TOUCH_OF_HARM         = 180166,
    SPELL_SEAL_OF_DECAY         = 179999,
    SPELL_ANNIHILATING_STRIKE   = 180260,
    SPELL_TAINTED_SHADOWS       = 180533,
    SPELL_BULWARK_OF_THE_TYRANT = 180600,
    SPELL_GAVEL_OF_THE_TYRANT   = 180608
};

enum VelhariEvents
{
    EVENT_EDICT                 = 1,
    EVENT_TOUCH_OF_HARM         = 2,
    EVENT_SEAL_OF_DECAY         = 3,
    EVENT_STAGE_ABILITY         = 4
};

enum VelhariMisc
{
    VELHARI_CONTEMPT_PCT        = 66,
    VELHARI_MALICE_PCT          = 33
};

class boss_tyrant_velhari : public CreatureScript
{
    public:
        boss_tyrant_velhari() : CreatureScript("boss_tyrant_velhari") { }

        struct boss_tyrant_velhariAI : public BossAI
        {
            boss_tyrant_velhariAI(Creature* creature) : BossAI(creature, DATA_VELHARI) { }

            void Reset()
            {
                _Reset();
                _stage = 1;
                me->RemoveAurasDueToSpell(SPELL_AURA_OF_OPPRESSION);
                me->RemoveAurasDueToSpell(SPELL_AURA_OF_CONTEMPT);
                me->RemoveAurasDueToSpell(SPELL_AURA_OF_MALICE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                // The aura is the stage: it grows harsher as she loses health.
                DoCast(me, SPELL_AURA_OF_OPPRESSION, true);

                events.ScheduleEvent(EVENT_EDICT, urand(15000, 20000));
                events.ScheduleEvent(EVENT_TOUCH_OF_HARM, urand(12000, 16000));
                events.ScheduleEvent(EVENT_SEAL_OF_DECAY, urand(20000, 25000));
                events.ScheduleEvent(EVENT_STAGE_ABILITY, urand(8000, 12000));
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _JustDied();
                summons.DespawnAll();
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_stage == 1 && HealthBelowPct(VELHARI_CONTEMPT_PCT))
                {
                    _stage = 2;
                    Talk(SAY_SPECIAL);
                    me->RemoveAurasDueToSpell(SPELL_AURA_OF_OPPRESSION);
                    DoCast(me, SPELL_AURA_OF_CONTEMPT, true);
                }
                else if (_stage == 2 && HealthBelowPct(VELHARI_MALICE_PCT))
                {
                    _stage = 3;
                    Talk(SAY_SPECIAL);
                    me->RemoveAurasDueToSpell(SPELL_AURA_OF_CONTEMPT);
                    DoCast(me, SPELL_AURA_OF_MALICE, true);
                    DoCast(me, SPELL_BULWARK_OF_THE_TYRANT, true);
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_EDICT:
                            DoCastAOE(SPELL_EDICT_OF_CONDEMNATION);
                            events.ScheduleEvent(EVENT_EDICT, urand(15000, 20000));
                            break;
                        case EVENT_TOUCH_OF_HARM:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_TOUCH_OF_HARM);
                            events.ScheduleEvent(EVENT_TOUCH_OF_HARM, urand(12000, 16000));
                            break;
                        case EVENT_SEAL_OF_DECAY:
                            DoCastVictim(SPELL_SEAL_OF_DECAY);
                            events.ScheduleEvent(EVENT_SEAL_OF_DECAY, urand(20000, 25000));
                            break;
                        case EVENT_STAGE_ABILITY:
                            switch (_stage)
                            {
                                case 1:  DoCastVictim(SPELL_ANNIHILATING_STRIKE); break;
                                case 2:  DoCastAOE(SPELL_TAINTED_SHADOWS);        break;
                                default: DoCastVictim(SPELL_GAVEL_OF_THE_TYRANT); break;
                            }
                            events.ScheduleEvent(EVENT_STAGE_ABILITY, urand(8000, 12000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint8 _stage;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_tyrant_velhariAI(creature);
        }
};

/*######
## Fel Lord Zakuun -- his soul has to be knocked out of him before he can die
######*/

enum ZakuunSpells
{
    SPELL_LATENT_ENERGY         = 182008,
    SPELL_UNLEASHED_ENERGY      = 181825,
    SPELL_WAKE_OF_DESTRUCTION   = 181499,
    SPELL_RUMBLING_FISSURE      = 179582,
    SPELL_SOUL_CLEAVE           = 179406,
    SPELL_DISEMBODIED           = 179407,
    SPELL_BEFOULED              = 179711,
    SPELL_SEED_OF_DESTRUCTION   = 181508
};

enum ZakuunEvents
{
    EVENT_SOUL_CLEAVE           = 1,
    EVENT_WAKE_OF_DESTRUCTION   = 2,
    EVENT_RUMBLING_FISSURE      = 3,
    EVENT_SEED_OF_DESTRUCTION   = 4,
    EVENT_DISEMBODY             = 5
};

class boss_fel_lord_zakuun : public CreatureScript
{
    public:
        boss_fel_lord_zakuun() : CreatureScript("boss_fel_lord_zakuun") { }

        struct boss_fel_lord_zakuunAI : public BossAI
        {
            boss_fel_lord_zakuunAI(Creature* creature) : BossAI(creature, DATA_ZAKUUN) { }

            void Reset()
            {
                _Reset();
                me->RemoveAurasDueToSpell(SPELL_LATENT_ENERGY);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                DoCast(me, SPELL_LATENT_ENERGY, true);

                events.ScheduleEvent(EVENT_SOUL_CLEAVE, urand(10000, 14000));
                events.ScheduleEvent(EVENT_WAKE_OF_DESTRUCTION, urand(15000, 20000));
                events.ScheduleEvent(EVENT_RUMBLING_FISSURE, urand(12000, 16000));
                events.ScheduleEvent(EVENT_SEED_OF_DESTRUCTION, urand(25000, 30000));
                events.ScheduleEvent(EVENT_DISEMBODY, 40000);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _JustDied();
                summons.DespawnAll();
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SOUL_CLEAVE:
                            // Splits the target's soul off; the halves have to
                            // be dealt with separately.
                            DoCastVictim(SPELL_SOUL_CLEAVE);
                            DoCastVictim(SPELL_DISEMBODIED, true);
                            events.ScheduleEvent(EVENT_SOUL_CLEAVE, urand(10000, 14000));
                            break;
                        case EVENT_WAKE_OF_DESTRUCTION:
                            DoCastAOE(SPELL_WAKE_OF_DESTRUCTION);
                            events.ScheduleEvent(EVENT_WAKE_OF_DESTRUCTION, urand(15000, 20000));
                            break;
                        case EVENT_RUMBLING_FISSURE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_RUMBLING_FISSURE);
                            events.ScheduleEvent(EVENT_RUMBLING_FISSURE, urand(12000, 16000));
                            break;
                        case EVENT_SEED_OF_DESTRUCTION:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_SEED_OF_DESTRUCTION);
                            events.ScheduleEvent(EVENT_SEED_OF_DESTRUCTION, urand(25000, 30000));
                            break;
                        case EVENT_DISEMBODY:
                            Talk(SAY_SPECIAL);
                            DoCast(me, SPELL_UNLEASHED_ENERGY, true);
                            DoCastAOE(SPELL_BEFOULED);
                            events.ScheduleEvent(EVENT_DISEMBODY, urand(45000, 55000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_fel_lord_zakuunAI(creature);
        }
};

/*######
## Xhul'horac -- fel first, then void, then both at once
######*/

enum XhulSpells
{
    SPELL_FEL_STRIKE            = 186271,
    SPELL_FEL_SURGE             = 186407,
    SPELL_CHAINS_OF_FEL         = 186490,
    SPELL_FEL_ORB               = 186532,
    SPELL_VOID_STRIKE           = 186292,
    SPELL_VOID_SURGE            = 186333,
    SPELL_WITHERING_GAZE        = 186785,
    SPELL_BLACK_HOLE            = 186546,
    SPELL_SHADOWFEL_PHASING     = 189047,
    SPELL_OVERWHELMING_CHAOS    = 187204
};

enum XhulEvents
{
    EVENT_XHUL_STRIKE           = 1,
    EVENT_XHUL_SURGE            = 2,
    EVENT_XHUL_SPECIAL          = 3,
    EVENT_OVERWHELMING_CHAOS    = 4
};

enum XhulMisc
{
    XHUL_VOID_PCT               = 65,
    XHUL_AMALGAM_PCT            = 35
};

class boss_xhulhorac : public CreatureScript
{
    public:
        boss_xhulhorac() : CreatureScript("boss_xhulhorac") { }

        struct boss_xhulhoracAI : public BossAI
        {
            boss_xhulhoracAI(Creature* creature) : BossAI(creature, DATA_XHULHORAC) { }

            void Reset()
            {
                _Reset();
                _stage = 1;
                me->RemoveAurasDueToSpell(SPELL_SHADOWFEL_PHASING);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_XHUL_STRIKE, urand(8000, 11000));
                events.ScheduleEvent(EVENT_XHUL_SURGE, urand(15000, 20000));
                events.ScheduleEvent(EVENT_XHUL_SPECIAL, urand(20000, 25000));
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _JustDied();
                summons.DespawnAll();
            }

            // He works through fel, then void, then holds both -- and once he
            // does, the fight is on a timer.
            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_stage == 1 && HealthBelowPct(XHUL_VOID_PCT))
                {
                    _stage = 2;
                    Talk(SAY_SPECIAL);
                }
                else if (_stage == 2 && HealthBelowPct(XHUL_AMALGAM_PCT))
                {
                    _stage = 3;
                    Talk(SAY_SPECIAL);
                    DoCast(me, SPELL_SHADOWFEL_PHASING, true);
                    events.ScheduleEvent(EVENT_OVERWHELMING_CHAOS, 60000);
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_XHUL_STRIKE:
                            DoCastVictim(_stage == 1 ? SPELL_FEL_STRIKE : SPELL_VOID_STRIKE);
                            events.ScheduleEvent(EVENT_XHUL_STRIKE, urand(8000, 11000));
                            break;
                        case EVENT_XHUL_SURGE:
                            DoCastAOE(_stage == 1 ? SPELL_FEL_SURGE : SPELL_VOID_SURGE);
                            events.ScheduleEvent(EVENT_XHUL_SURGE, urand(15000, 20000));
                            break;
                        case EVENT_XHUL_SPECIAL:
                            if (_stage == 1)
                            {
                                DoCastAOE(SPELL_CHAINS_OF_FEL);
                                DoCastAOE(SPELL_FEL_ORB);
                            }
                            else
                            {
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                    DoCast(target, SPELL_WITHERING_GAZE);
                                DoCastAOE(SPELL_BLACK_HOLE);
                            }
                            events.ScheduleEvent(EVENT_XHUL_SPECIAL, urand(20000, 25000));
                            break;
                        case EVENT_OVERWHELMING_CHAOS:
                            DoCastAOE(SPELL_OVERWHELMING_CHAOS);
                            events.ScheduleEvent(EVENT_OVERWHELMING_CHAOS, 30000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint8 _stage;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_xhulhoracAI(creature);
        }
};

/*######
## Mannoroth -- dies, is raised by Gul'dan, and comes back worse
######*/

enum MannorothSpells
{
    SPELL_CURSE_OF_THE_LEGION   = 181275,
    SPELL_MARK_OF_DOOM          = 181099,
    SPELL_SHADOW_BOLT_VOLLEY    = 181126,
    SPELL_GLAIVE_COMBO          = 181354,
    SPELL_MASSIVE_BLAST         = 181359,
    SPELL_FEL_HELLSTORM         = 181557,
    SPELL_MANNOROTHS_GAZE       = 181597,
    SPELL_FELSEEKER             = 181735,
    SPELL_SHADOWFORCE           = 181799,
    SPELL_EMPOWERED_SHADOWFORCE = 182084
};

enum MannorothEvents
{
    EVENT_MARK_OF_DOOM          = 1,
    EVENT_SHADOW_BOLT_VOLLEY    = 2,
    EVENT_GLAIVE_COMBO          = 3,
    EVENT_MASSIVE_BLAST         = 4,
    EVENT_FELSEEKER             = 5,
    EVENT_SHADOWFORCE           = 6
};

enum MannorothMisc
{
    MANNOROTH_RESURRECTION_PCT  = 70,
    MANNOROTH_TRUE_POWER_PCT    = 40,
    MANNOROTH_EMPOWERED_PCT     = 20
};

class boss_mannoroth_hfc : public CreatureScript
{
    public:
        boss_mannoroth_hfc() : CreatureScript("boss_mannoroth_hfc") { }

        struct boss_mannoroth_hfcAI : public BossAI
        {
            boss_mannoroth_hfcAI(Creature* creature) : BossAI(creature, DATA_MANNOROTH) { }

            void Reset()
            {
                _Reset();
                _stage = 1;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                DoCast(me, SPELL_CURSE_OF_THE_LEGION, true);

                events.ScheduleEvent(EVENT_MARK_OF_DOOM, urand(12000, 16000));
                events.ScheduleEvent(EVENT_SHADOW_BOLT_VOLLEY, urand(8000, 12000));
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _JustDied();
                summons.DespawnAll();
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_stage == 1 && HealthBelowPct(MANNOROTH_RESURRECTION_PCT))
                {
                    // Gul'dan puts him back on his feet with the glaives.
                    _stage = 2;
                    Talk(SAY_SPECIAL);
                    events.CancelEvent(EVENT_SHADOW_BOLT_VOLLEY);
                    events.ScheduleEvent(EVENT_GLAIVE_COMBO, urand(10000, 14000));
                    events.ScheduleEvent(EVENT_MASSIVE_BLAST, urand(15000, 20000));
                    events.ScheduleEvent(EVENT_FELSEEKER, urand(25000, 30000));
                }
                else if (_stage == 2 && HealthBelowPct(MANNOROTH_TRUE_POWER_PCT))
                {
                    _stage = 3;
                    Talk(SAY_SPECIAL);
                    events.ScheduleEvent(EVENT_SHADOWFORCE, urand(20000, 25000));
                }
                else if (_stage == 3 && HealthBelowPct(MANNOROTH_EMPOWERED_PCT))
                {
                    _stage = 4;
                    Talk(SAY_SPECIAL);
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MARK_OF_DOOM:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_MARK_OF_DOOM);
                            events.ScheduleEvent(EVENT_MARK_OF_DOOM, urand(12000, 16000));
                            break;
                        case EVENT_SHADOW_BOLT_VOLLEY:
                            DoCastAOE(SPELL_SHADOW_BOLT_VOLLEY);
                            events.ScheduleEvent(EVENT_SHADOW_BOLT_VOLLEY, urand(8000, 12000));
                            break;
                        case EVENT_GLAIVE_COMBO:
                            DoCastVictim(SPELL_GLAIVE_COMBO);
                            events.ScheduleEvent(EVENT_GLAIVE_COMBO, urand(10000, 14000));
                            break;
                        case EVENT_MASSIVE_BLAST:
                            DoCastVictim(SPELL_MASSIVE_BLAST);
                            events.ScheduleEvent(EVENT_MASSIVE_BLAST, urand(15000, 20000));
                            break;
                        case EVENT_FELSEEKER:
                            DoCastAOE(SPELL_FEL_HELLSTORM);
                            DoCastAOE(SPELL_FELSEEKER);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_MANNOROTHS_GAZE);
                            events.ScheduleEvent(EVENT_FELSEEKER, urand(25000, 30000));
                            break;
                        case EVENT_SHADOWFORCE:
                            DoCastAOE(_stage >= 4 ? SPELL_EMPOWERED_SHADOWFORCE : SPELL_SHADOWFORCE);
                            events.ScheduleEvent(EVENT_SHADOWFORCE, urand(20000, 25000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint8 _stage;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_mannoroth_hfcAI(creature);
        }
};

/*######
## Archimonde -- the end of Draenor
######*/

enum ArchimondeSpells
{
    SPELL_SHADOW_BLAST          = 183864,
    SPELL_DEATH_BRAND           = 183828,
    SPELL_DOOMFIRE              = 182826,
    SPELL_ALLURE_OF_FLAMES      = 183254,
    SPELL_SHACKLED_TORMENT      = 184931,
    SPELL_WROUGHT_CHAOS         = 184265,
    SPELL_DESECRATE             = 185590,
    SPELL_NETHER_BANISH         = 186961,
    SPELL_RAIN_OF_CHAOS         = 182225,
    SPELL_SHADOWFEL_BURST       = 183817
};

enum ArchimondeEvents
{
    EVENT_SHADOW_BLAST          = 1,
    EVENT_DEATH_BRAND           = 2,
    EVENT_DOOMFIRE              = 3,
    EVENT_SHACKLED_TORMENT      = 4,
    EVENT_WROUGHT_CHAOS         = 5,
    EVENT_DESECRATE             = 6,
    EVENT_NETHER_BANISH         = 7,
    EVENT_RAIN_OF_CHAOS         = 8
};

enum ArchimondeMisc
{
    ARCHIMONDE_STAGE_TWO_PCT    = 70,
    ARCHIMONDE_STAGE_THREE_PCT  = 30
};

class boss_hfc_archimonde : public CreatureScript
{
    public:
        boss_hfc_archimonde() : CreatureScript("boss_hfc_archimonde") { }

        struct boss_hfc_archimondeAI : public BossAI
        {
            boss_hfc_archimondeAI(Creature* creature) : BossAI(creature, DATA_ARCHIMONDE) { }

            void Reset()
            {
                _Reset();
                _stage = 1;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_SHADOW_BLAST, urand(8000, 11000));
                events.ScheduleEvent(EVENT_DEATH_BRAND, urand(15000, 20000));
                events.ScheduleEvent(EVENT_DOOMFIRE, urand(25000, 30000));
                events.ScheduleEvent(EVENT_SHACKLED_TORMENT, urand(20000, 25000));
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _JustDied();
                summons.DespawnAll();
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_stage == 1 && HealthBelowPct(ARCHIMONDE_STAGE_TWO_PCT))
                {
                    _stage = 2;
                    Talk(SAY_SPECIAL);
                    events.ScheduleEvent(EVENT_WROUGHT_CHAOS, urand(15000, 20000));
                    events.ScheduleEvent(EVENT_DESECRATE, urand(25000, 30000));
                }
                else if (_stage == 2 && HealthBelowPct(ARCHIMONDE_STAGE_THREE_PCT))
                {
                    // The last third is fought in the Twisting Nether.
                    _stage = 3;
                    Talk(SAY_SPECIAL);
                    events.ScheduleEvent(EVENT_NETHER_BANISH, 10000);
                    events.ScheduleEvent(EVENT_RAIN_OF_CHAOS, urand(20000, 25000));
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHADOW_BLAST:
                            DoCastVictim(SPELL_SHADOW_BLAST);
                            events.ScheduleEvent(EVENT_SHADOW_BLAST, urand(8000, 11000));
                            break;
                        case EVENT_DEATH_BRAND:
                            DoCastAOE(SPELL_DEATH_BRAND);
                            events.ScheduleEvent(EVENT_DEATH_BRAND, urand(15000, 20000));
                            break;
                        case EVENT_DOOMFIRE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            {
                                DoCast(target, SPELL_DOOMFIRE);
                                DoCast(target, SPELL_ALLURE_OF_FLAMES, true);
                            }
                            events.ScheduleEvent(EVENT_DOOMFIRE, urand(25000, 30000));
                            break;
                        case EVENT_SHACKLED_TORMENT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_SHACKLED_TORMENT);
                            events.ScheduleEvent(EVENT_SHACKLED_TORMENT, urand(20000, 25000));
                            break;
                        case EVENT_WROUGHT_CHAOS:
                            DoCastAOE(SPELL_WROUGHT_CHAOS);
                            events.ScheduleEvent(EVENT_WROUGHT_CHAOS, urand(15000, 20000));
                            break;
                        case EVENT_DESECRATE:
                            DoCastAOE(SPELL_DESECRATE);
                            events.ScheduleEvent(EVENT_DESECRATE, urand(25000, 30000));
                            break;
                        case EVENT_NETHER_BANISH:
                            DoCastAOE(SPELL_NETHER_BANISH);
                            DoCastAOE(SPELL_SHADOWFEL_BURST);
                            events.ScheduleEvent(EVENT_NETHER_BANISH, urand(30000, 40000));
                            break;
                        case EVENT_RAIN_OF_CHAOS:
                            DoCastAOE(SPELL_RAIN_OF_CHAOS);
                            events.ScheduleEvent(EVENT_RAIN_OF_CHAOS, urand(20000, 25000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint8 _stage;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_hfc_archimondeAI(creature);
        }
};

void AddSC_hfc_upper()
{
    new boss_shadow_lord_iskar();
    new boss_socrethar_the_eternal();
    new boss_tyrant_velhari();
    new boss_fel_lord_zakuun();
    new boss_xhulhorac();
    new boss_mannoroth_hfc();
    new boss_hfc_archimonde();
}
