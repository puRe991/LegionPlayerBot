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
SDName: Hellfire_Citadel_Lower
SD%Complete: 70
SDComment: Hellfire Assault, Iron Reaver, Kormrok, Kilrogg Deadeye, the
           Hellfire High Council and Gorefiend. Spell ids come from the
           Warlords of Draenor encounter journal on Wowhead.
SDCategory: Hellfire Citadel
EndScriptData */

#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "hellfire_citadel.h"

enum HFCTexts
{
    SAY_AGGRO       = 0,
    SAY_SLAY        = 1,
    SAY_DEATH       = 2,
    SAY_SPECIAL     = 3
};

/*######
## Hellfire Assault -- Mar'tak directs while the siege engines roll in
######*/

enum AssaultSpells
{
    SPELL_HOWLING_AXE           = 184379,
    SPELL_SHOCKWAVE             = 184393,
    SPELL_INSPIRING_PRESENCE    = 185090,
    SPELL_CALL_TO_ARMS          = 185025
};

enum AssaultEvents
{
    EVENT_HOWLING_AXE           = 1,
    EVENT_SHOCKWAVE             = 2,
    EVENT_REINFORCEMENTS        = 3
};

class boss_hellfire_assault : public CreatureScript
{
    public:
        boss_hellfire_assault() : CreatureScript("boss_hellfire_assault") { }

        struct boss_hellfire_assaultAI : public BossAI
        {
            boss_hellfire_assaultAI(Creature* creature) : BossAI(creature, DATA_HELLFIRE_ASSAULT) { }

            void Reset()
            {
                _Reset();
                me->RemoveAurasDueToSpell(SPELL_INSPIRING_PRESENCE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                // She never fights alone; her presence is what keeps the
                // reinforcements coming.
                DoCast(me, SPELL_INSPIRING_PRESENCE, true);

                events.ScheduleEvent(EVENT_HOWLING_AXE, urand(10000, 14000));
                events.ScheduleEvent(EVENT_SHOCKWAVE, urand(15000, 20000));
                events.ScheduleEvent(EVENT_REINFORCEMENTS, 30000);
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
                        case EVENT_HOWLING_AXE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_HOWLING_AXE);
                            events.ScheduleEvent(EVENT_HOWLING_AXE, urand(10000, 14000));
                            break;
                        case EVENT_SHOCKWAVE:
                            DoCastAOE(SPELL_SHOCKWAVE);
                            events.ScheduleEvent(EVENT_SHOCKWAVE, urand(15000, 20000));
                            break;
                        case EVENT_REINFORCEMENTS:
                            Talk(SAY_SPECIAL);
                            DoCast(me, SPELL_CALL_TO_ARMS, true);
                            events.ScheduleEvent(EVENT_REINFORCEMENTS, urand(40000, 50000));
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
            return new boss_hellfire_assaultAI(creature);
        }
};

/*######
## Iron Reaver -- alternates between the ground and the air
######*/

enum ReaverSpells
{
    SPELL_IMMOLATION            = 182074,
    SPELL_BLITZ                 = 179897,
    SPELL_POUNDING              = 182022,
    SPELL_ARTILLERY             = 182108,
    SPELL_UNSTABLE_ORB          = 182001,
    SPELL_BARRAGE               = 185284,
    SPELL_FIREBOMB              = 182005,
    SPELL_FALLING_SLAM          = 182362
};

enum ReaverEvents
{
    EVENT_BLITZ                 = 1,
    EVENT_POUNDING              = 2,
    EVENT_ARTILLERY             = 3,
    EVENT_UNSTABLE_ORB          = 4,
    EVENT_TAKE_OFF              = 5,
    EVENT_LAND                  = 6,
    EVENT_BARRAGE               = 7
};

enum ReaverMisc
{
    REAVER_FLIGHT_INTERVAL      = 45000,
    REAVER_FLIGHT_DURATION      = 20000
};

class boss_iron_reaver : public CreatureScript
{
    public:
        boss_iron_reaver() : CreatureScript("boss_iron_reaver") { }

