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
SDName: Blackrock_Foundry_Black_Forge
SD%Complete: 75
SDComment: Hans'gar and Franzok, Flamebender Ka'graz and Kromog. Spell ids come
           from the Warlords of Draenor encounter journal on Wowhead.
SDCategory: Blackrock Foundry
EndScriptData */

#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackrock_foundry.h"

enum ForgeTexts
{
    SAY_AGGRO       = 0,
    SAY_SLAY        = 1,
    SAY_DEATH       = 2,
    SAY_SPECIAL     = 3
};

/*######
## Hans'gar and Franzok -- two bodies, one health pool
######*/

enum BrothersSpells
{
    SPELL_BOUND_BY_BLOOD        = 161029,
    SPELL_DISRUPTING_ROAR       = 160838,
    SPELL_AFTERSHOCK            = 157853,
    SPELL_CRIPPLING_SUPLEX      = 156938,
    SPELL_SHATTERED_VERTEBRAE   = 157139,
    SPELL_SEARING_PLATES        = 161570,
    SPELL_SCORCHING_BURNS       = 155818,
    SPELL_BODY_SLAM             = 159646,
    SPELL_SKULLCRACKER          = 153470,
    SPELL_PUMPED_UP             = 155665
};

enum BrothersEvents
{
    EVENT_DISRUPTING_ROAR       = 1,
    EVENT_SUPLEX                = 2,
    EVENT_BODY_SLAM             = 3,
    EVENT_SKULLCRACKER          = 4,
    EVENT_SEARING_PLATES        = 5
};

// Both brothers run the same AI; Bound by Blood keeps their health together, so
// there is no point in giving them separate scripts.
class boss_hansgar_and_franzok : public CreatureScript
{
    public:
        boss_hansgar_and_franzok() : CreatureScript("boss_hansgar_and_franzok") { }

        struct boss_hansgar_and_franzokAI : public BossAI
        {
            boss_hansgar_and_franzokAI(Creature* creature) : BossAI(creature, DataHansgarAndFranzok) { }

            void Reset()
            {
                _Reset();
                _pumped = false;
                me->RemoveAurasDueToSpell(SPELL_PUMPED_UP);
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                DoCast(me, SPELL_BOUND_BY_BLOOD, true);

                // Pulling one pulls the other -- they share a health pool, so
                // fighting them apart makes no sense.
                if (instance)
                {
                    uint32 const otherGuidType = me->GetEntry() == NpcHansgar ? DataFranzokGuid : DataHansgarGuid;
                    if (Creature* brother = ObjectAccessor::GetCreature(*me, instance->GetGuidData(otherGuidType)))
                        if (brother->isAlive() && !brother->isInCombat())
                            brother->AI()->AttackStart(who);
                }

                events.ScheduleEvent(EVENT_DISRUPTING_ROAR, urand(10000, 14000));
                events.ScheduleEvent(EVENT_SUPLEX, urand(15000, 20000));
                events.ScheduleEvent(EVENT_BODY_SLAM, urand(8000, 12000));
                events.ScheduleEvent(EVENT_SKULLCRACKER, urand(20000, 25000));
                events.ScheduleEvent(EVENT_SEARING_PLATES, urand(30000, 40000));
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

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_pumped || !HealthBelowPct(30))
                    return;

