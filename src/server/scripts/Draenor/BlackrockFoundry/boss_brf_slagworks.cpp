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
SDName: Blackrock_Foundry_Slagworks
SD%Complete: 75
SDComment: Gruul, Oregorger and the Blast Furnace. The raid shipped with no
           encounters at all, only a header naming them.

           Spell ids come from the Warlords of Draenor encounter journal on
           Wowhead, one lookup per boss. What is modelled here is each fight's
           shape -- Gruul's petrify-and-shatter, Oregorger's rolling phase, the
           furnace's heat build-up -- not every mythic-only refinement.
SDCategory: Blackrock Foundry
EndScriptData */

#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackrock_foundry.h"

enum SlagworksTexts
{
    SAY_AGGRO       = 0,
    SAY_SLAY        = 1,
    SAY_DEATH       = 2,
    SAY_SPECIAL     = 3
};

/*######
## Gruul -- the tank stacks Overwhelming Blows until someone else takes over,
## and Petrifying Slam turns players to stone for Shatter to break.
######*/

enum GruulSpells
{
    SPELL_INFERNO_SLICE         = 155080,
    SPELL_OVERWHELMING_BLOWS    = 155078,
    SPELL_PETRIFYING_SLAM       = 155326,
    SPELL_SHATTER               = 155530,
    SPELL_OVERHEAD_SMASH        = 155301,
    SPELL_CAVE_IN               = 173192,
    SPELL_DESTRUCTIVE_RAMPAGE   = 155539
};

enum GruulEvents
{
    EVENT_INFERNO_SLICE         = 1,
    EVENT_PETRIFYING_SLAM       = 2,
    EVENT_OVERHEAD_SMASH        = 3,
    EVENT_CAVE_IN               = 4,
    EVENT_SHATTER               = 5
};

enum GruulMisc
{
    GRUUL_RAMPAGE_PCT           = 30
};

class boss_gruul_brf : public CreatureScript
{
    public:
        boss_gruul_brf() : CreatureScript("boss_gruul_brf") { }

        struct boss_gruul_brfAI : public BossAI
        {
            boss_gruul_brfAI(Creature* creature) : BossAI(creature, DataGruul) { }

            void Reset()
            {
                _Reset();
                _rampaging = false;
                me->RemoveAurasDueToSpell(SPELL_DESTRUCTIVE_RAMPAGE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_INFERNO_SLICE, urand(8000, 10000));
                events.ScheduleEvent(EVENT_PETRIFYING_SLAM, urand(25000, 30000));
                events.ScheduleEvent(EVENT_OVERHEAD_SMASH, urand(15000, 20000));
                events.ScheduleEvent(EVENT_CAVE_IN, urand(20000, 25000));
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
            }

            // The last third is a hard enrage rather than a new phase.
            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_rampaging || !HealthBelowPct(GRUUL_RAMPAGE_PCT))
                    return;

                _rampaging = true;
                Talk(SAY_SPECIAL);
                DoCast(me, SPELL_DESTRUCTIVE_RAMPAGE, true);
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
                        case EVENT_INFERNO_SLICE:
                            // Cleaves the tank and stacks Overwhelming Blows,
                            // which is what forces the swap.
                            DoCastVictim(SPELL_INFERNO_SLICE);
                            DoCastVictim(SPELL_OVERWHELMING_BLOWS, true);
                            events.ScheduleEvent(EVENT_INFERNO_SLICE, urand(8000, 10000));
                            break;
                        case EVENT_PETRIFYING_SLAM:
                            DoCastAOE(SPELL_PETRIFYING_SLAM);
                            // Shatter follows on its own; standing petrified
                            // together is what kills people.
                            events.ScheduleEvent(EVENT_SHATTER, 6000);
                            events.ScheduleEvent(EVENT_PETRIFYING_SLAM, urand(45000, 55000));
                            break;
                        case EVENT_SHATTER:
                            DoCastAOE(SPELL_SHATTER);
                            break;
                        case EVENT_OVERHEAD_SMASH:
                            DoCastVictim(SPELL_OVERHEAD_SMASH);
                            events.ScheduleEvent(EVENT_OVERHEAD_SMASH, urand(15000, 20000));
                            break;
                        case EVENT_CAVE_IN:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_CAVE_IN);
                            events.ScheduleEvent(EVENT_CAVE_IN, urand(20000, 25000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool _rampaging;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_gruul_brfAI(creature);
        }
};

/*######
## Oregorger -- eats his way around the room, then rolls
######*/