        struct boss_iron_reaverAI : public BossAI
        {
            boss_iron_reaverAI(Creature* creature) : BossAI(creature, DATA_IRON_REAVER) { }

            void Reset()
            {
                _Reset();
                _airborne = false;
                me->RemoveAurasDueToSpell(SPELL_IMMOLATION);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                DoCast(me, SPELL_IMMOLATION, true);
                ScheduleGroundEvents();
                events.ScheduleEvent(EVENT_TAKE_OFF, REAVER_FLIGHT_INTERVAL);
            }

            void ScheduleGroundEvents()
            {
                events.ScheduleEvent(EVENT_BLITZ, urand(15000, 20000));
                events.ScheduleEvent(EVENT_POUNDING, urand(20000, 25000));
                events.ScheduleEvent(EVENT_ARTILLERY, urand(10000, 14000));
                events.ScheduleEvent(EVENT_UNSTABLE_ORB, urand(12000, 16000));
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
                        case EVENT_BLITZ:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_BLITZ);
                            events.ScheduleEvent(EVENT_BLITZ, urand(15000, 20000));
                            break;
                        case EVENT_POUNDING:
                            DoCastAOE(SPELL_POUNDING);
                            events.ScheduleEvent(EVENT_POUNDING, urand(20000, 25000));
                            break;
                        case EVENT_ARTILLERY:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_ARTILLERY);
                            events.ScheduleEvent(EVENT_ARTILLERY, urand(10000, 14000));
                            break;
                        case EVENT_UNSTABLE_ORB:
                            DoCastAOE(SPELL_UNSTABLE_ORB);
                            events.ScheduleEvent(EVENT_UNSTABLE_ORB, urand(12000, 16000));
                            break;
                        case EVENT_TAKE_OFF:
                            // In the air she stops being a tank problem and
                            // becomes a raid problem.
                            _airborne = true;
                            Talk(SAY_SPECIAL);
                            events.CancelEvent(EVENT_BLITZ);
                            events.CancelEvent(EVENT_POUNDING);
                            events.ScheduleEvent(EVENT_BARRAGE, 3000);
                            events.ScheduleEvent(EVENT_LAND, REAVER_FLIGHT_DURATION);
                            break;
                        case EVENT_BARRAGE:
                            DoCastAOE(SPELL_BARRAGE);
                            DoCastAOE(SPELL_FIREBOMB);
                            if (_airborne)
                                events.ScheduleEvent(EVENT_BARRAGE, 5000);
                            break;
                        case EVENT_LAND:
                            _airborne = false;
                            DoCastAOE(SPELL_FALLING_SLAM);
                            events.CancelEvent(EVENT_BARRAGE);
                            ScheduleGroundEvents();
                            events.ScheduleEvent(EVENT_TAKE_OFF, REAVER_FLIGHT_INTERVAL);
                            break;
                        default:
                            break;
                    }
                }

                if (!_airborne)
                    DoMeleeAttackIfReady();
            }

        private:
            bool _airborne;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_iron_reaverAI(creature);
        }
};

/*######
## Kormrok -- hops between pools and takes on whatever he lands in
######*/

enum KormrokSpells
{
    SPELL_EXPLOSIVE_BURST       = 181306,
    SPELL_GRASPING_HANDS        = 181299,
    SPELL_FOUL_ENERGY           = 180117
};

enum KormrokEvents
{
    EVENT_EXPLOSIVE_BURST       = 1,
    EVENT_GRASPING_HANDS        = 2,
    EVENT_LEAP                  = 3
};

class boss_kormrok : public CreatureScript
{
    public:
        boss_kormrok() : CreatureScript("boss_kormrok") { }

        struct boss_kormrokAI : public BossAI
        {
            boss_kormrokAI(Creature* creature) : BossAI(creature, DATA_KORMROK) { }