                _pumped = true;
                Talk(SAY_SPECIAL);
                DoCast(me, SPELL_PUMPED_UP, true);
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
                        case EVENT_DISRUPTING_ROAR:
                            DoCastAOE(SPELL_DISRUPTING_ROAR);
                            events.ScheduleEvent(EVENT_DISRUPTING_ROAR, urand(10000, 14000));
                            break;
                        case EVENT_SUPLEX:
                            // Lands on the tank and leaves Shattered Vertebrae
                            // behind, which is what forces the swap.
                            DoCastVictim(SPELL_CRIPPLING_SUPLEX);
                            DoCastVictim(SPELL_SHATTERED_VERTEBRAE, true);
                            events.ScheduleEvent(EVENT_SUPLEX, urand(15000, 20000));
                            break;
                        case EVENT_BODY_SLAM:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_BODY_SLAM);
                            events.ScheduleEvent(EVENT_BODY_SLAM, urand(8000, 12000));
                            break;
                        case EVENT_SKULLCRACKER:
                            DoCastAOE(SPELL_SKULLCRACKER);
                            events.ScheduleEvent(EVENT_SKULLCRACKER, urand(20000, 25000));
                            break;
                        case EVENT_SEARING_PLATES:
                            // The floor itself turns hostile for a while.
                            DoCast(me, SPELL_SEARING_PLATES, true);
                            DoCastAOE(SPELL_SCORCHING_BURNS);
                            events.ScheduleEvent(EVENT_SEARING_PLATES, urand(30000, 40000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool _pumped;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_hansgar_and_franzokAI(creature);
        }
};

/*######
## Flamebender Ka'graz
######*/

enum KagrazSpells
{
    SPELL_LAVA_SLASH            = 155318,
    SPELL_SUMMON_CINDER_WOLVES  = 155776,
    SPELL_SUMMON_ARMAMENTS      = 156724,
    SPELL_MOLTEN_TORRENT        = 154938,
    SPELL_BLAZING_RADIANCE      = 155277,
    SPELL_FIRESTORM             = 155493,
    SPELL_UNQUENCHABLE_FLAME    = 156713
};

enum KagrazEvents
{
    EVENT_LAVA_SLASH            = 1,
    EVENT_CINDER_WOLVES         = 2,
    EVENT_ARMAMENTS             = 3,
    EVENT_MOLTEN_TORRENT        = 4,
    EVENT_FIRESTORM             = 5
};

class boss_flamebender_kagraz : public CreatureScript
{
    public:
        boss_flamebender_kagraz() : CreatureScript("boss_flamebender_kagraz") { }

        struct boss_flamebender_kagrazAI : public BossAI
        {
            boss_flamebender_kagrazAI(Creature* creature) : BossAI(creature, DataFlamebenderKagraz) { }

            void Reset()
            {
                _Reset();
                _enraged = false;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_LAVA_SLASH, urand(8000, 12000));
                events.ScheduleEvent(EVENT_CINDER_WOLVES, urand(25000, 30000));
                events.ScheduleEvent(EVENT_ARMAMENTS, urand(40000, 50000));
                events.ScheduleEvent(EVENT_MOLTEN_TORRENT, urand(15000, 20000));
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
                if (_enraged || !HealthBelowPct(30))
                    return;

