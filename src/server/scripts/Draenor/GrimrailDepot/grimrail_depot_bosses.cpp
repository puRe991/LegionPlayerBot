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
SDName: Grimrail_Depot_Bosses
SD%Complete: 75
SDComment: The three encounters of Grimrail Depot. Rocketspark and Borka share
           an encounter: Borka does the hitting while Rocketspark shells the
           platform from his jetpack, and killing Borka pushes Rocketspark into
           his missile phase. Nitrogg abandons the fight at 60% for his assault
           cannon. Tovra fights while her rylak seeds the ground with pools.

           Entries and spells were looked up against the Warlords of Draenor
           data on Wowhead. Check them against creature_template and the spell
           store before relying on them.
SDCategory: Grimrail Depot
EndScriptData */

#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "grimrail_depot.h"

enum GDSpells
{
    // Railmaster Rocketspark and Borka the Brute
    SPELL_LOCKING_ON            = 162075,
    SPELL_MISSILE_BARRAGE       = 161588,
    SPELL_NEW_PLAN              = 162058,
    SPELL_MAD_DASH              = 162066,
    SPELL_SLAM                  = 162070,

    // Nitrogg Thundertower
    SPELL_SHREDDING_SWIPE       = 165102,
    SPELL_CANNON_BLAST          = 162078,
    SPELL_THUNDER_SLAM          = 162090,

    // Skylord Tovra
    SPELL_FREEZING_SNARE        = 168398,
    SPELL_SPINNING_SPEAR        = 168345,
    SPELL_DIFFUSED_ENERGY       = 168318
};

enum GDEvents
{
    EVENT_LOCKING_ON            = 1,
    EVENT_MISSILE_BARRAGE       = 2,
    EVENT_MAD_DASH              = 3,
    EVENT_SLAM                  = 4,
    EVENT_SHREDDING_SWIPE       = 5,
    EVENT_THUNDER_SLAM          = 6,
    EVENT_FREEZING_SNARE        = 7,
    EVENT_SPINNING_SPEAR        = 8,
    EVENT_DIFFUSED_ENERGY       = 9
};

enum GDTexts
{
    SAY_AGGRO                   = 0,
    SAY_SLAY                    = 1,
    SAY_DEATH                   = 2,
    SAY_SPECIAL                 = 3
};

enum GDMisc
{
    NITROGG_CANNON_HEALTH_PCT   = 60
};

/*######
## boss_railmaster_rocketspark -- the ranged half of the first encounter
######*/

class boss_railmaster_rocketspark : public CreatureScript
{
    public:
        boss_railmaster_rocketspark() : CreatureScript("boss_railmaster_rocketspark") { }

        struct boss_railmaster_rocketsparkAI : public ScriptedAI
        {
            boss_railmaster_rocketsparkAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset() override
            {
                _events.Reset();
                _borkaDown = false;
                me->RemoveAurasDueToSpell(SPELL_NEW_PLAN);
            }

            void EnterCombat(Unit* who) override
            {
                Talk(SAY_AGGRO);

                if (_instance)
                    _instance->SetBossState(DATA_ROCKETSPARK_AND_BORKA, IN_PROGRESS);

                // The two are one encounter; pulling either brings both.
                if (_instance)
                    if (Creature* borka = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(DATA_BORKA_GUID)))
                        if (borka->isAlive() && !borka->isInCombat())
                            borka->AI()->AttackStart(who);

                _events.ScheduleEvent(EVENT_LOCKING_ON, urand(8000, 12000));
                _events.ScheduleEvent(EVENT_MISSILE_BARRAGE, urand(15000, 20000));
            }

            // Losing Borka is what makes him dangerous rather than what beats
            // him: he stops aiming and simply floods the platform.
            void DoAction(int32 const action) override
            {
                if (action != ACTION_BORKA_DIED || _borkaDown)
                    return;

                _borkaDown = true;
                Talk(SAY_SPECIAL);
                DoCast(me, SPELL_NEW_PLAN, true);
                _events.RescheduleEvent(EVENT_MISSILE_BARRAGE, 5000);
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/) override
            {
                Talk(SAY_DEATH);

                if (_instance)
                    _instance->SetBossState(DATA_ROCKETSPARK_AND_BORKA, DONE);
            }