            void Reset()
            {
                _Reset();
                me->RemoveAurasDueToSpell(SPELL_FOUL_ENERGY);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_EXPLOSIVE_BURST, urand(12000, 16000));
                events.ScheduleEvent(EVENT_GRASPING_HANDS, urand(15000, 20000));
                events.ScheduleEvent(EVENT_LEAP, 40000);
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
                        case EVENT_EXPLOSIVE_BURST:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_EXPLOSIVE_BURST);
                            events.ScheduleEvent(EVENT_EXPLOSIVE_BURST, urand(12000, 16000));
                            break;
                        case EVENT_GRASPING_HANDS:
                            DoCastAOE(SPELL_GRASPING_HANDS);
                            events.ScheduleEvent(EVENT_GRASPING_HANDS, urand(15000, 20000));
                            break;
                        case EVENT_LEAP:
                            // He jumps into one of the three pools and comes
                            // back charged with it.
                            Talk(SAY_SPECIAL);
                            DoCast(me, SPELL_FOUL_ENERGY, true);
                            events.ScheduleEvent(EVENT_LEAP, urand(40000, 50000));
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
            return new boss_kormrokAI(creature);
        }
};

/*######
## Kilrogg Deadeye -- the raid keeps being pulled into his vision of death
######*/

enum KilroggSpells
{
    SPELL_SHRED_ARMOR           = 180199,
    SPELL_HEART_SEEKER          = 180372,
    SPELL_BLOOD_SPLATTER        = 188852,
    SPELL_DEATH_THROES          = 180224,
    SPELL_VISION_OF_DEATH       = 182428,
    SPELL_FEL_CORRUPTION        = 182159,
    SPELL_DEATHS_DOOR           = 184551
};

enum KilroggEvents
{
    EVENT_SHRED_ARMOR           = 1,
    EVENT_HEART_SEEKER          = 2,
    EVENT_BLOOD_SPLATTER        = 3,
    EVENT_VISION_OF_DEATH       = 4,
    EVENT_DEATH_THROES          = 5
};

class boss_kilrogg_deadeye : public CreatureScript
{
    public:
        boss_kilrogg_deadeye() : CreatureScript("boss_kilrogg_deadeye") { }

        struct boss_kilrogg_deadeyeAI : public BossAI
        {
            boss_kilrogg_deadeyeAI(Creature* creature) : BossAI(creature, DATA_KILROGG) { }

            void Reset()
            {
                _Reset();
                _corrupted = false;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_SHRED_ARMOR, urand(8000, 11000));
                events.ScheduleEvent(EVENT_HEART_SEEKER, urand(15000, 20000));
                events.ScheduleEvent(EVENT_BLOOD_SPLATTER, urand(12000, 16000));
                events.ScheduleEvent(EVENT_VISION_OF_DEATH, 45000);
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
                if (_corrupted || !HealthBelowPct(30))
                    return;

                _corrupted = true;
                Talk(SAY_SPECIAL);
                DoCast(me, SPELL_FEL_CORRUPTION, true);
                events.ScheduleEvent(EVENT_DEATH_THROES, 5000);
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
                        case EVENT_SHRED_ARMOR:
                            DoCastVictim(SPELL_SHRED_ARMOR);
                            events.ScheduleEvent(EVENT_SHRED_ARMOR, urand(8000, 11000));
                            break;
                        case EVENT_HEART_SEEKER:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_HEART_SEEKER);
                            events.ScheduleEvent(EVENT_HEART_SEEKER, urand(15000, 20000));
                            break;
                        case EVENT_BLOOD_SPLATTER:
                            DoCastAOE(SPELL_BLOOD_SPLATTER);
                            events.ScheduleEvent(EVENT_BLOOD_SPLATTER, urand(12000, 16000));
                            break;
                        case EVENT_VISION_OF_DEATH:
                            // Part of the raid is pulled out of the fight and
                            // has to work its way back.
                            Talk(SAY_SPECIAL);
                            DoCastAOE(SPELL_VISION_OF_DEATH);
                            DoCastAOE(SPELL_DEATHS_DOOR);
                            events.ScheduleEvent(EVENT_VISION_OF_DEATH, urand(60000, 70000));
                            break;
                        case EVENT_DEATH_THROES:
                            DoCastAOE(SPELL_DEATH_THROES);
                            events.ScheduleEvent(EVENT_DEATH_THROES, urand(15000, 20000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool _corrupted;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_kilrogg_deadeyeAI(creature);
        }
};

/*######
## Hellfire High Council -- three bosses, one AI branching on entry
######*/

enum CouncilSpells
{
    // Blademaster Jubei'thos
    SPELL_FEL_BLADE             = 183226,
    SPELL_FELSTORM              = 183701,
    SPELL_MIRROR_IMAGES         = 183885,