enum OregorgerSpells
{
    SPELL_ACID_TORRENT          = 156324,
    SPELL_RETCHED_BLACKROCK     = 156203,
    SPELL_EXPLOSIVE_SHARD       = 156388,
    SPELL_ACID_MAW              = 173471,
    SPELL_HUNGER_DRIVE          = 155819,
    SPELL_ROLLING_FURY          = 155900,
    SPELL_EARTHSHAKING_COLLISION = 155897,
    SPELL_BLACKROCK_BARRAGE     = 156879
};

enum OregorgerEvents
{
    EVENT_ACID_TORRENT          = 1,
    EVENT_RETCHED_BLACKROCK     = 2,
    EVENT_EXPLOSIVE_SHARD       = 3,
    EVENT_ROLL_END              = 4,
    EVENT_BLACKROCK_BARRAGE     = 5
};

enum OregorgerMisc
{
    // He rolls off at each of these and comes back when the timer runs out.
    OREGORGER_ROLL_PCT_STEP     = 30,
    OREGORGER_ROLL_DURATION     = 45000
};

class boss_oregorger : public CreatureScript
{
    public:
        boss_oregorger() : CreatureScript("boss_oregorger") { }

        struct boss_oregorgerAI : public BossAI
        {
            boss_oregorgerAI(Creature* creature) : BossAI(creature, DataOregorger) { }

            void Reset()
            {
                _Reset();
                _rolling = false;
                _nextRollPct = 100 - OREGORGER_ROLL_PCT_STEP;
                me->RemoveAurasDueToSpell(SPELL_ROLLING_FURY);
                me->SetReactState(REACT_AGGRESSIVE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);
                ScheduleCombatEvents();
            }

            void ScheduleCombatEvents()
            {
                events.ScheduleEvent(EVENT_ACID_TORRENT, urand(8000, 11000));
                events.ScheduleEvent(EVENT_RETCHED_BLACKROCK, urand(12000, 16000));
                events.ScheduleEvent(EVENT_EXPLOSIVE_SHARD, urand(15000, 20000));
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
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*dmgType*/)
            {
                if (_rolling)
                {
                    // Untouchable while he is careening around the room.
                    damage = 0;
                    return;
                }

                if (_nextRollPct == 0 || !HealthBelowPct(_nextRollPct))
                    return;

                StartRolling();
            }

            void StartRolling()
            {
                _rolling = true;
                Talk(SAY_SPECIAL);

                if (_nextRollPct > OREGORGER_ROLL_PCT_STEP)
                    _nextRollPct -= OREGORGER_ROLL_PCT_STEP;
                else
                    _nextRollPct = 0;

                events.Reset();
                me->AttackStop();
                me->SetReactState(REACT_PASSIVE);

                DoCast(me, SPELL_HUNGER_DRIVE, true);
                DoCast(me, SPELL_ROLLING_FURY, true);

                events.ScheduleEvent(EVENT_BLACKROCK_BARRAGE, 3000);
                events.ScheduleEvent(EVENT_ROLL_END, OREGORGER_ROLL_DURATION);
            }

            void EndRolling()
            {
                _rolling = false;
                me->RemoveAurasDueToSpell(SPELL_ROLLING_FURY);
                me->RemoveAurasDueToSpell(SPELL_HUNGER_DRIVE);
                me->SetReactState(REACT_AGGRESSIVE);

                events.Reset();
                ScheduleCombatEvents();

                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                    AttackStart(target);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() && !_rolling)
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ACID_TORRENT:
                            // A frontal cone, so he is turned away from the raid.
                            DoCastVictim(SPELL_ACID_TORRENT);
                            DoCastVictim(SPELL_ACID_MAW, true);
                            events.ScheduleEvent(EVENT_ACID_TORRENT, urand(8000, 11000));
                            break;
                        case EVENT_RETCHED_BLACKROCK:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_RETCHED_BLACKROCK);
                            events.ScheduleEvent(EVENT_RETCHED_BLACKROCK, urand(12000, 16000));
                            break;
                        case EVENT_EXPLOSIVE_SHARD:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_EXPLOSIVE_SHARD);
                            events.ScheduleEvent(EVENT_EXPLOSIVE_SHARD, urand(15000, 20000));
                            break;
                        case EVENT_BLACKROCK_BARRAGE:
                            DoCastAOE(SPELL_BLACKROCK_BARRAGE);
                            events.ScheduleEvent(EVENT_BLACKROCK_BARRAGE, 8000);
                            break;
                        case EVENT_ROLL_END:
                            DoCastAOE(SPELL_EARTHSHAKING_COLLISION);
                            EndRolling();
                            break;
                        default:
                            break;
                    }
                }

                if (!_rolling)
                    DoMeleeAttackIfReady();
            }

        private:
            bool _rolling;
            uint32 _nextRollPct;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_oregorgerAI(creature);
        }
};