            void EnterEvadeMode() override
            {
                if (_instance)
                    _instance->SetBossState(DATA_ROCKETSPARK_AND_BORKA, FAIL);

                ScriptedAI::EnterEvadeMode();
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_LOCKING_ON:
                            // Interruptible on purpose -- left alone it runs
                            // away with the fight.
                            DoCastVictim(SPELL_LOCKING_ON);
                            _events.ScheduleEvent(EVENT_LOCKING_ON, urand(15000, 20000));
                            break;
                        case EVENT_MISSILE_BARRAGE:
                            DoCastAOE(SPELL_MISSILE_BARRAGE);
                            _events.ScheduleEvent(EVENT_MISSILE_BARRAGE, _borkaDown ? urand(8000, 12000) : urand(15000, 20000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* _instance;
            EventMap _events;
            bool _borkaDown;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_railmaster_rocketsparkAI(creature);
        }
};

/*######
## boss_borka_the_brute -- the melee half
######*/

class boss_borka_the_brute : public CreatureScript
{
    public:
        boss_borka_the_brute() : CreatureScript("boss_borka_the_brute") { }

        struct boss_borka_the_bruteAI : public ScriptedAI
        {
            boss_borka_the_bruteAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset() override
            {
                _events.Reset();
            }

            void EnterCombat(Unit* who) override
            {
                if (_instance)
                    if (Creature* rocketspark = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(DATA_ROCKETSPARK_GUID)))
                        if (rocketspark->isAlive() && !rocketspark->isInCombat())
                            rocketspark->AI()->AttackStart(who);

                _events.ScheduleEvent(EVENT_MAD_DASH, urand(10000, 14000));
                _events.ScheduleEvent(EVENT_SLAM, urand(6000, 9000));
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (!_instance)
                    return;

                if (Creature* rocketspark = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(DATA_ROCKETSPARK_GUID)))
                    if (rocketspark->AI())
                        rocketspark->AI()->DoAction(ACTION_BORKA_DIED);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MAD_DASH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_MAD_DASH);
                            _events.ScheduleEvent(EVENT_MAD_DASH, urand(10000, 14000));
                            break;
                        case EVENT_SLAM:
                            DoCastVictim(SPELL_SLAM);
                            _events.ScheduleEvent(EVENT_SLAM, urand(6000, 9000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* _instance;
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_borka_the_bruteAI(creature);
        }
};

/*######
## boss_nitrogg_thundertower
######*/

class boss_nitrogg_thundertower : public CreatureScript
{
    public:
        boss_nitrogg_thundertower() : CreatureScript("boss_nitrogg_thundertower") { }

        struct boss_nitrogg_thundertowerAI : public ScriptedAI
        {
            boss_nitrogg_thundertowerAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset() override
            {
                _events.Reset();
                _onCannon = false;
            }

            void EnterCombat(Unit* /*who*/) override
            {
                Talk(SAY_AGGRO);

                if (_instance)
                    _instance->SetBossState(DATA_NITROGG_THUNDERTOWER, IN_PROGRESS);

                _events.ScheduleEvent(EVENT_SHREDDING_SWIPE, urand(6000, 9000));
                _events.ScheduleEvent(EVENT_THUNDER_SLAM, urand(12000, 16000));
            }

            // At 60% he gives up on melee and climbs into his cannon, which is
            // what the group then has to bring down.
            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/) override
            {
                if (_onCannon || !HealthBelowPct(NITROGG_CANNON_HEALTH_PCT))
                    return;

                _onCannon = true;
                Talk(SAY_SPECIAL);

                _events.Reset();
                me->AttackStop();
                me->SetReactState(REACT_PASSIVE);

                me->SummonCreature(NPC_ASSAULT_CANNON, me->GetPositionX(), me->GetPositionY(),
                    me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 120000);
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/) override
            {
                Talk(SAY_DEATH);

                if (_instance)
                    _instance->SetBossState(DATA_NITROGG_THUNDERTOWER, DONE);
            }

            void EnterEvadeMode() override
            {
                if (_instance)
                    _instance->SetBossState(DATA_NITROGG_THUNDERTOWER, FAIL);

                ScriptedAI::EnterEvadeMode();
            }

            void UpdateAI(uint32 diff) override
            {
                if (_onCannon)
                    return;

                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHREDDING_SWIPE:
                            DoCastVictim(SPELL_SHREDDING_SWIPE);
                            _events.ScheduleEvent(EVENT_SHREDDING_SWIPE, urand(6000, 9000));
                            break;
                        case EVENT_THUNDER_SLAM:
                            DoCastAOE(SPELL_THUNDER_SLAM);
                            _events.ScheduleEvent(EVENT_THUNDER_SLAM, urand(12000, 16000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* _instance;
            EventMap _events;
            bool _onCannon;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_nitrogg_thundertowerAI(creature);
        }
};

/*######
## boss_skylord_tovra
######*/

class boss_skylord_tovra : public CreatureScript
{
    public:
        boss_skylord_tovra() : CreatureScript("boss_skylord_tovra") { }

        struct boss_skylord_tovraAI : public ScriptedAI
        {
            boss_skylord_tovraAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset() override
            {
                _events.Reset();
            }

            void EnterCombat(Unit* /*who*/) override
            {
                Talk(SAY_AGGRO);

                if (_instance)
                    _instance->SetBossState(DATA_SKYLORD_TOVRA, IN_PROGRESS);

                _events.ScheduleEvent(EVENT_FREEZING_SNARE, urand(8000, 12000));
                _events.ScheduleEvent(EVENT_SPINNING_SPEAR, urand(10000, 14000));
                _events.ScheduleEvent(EVENT_DIFFUSED_ENERGY, urand(15000, 20000));
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/) override
            {
                Talk(SAY_DEATH);

                if (_instance)
                    _instance->SetBossState(DATA_SKYLORD_TOVRA, DONE);
            }

            void EnterEvadeMode() override
            {
                if (_instance)
                    _instance->SetBossState(DATA_SKYLORD_TOVRA, FAIL);

                ScriptedAI::EnterEvadeMode();
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FREEZING_SNARE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_FREEZING_SNARE);
                            _events.ScheduleEvent(EVENT_FREEZING_SNARE, urand(12000, 16000));
                            break;
                        case EVENT_SPINNING_SPEAR:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_SPINNING_SPEAR);
                            _events.ScheduleEvent(EVENT_SPINNING_SPEAR, urand(10000, 14000));
                            break;
                        case EVENT_DIFFUSED_ENERGY:
                            // The pools accumulate, so the arena keeps
                            // shrinking as the fight drags on.
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_DIFFUSED_ENERGY);
                            _events.ScheduleEvent(EVENT_DIFFUSED_ENERGY, urand(15000, 20000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* _instance;
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_skylord_tovraAI(creature);
        }
};

void AddSC_grimrail_depot_bosses()
{
    new boss_railmaster_rocketspark();
    new boss_borka_the_brute();
    new boss_nitrogg_thundertower();
    new boss_skylord_tovra();
}