    // Dia Darkwhisper
    SPELL_VOID_BOLT             = 184675,
    SPELL_MARK_OF_THE_NECROMANCER = 184449,
    SPELL_REAP                  = 184652,

    // Gurtogg Bloodboil
    SPELL_BLOODBOIL             = 184355,
    SPELL_FEL_RAGE              = 184358,
    SPELL_DEMOLISHING_LEAP      = 184366
};

enum CouncilEvents
{
    EVENT_COUNCIL_PRIMARY       = 1,
    EVENT_COUNCIL_SECONDARY     = 2
};

class boss_hellfire_high_council : public CreatureScript
{
    public:
        boss_hellfire_high_council() : CreatureScript("boss_hellfire_high_council") { }

        struct boss_hellfire_high_councilAI : public BossAI
        {
            boss_hellfire_high_councilAI(Creature* creature) : BossAI(creature, DATA_COUNCIL) { }

            void Reset()
            {
                _Reset();
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);
                PullOthers(who);

                events.ScheduleEvent(EVENT_COUNCIL_PRIMARY, urand(6000, 9000));
                events.ScheduleEvent(EVENT_COUNCIL_SECONDARY, urand(18000, 24000));
            }

            void PullOthers(Unit* who)
            {
                if (!instance)
                    return;

                uint32 const members[3] =
                {
                    DATA_DIA_DARKWHISPER_GUID,
                    DATA_GURTOGG_BLOODBOIL_GUID,
                    DATA_BLADEMASTER_JUBEITHOS_GUID
                };

                for (uint8 i = 0; i < 3; ++i)
                    if (Creature* other = ObjectAccessor::GetCreature(*me, instance->GetGuidData(members[i])))
                        if (other != me && other->isAlive() && !other->isInCombat())
                            other->AI()->AttackStart(who);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            // The council falls together, not one at a time.
            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);

                if (!instance)
                    return;

                uint32 const members[3] =
                {
                    DATA_DIA_DARKWHISPER_GUID,
                    DATA_GURTOGG_BLOODBOIL_GUID,
                    DATA_BLADEMASTER_JUBEITHOS_GUID
                };

                for (uint8 i = 0; i < 3; ++i)
                    if (Creature* other = ObjectAccessor::GetCreature(*me, instance->GetGuidData(members[i])))
                        if (other != me && other->isAlive())
                            return;