/*######
## The Blast Furnace -- the Heart is the encounter, Feldspar keeps it running
######*/

enum FurnaceSpells
{
    SPELL_HEAT                  = 155242,
    SPELL_MELT                  = 155225,
    SPELL_TEMPERED              = 155240,
    SPELL_SLAG_POOL             = 163532,
    SPELL_RUPTURE               = 156934,
    SPELL_PYROCLASM             = 156937,
    SPELL_VOLATILE_FIRE         = 176121,
    SPELL_HOT_BLOODED           = 158247
};

enum FurnaceEvents
{
    EVENT_HEAT                  = 1,
    EVENT_MELT                  = 2,
    EVENT_SLAG_POOL             = 3,
    EVENT_RUPTURE               = 4,
    EVENT_PYROCLASM             = 5,
    EVENT_VOLATILE_FIRE         = 6
};

enum FurnaceMisc
{
    // The Heart stokes itself all fight; this is what makes the encounter a
    // race rather than a damage check.
    FURNACE_HEAT_INTERVAL       = 5000,
    FURNACE_SHIELD_DOWN_PCT     = 50
};

class boss_heart_of_the_mountain : public CreatureScript
{
    public:
        boss_heart_of_the_mountain() : CreatureScript("boss_heart_of_the_mountain") { }

        struct boss_heart_of_the_mountainAI : public BossAI
        {
            boss_heart_of_the_mountainAI(Creature* creature) : BossAI(creature, DataBlastFurnace) { }

            void Reset()
            {
                _Reset();
                _secondStage = false;
                me->RemoveAurasDueToSpell(SPELL_TEMPERED);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_HEAT, FURNACE_HEAT_INTERVAL);
                events.ScheduleEvent(EVENT_SLAG_POOL, urand(15000, 20000));
                events.ScheduleEvent(EVENT_MELT, urand(20000, 25000));
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
            }

            // Once the shielding is gone the furnace itself starts erupting.
            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_secondStage || !HealthBelowPct(FURNACE_SHIELD_DOWN_PCT))
                    return;

                _secondStage = true;
                Talk(SAY_SPECIAL);

                events.ScheduleEvent(EVENT_RUPTURE, 5000);
                events.ScheduleEvent(EVENT_PYROCLASM, urand(20000, 25000));
                events.ScheduleEvent(EVENT_VOLATILE_FIRE, urand(12000, 16000));
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
                        case EVENT_HEAT:
                            DoCast(me, SPELL_HEAT, true);
                            events.ScheduleEvent(EVENT_HEAT, FURNACE_HEAT_INTERVAL);
                            break;
                        case EVENT_MELT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_MELT);
                            events.ScheduleEvent(EVENT_MELT, urand(20000, 25000));
                            break;
                        case EVENT_SLAG_POOL:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_SLAG_POOL);
                            events.ScheduleEvent(EVENT_SLAG_POOL, urand(15000, 20000));
                            break;
                        case EVENT_RUPTURE:
                            DoCastAOE(SPELL_RUPTURE);
                            events.ScheduleEvent(EVENT_RUPTURE, urand(15000, 20000));
                            break;
                        case EVENT_PYROCLASM:
                            DoCastAOE(SPELL_PYROCLASM);
                            events.ScheduleEvent(EVENT_PYROCLASM, urand(20000, 25000));
                            break;
                        case EVENT_VOLATILE_FIRE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_VOLATILE_FIRE);
                            events.ScheduleEvent(EVENT_VOLATILE_FIRE, urand(12000, 16000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool _secondStage;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_heart_of_the_mountainAI(creature);
        }
};

// Feldspar is not a boss in his own right -- he keeps the furnace stoked and
// burns anyone who comes to shut him down.
class npc_foreman_feldspar : public CreatureScript
{
    public:
        npc_foreman_feldspar() : CreatureScript("npc_foreman_feldspar") { }

        struct npc_foreman_feldsparAI : public ScriptedAI
        {
            npc_foreman_feldsparAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                me->RemoveAurasDueToSpell(SPELL_HOT_BLOODED);
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoCast(me, SPELL_HOT_BLOODED, true);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_foreman_feldsparAI(creature);
        }
};

void AddSC_brf_slagworks()
{
    new boss_gruul_brf();
    new boss_oregorger();
    new boss_heart_of_the_mountain();
    new npc_foreman_feldspar();
}