                _enraged = true;
                Talk(SAY_SPECIAL);
                DoCast(me, SPELL_UNQUENCHABLE_FLAME, true);
                events.ScheduleEvent(EVENT_FIRESTORM, 5000);
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
                        case EVENT_LAVA_SLASH:
                            DoCastVictim(SPELL_LAVA_SLASH);
                            events.ScheduleEvent(EVENT_LAVA_SLASH, urand(8000, 12000));
                            break;
                        case EVENT_CINDER_WOLVES:
                            // The wolves are linked to each other; splitting
                            // them up is the mechanic.
                            DoCast(me, SPELL_SUMMON_CINDER_WOLVES, true);
                            events.ScheduleEvent(EVENT_CINDER_WOLVES, urand(45000, 55000));
                            break;
                        case EVENT_ARMAMENTS:
                            DoCast(me, SPELL_SUMMON_ARMAMENTS, true);
                            events.ScheduleEvent(EVENT_ARMAMENTS, urand(45000, 55000));
                            break;
                        case EVENT_MOLTEN_TORRENT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            {
                                DoCast(target, SPELL_MOLTEN_TORRENT);
                                DoCast(target, SPELL_BLAZING_RADIANCE, true);
                            }
                            events.ScheduleEvent(EVENT_MOLTEN_TORRENT, urand(15000, 20000));
                            break;
                        case EVENT_FIRESTORM:
                            DoCastAOE(SPELL_FIRESTORM);
                            events.ScheduleEvent(EVENT_FIRESTORM, urand(20000, 25000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool _enraged;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_flamebender_kagrazAI(creature);
        }
};

/*######
## Kromog -- never moves; the room comes to the raid instead
######*/

enum KromogSpells
{
    SPELL_WARPED_ARMOR          = 156766,
    SPELL_FISTS_OF_STONE        = 162349,
    SPELL_STONE_BREATH          = 156844,
    SPELL_SLAM                  = 156704,
    SPELL_RIPPLING_SMASH        = 157659,
    SPELL_RUNE_OF_GRASPING_EARTH = 157060,
    SPELL_THUNDERING_BLOWS      = 157054,
    SPELL_SHATTERED_EARTH       = 161893,
    SPELL_FRENZY                = 156861
};

enum KromogEvents
{
    EVENT_SLAM                  = 1,
    EVENT_STONE_BREATH          = 2,
    EVENT_RIPPLING_SMASH        = 3,
    EVENT_GRASPING_EARTH        = 4,
    EVENT_THUNDERING_BLOWS      = 5
};

class boss_kromog : public CreatureScript
{
    public:
        boss_kromog() : CreatureScript("boss_kromog") { }

        struct boss_kromogAI : public BossAI
        {
            boss_kromogAI(Creature* creature) : BossAI(creature, DataKromog) { }

            void Reset()
            {
                _Reset();
                _frenzied = false;
                me->RemoveAurasDueToSpell(SPELL_FRENZY);
                SetCombatMovement(false);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                DoCast(me, SPELL_WARPED_ARMOR, true);

                events.ScheduleEvent(EVENT_SLAM, urand(8000, 11000));
                events.ScheduleEvent(EVENT_STONE_BREATH, urand(15000, 20000));
                events.ScheduleEvent(EVENT_RIPPLING_SMASH, urand(12000, 16000));
                events.ScheduleEvent(EVENT_GRASPING_EARTH, urand(30000, 40000));
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

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_frenzied || !HealthBelowPct(20))
                    return;

                _frenzied = true;
                Talk(SAY_SPECIAL);
                DoCast(me, SPELL_FRENZY, true);
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
                        case EVENT_SLAM:
                            DoCastVictim(SPELL_SLAM);
                            events.ScheduleEvent(EVENT_SLAM, urand(8000, 11000));
                            break;
                        case EVENT_STONE_BREATH:
                            DoCastVictim(SPELL_STONE_BREATH);
                            events.ScheduleEvent(EVENT_STONE_BREATH, urand(15000, 20000));
                            break;
                        case EVENT_RIPPLING_SMASH:
                            DoCastAOE(SPELL_RIPPLING_SMASH);
                            events.ScheduleEvent(EVENT_RIPPLING_SMASH, urand(12000, 16000));
                            break;
                        case EVENT_GRASPING_EARTH:
                            // Hands come out of the floor and hold the raid
                            // still for what follows.
                            DoCastAOE(SPELL_RUNE_OF_GRASPING_EARTH);
                            events.ScheduleEvent(EVENT_THUNDERING_BLOWS, 8000);
                            events.ScheduleEvent(EVENT_GRASPING_EARTH, urand(45000, 55000));
                            break;
                        case EVENT_THUNDERING_BLOWS:
                            DoCastAOE(SPELL_THUNDERING_BLOWS);
                            DoCastAOE(SPELL_SHATTERED_EARTH);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool _frenzied;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_kromogAI(creature);
        }
};

void AddSC_brf_black_forge()
{
    new boss_hansgar_and_franzok();
    new boss_flamebender_kagraz();
    new boss_kromog();
}