                _JustDied();
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
                        case EVENT_COUNCIL_PRIMARY:
                            CastPrimary();
                            events.ScheduleEvent(EVENT_COUNCIL_PRIMARY, urand(6000, 9000));
                            break;
                        case EVENT_COUNCIL_SECONDARY:
                            CastSecondary();
                            events.ScheduleEvent(EVENT_COUNCIL_SECONDARY, urand(18000, 24000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void CastPrimary()
            {
                switch (me->GetEntry())
                {
                    case NPC_BLADEMASTER_JUBEITHOS:
                        DoCastVictim(SPELL_FEL_BLADE);
                        break;
                    case NPC_DIA_DARKWHISPER:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            DoCast(target, SPELL_VOID_BOLT);
                        break;
                    case NPC_GURTOGG_BLOODBOIL:
                        DoCastAOE(SPELL_BLOODBOIL);
                        break;
                    default:
                        break;
                }
            }

            void CastSecondary()
            {
                switch (me->GetEntry())
                {
                    case NPC_BLADEMASTER_JUBEITHOS:
                        DoCastAOE(SPELL_FELSTORM);
                        DoCast(me, SPELL_MIRROR_IMAGES, true);
                        break;
                    case NPC_DIA_DARKWHISPER:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                        {
                            DoCast(target, SPELL_MARK_OF_THE_NECROMANCER);
                            DoCast(target, SPELL_REAP, true);
                        }
                        break;
                    case NPC_GURTOGG_BLOODBOIL:
                        DoCast(me, SPELL_FEL_RAGE, true);
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            DoCast(target, SPELL_DEMOLISHING_LEAP);
                        break;
                    default:
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_hellfire_high_councilAI(creature);
        }
};

/*######
## Gorefiend -- kills people and then makes them fight from inside him
######*/

enum GorefiendSpells
{
    SPELL_SHADOW_OF_DEATH       = 179864,
    SPELL_TOUCH_OF_DOOM         = 179978,
    SPELL_DOOM_WELL             = 179995,
    SPELL_SHARED_FATE           = 179909,
    SPELL_CRUSHING_DARKNESS     = 180017,
    SPELL_FEAST_OF_SOULS        = 181973,
    SPELL_GOREFIENDS_CORRUPTION = 179867
};

enum GorefiendEvents
{
    EVENT_SHADOW_OF_DEATH       = 1,
    EVENT_TOUCH_OF_DOOM         = 2,
    EVENT_CRUSHING_DARKNESS     = 3,
    EVENT_SHARED_FATE           = 4,
    EVENT_FEAST_OF_SOULS        = 5
};

class boss_gorefiend : public CreatureScript
{
    public:
        boss_gorefiend() : CreatureScript("boss_gorefiend") { }

        struct boss_gorefiendAI : public BossAI
        {
            boss_gorefiendAI(Creature* creature) : BossAI(creature, DATA_GOREFIEND) { }

            void Reset()
            {
                _Reset();
                _feasting = false;
                me->RemoveAurasDueToSpell(SPELL_GOREFIENDS_CORRUPTION);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                DoCast(me, SPELL_GOREFIENDS_CORRUPTION, true);

                events.ScheduleEvent(EVENT_SHADOW_OF_DEATH, urand(20000, 25000));
                events.ScheduleEvent(EVENT_TOUCH_OF_DOOM, urand(12000, 16000));
                events.ScheduleEvent(EVENT_CRUSHING_DARKNESS, urand(25000, 30000));
                events.ScheduleEvent(EVENT_SHARED_FATE, urand(30000, 35000));
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
                if (_feasting || !HealthBelowPct(30))
                    return;

                _feasting = true;
                Talk(SAY_SPECIAL);
                events.ScheduleEvent(EVENT_FEAST_OF_SOULS, 3000);
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
                        case EVENT_SHADOW_OF_DEATH:
                            // The marked player dies and keeps fighting from
                            // inside him; that is the encounter.
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_SHADOW_OF_DEATH);
                            events.ScheduleEvent(EVENT_SHADOW_OF_DEATH, urand(30000, 40000));
                            break;
                        case EVENT_TOUCH_OF_DOOM:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_TOUCH_OF_DOOM);
                            DoCastAOE(SPELL_DOOM_WELL);
                            events.ScheduleEvent(EVENT_TOUCH_OF_DOOM, urand(12000, 16000));
                            break;
                        case EVENT_CRUSHING_DARKNESS:
                            DoCastVictim(SPELL_CRUSHING_DARKNESS);
                            events.ScheduleEvent(EVENT_CRUSHING_DARKNESS, urand(25000, 30000));
                            break;
                        case EVENT_SHARED_FATE:
                            DoCastAOE(SPELL_SHARED_FATE);
                            events.ScheduleEvent(EVENT_SHARED_FATE, urand(30000, 35000));
                            break;
                        case EVENT_FEAST_OF_SOULS:
                            DoCastAOE(SPELL_FEAST_OF_SOULS);
                            events.ScheduleEvent(EVENT_FEAST_OF_SOULS, urand(15000, 20000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool _feasting;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_gorefiendAI(creature);
        }
};

void AddSC_hfc_lower()
{
    new boss_hellfire_assault();
    new boss_iron_reaver();
    new boss_kormrok();
    new boss_kilrogg_deadeye();
    new boss_hellfire_high_council();
    new boss_gorefiend();
}
