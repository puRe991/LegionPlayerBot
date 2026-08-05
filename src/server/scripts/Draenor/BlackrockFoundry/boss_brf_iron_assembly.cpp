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
SDName: Blackrock_Foundry_Iron_Assembly
SD%Complete: 70
SDComment: Beastlord Darmac, Operator Thogar, the Iron Maidens and Blackhand.
           Spell ids come from the Warlords of Draenor encounter journal on
           Wowhead. Thogar's trains and Blackhand's two floor collapses need
           transport and terrain data that cannot be checked here, so those are
           driven by timers and health rather than by the real machinery.
SDCategory: Blackrock Foundry
EndScriptData */

#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackrock_foundry.h"

enum AssemblyTexts
{
    SAY_AGGRO       = 0,
    SAY_SLAY        = 1,
    SAY_DEATH       = 2,
    SAY_SPECIAL     = 3
};

/*######
## Beastlord Darmac -- rides a different beast every 20%
######*/

enum DarmacSpells
{
    SPELL_PIN_DOWN              = 155365,
    SPELL_CALL_THE_PACK         = 154975,
    SPELL_RRND_AND_TEAR         = 155061,
    SPELL_SAVAGE_HOWL           = 155198,
    SPELL_INFERNO_BREATH        = 154989,
    SPELL_CONFLAGRATION         = 155399,
    SPELL_TANTRUM               = 155222,
    SPELL_STAMPEDE              = 155247,
    SPELL_CRUSH_ARMOR           = 155236,
    SPELL_CANNONBALL_BARRAGE    = 155284,
    SPELL_EPICENTER             = 159044
};

enum DarmacEvents
{
    EVENT_PIN_DOWN              = 1,
    EVENT_CALL_THE_PACK         = 2,
    EVENT_MOUNT_ABILITY         = 3,
    EVENT_TANTRUM               = 4
};

enum DarmacMisc
{
    DARMAC_FIRST_MOUNT_PCT      = 85,
    DARMAC_MOUNT_STEP_PCT       = 20
};

class boss_beastlord_darmac : public CreatureScript
{
    public:
        boss_beastlord_darmac() : CreatureScript("boss_beastlord_darmac") { }

        struct boss_beastlord_darmacAI : public BossAI
        {
            boss_beastlord_darmacAI(Creature* creature) : BossAI(creature, DataBeastlordDarmac) { }

            void Reset()
            {
                _Reset();
                _mountsTaken = 0;
                _nextMountPct = DARMAC_FIRST_MOUNT_PCT;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_PIN_DOWN, urand(10000, 14000));
                events.ScheduleEvent(EVENT_CALL_THE_PACK, urand(20000, 25000));
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

            // He takes a new beast at 85% and every 20% after, and keeps the
            // abilities of each one he has ridden.
            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_nextMountPct == 0 || !HealthBelowPct(_nextMountPct))
                    return;

                ++_mountsTaken;
                Talk(SAY_SPECIAL);

                if (_nextMountPct > DARMAC_MOUNT_STEP_PCT)
                    _nextMountPct -= DARMAC_MOUNT_STEP_PCT;
                else
                    _nextMountPct = 0;

                events.RescheduleEvent(EVENT_MOUNT_ABILITY, 3000);

                if (_mountsTaken >= 3)
                    events.RescheduleEvent(EVENT_TANTRUM, urand(15000, 20000));
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
                        case EVENT_PIN_DOWN:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_PIN_DOWN);
                            events.ScheduleEvent(EVENT_PIN_DOWN, urand(10000, 14000));
                            break;
                        case EVENT_CALL_THE_PACK:
                            DoCast(me, SPELL_CALL_THE_PACK, true);
                            events.ScheduleEvent(EVENT_CALL_THE_PACK, urand(25000, 30000));
                            break;
                        case EVENT_MOUNT_ABILITY:
                            // Which beast he is on decides what comes out.
                            switch (_mountsTaken)
                            {
                                case 1:
                                    DoCastVictim(SPELL_RRND_AND_TEAR);
                                    DoCast(me, SPELL_SAVAGE_HOWL, true);
                                    break;
                                case 2:
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                        DoCast(target, SPELL_INFERNO_BREATH);
                                    DoCastAOE(SPELL_CONFLAGRATION);
                                    break;
                                default:
                                    DoCastVictim(SPELL_CRUSH_ARMOR);
                                    DoCastAOE(SPELL_STAMPEDE);
                                    break;
                            }
                            events.ScheduleEvent(EVENT_MOUNT_ABILITY, urand(15000, 20000));
                            break;
                        case EVENT_TANTRUM:
                            DoCastAOE(SPELL_TANTRUM);
                            DoCastAOE(SPELL_EPICENTER);
                            events.ScheduleEvent(EVENT_TANTRUM, urand(20000, 25000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint8 _mountsTaken;
            uint32 _nextMountPct;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_beastlord_darmacAI(creature);
        }
};

/*######
## Operator Thogar -- the trains are the encounter
######*/

enum ThogarSpells
{
    SPELL_ENKINDLE              = 155921,
    SPELL_PROTOTYPE_PULSE_GRENADE = 165195,
    SPELL_DELAYED_SIEGE_BOMB    = 159481,
    SPELL_OBLITERATION          = 156406
};

enum ThogarEvents
{
    EVENT_ENKINDLE              = 1,
    EVENT_PULSE_GRENADE         = 2,
    EVENT_TRAIN                 = 3
};

class boss_operator_thogar : public CreatureScript
{
    public:
        boss_operator_thogar() : CreatureScript("boss_operator_thogar") { }

        struct boss_operator_thogarAI : public BossAI
        {
            boss_operator_thogarAI(Creature* creature) : BossAI(creature, DataOperatorThogar) { }

            void Reset()
            {
                _Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_ENKINDLE, urand(10000, 14000));
                events.ScheduleEvent(EVENT_PULSE_GRENADE, urand(15000, 20000));
                events.ScheduleEvent(EVENT_TRAIN, 30000);
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
                        case EVENT_ENKINDLE:
                            // Stacks on whoever holds him, which is the whole
                            // reason the fight wants two tanks.
                            DoCastVictim(SPELL_ENKINDLE);
                            events.ScheduleEvent(EVENT_ENKINDLE, urand(10000, 14000));
                            break;
                        case EVENT_PULSE_GRENADE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_PROTOTYPE_PULSE_GRENADE);
                            events.ScheduleEvent(EVENT_PULSE_GRENADE, urand(15000, 20000));
                            break;
                        case EVENT_TRAIN:
                            // The real encounter runs trains down four tracks
                            // on a schedule. Without the transport data that
                            // needs, the siege bombardment stands in for it.
                            Talk(SAY_SPECIAL);
                            DoCastAOE(SPELL_DELAYED_SIEGE_BOMB);
                            events.ScheduleEvent(EVENT_TRAIN, urand(35000, 45000));
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
            return new boss_operator_thogarAI(creature);
        }
};

/*######
## The Iron Maidens -- three bosses, one encounter, shared Iron Will
######*/

enum MaidenSpells
{
    SPELL_IRON_WILL             = 159337,

    SPELL_IRON_SHOT             = 156666,
    SPELL_RAPID_FIRE            = 156629,
    SPELL_PENETRATING_SHOT      = 164264,
    SPELL_DEPLOY_TURRET         = 159585,

    SPELL_BLADE_DASH            = 160796,
    SPELL_CONVULSIVE_SHADOWS    = 156112,
    SPELL_DARK_HUNT             = 158315,

    SPELL_BLOOD_RITUAL          = 158078,
    SPELL_BLOODSOAKED_HEARTSEEKER = 158008,
    SPELL_SANGUINE_STRIKES      = 156601
};

enum MaidenEvents
{
    EVENT_MAIDEN_PRIMARY        = 1,
    EVENT_MAIDEN_SECONDARY      = 2
};

// One AI for all three; which entry it is decides what it casts. Iron Will ties
// them together so killing them out of order does not end the fight early.
class boss_iron_maidens : public CreatureScript
{
    public:
        boss_iron_maidens() : CreatureScript("boss_iron_maidens") { }

        struct boss_iron_maidensAI : public BossAI
        {
            boss_iron_maidensAI(Creature* creature) : BossAI(creature, DataIronMaidens) { }

            void Reset()
            {
                _Reset();
                me->RemoveAurasDueToSpell(SPELL_IRON_WILL);
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                DoCast(me, SPELL_IRON_WILL, true);
                PullSisters(who);

                events.ScheduleEvent(EVENT_MAIDEN_PRIMARY, urand(6000, 9000));
                events.ScheduleEvent(EVENT_MAIDEN_SECONDARY, urand(15000, 20000));
            }

            void PullSisters(Unit* who)
            {
                if (!instance)
                    return;

                uint32 const sisters[3] = { DataAdmiralGaranGuid, DataMarakTheBloodedGuid, DataEnforcerSorkaGuid };
                for (uint8 i = 0; i < 3; ++i)
                    if (Creature* sister = ObjectAccessor::GetCreature(*me, instance->GetGuidData(sisters[i])))
                        if (sister != me && sister->isAlive() && !sister->isInCombat())
                            sister->AI()->AttackStart(who);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);

                // The encounter is only over once all three are down.
                if (!instance)
                    return;

                uint32 const sisters[3] = { DataAdmiralGaranGuid, DataMarakTheBloodedGuid, DataEnforcerSorkaGuid };
                for (uint8 i = 0; i < 3; ++i)
                    if (Creature* sister = ObjectAccessor::GetCreature(*me, instance->GetGuidData(sisters[i])))
                        if (sister != me && sister->isAlive())
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
                        case EVENT_MAIDEN_PRIMARY:
                            CastPrimary();
                            events.ScheduleEvent(EVENT_MAIDEN_PRIMARY, urand(6000, 9000));
                            break;
                        case EVENT_MAIDEN_SECONDARY:
                            CastSecondary();
                            events.ScheduleEvent(EVENT_MAIDEN_SECONDARY, urand(15000, 20000));
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
                    case NpcAdmiralGaran:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            DoCast(target, SPELL_IRON_SHOT);
                        break;
                    case NpcEnforcerSorka:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            DoCast(target, SPELL_BLADE_DASH);
                        break;
                    case NpcMarakTheBlooded:
                        DoCastVictim(SPELL_SANGUINE_STRIKES);
                        break;
                    default:
                        break;
                }
            }

            void CastSecondary()
            {
                switch (me->GetEntry())
                {
                    case NpcAdmiralGaran:
                        DoCastAOE(SPELL_RAPID_FIRE);
                        DoCast(me, SPELL_DEPLOY_TURRET, true);
                        break;
                    case NpcEnforcerSorka:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                        {
                            DoCast(target, SPELL_CONVULSIVE_SHADOWS);
                            DoCast(target, SPELL_DARK_HUNT, true);
                        }
                        break;
                    case NpcMarakTheBlooded:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                        {
                            DoCast(target, SPELL_BLOOD_RITUAL);
                            DoCast(target, SPELL_BLOODSOAKED_HEARTSEEKER, true);
                        }
                        break;
                    default:
                        break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_iron_maidensAI(creature);
        }
};

/*######
## Blackhand -- three floors, each one further down the tower
######*/

enum BlackhandSpells
{
    SPELL_MOLTEN_SLAG           = 156401,
    SPELL_DEMOLITION            = 156425,
    SPELL_MASSIVE_DEMOLITION    = 156479,
    SPELL_MARKED_FOR_DEATH      = 156096,
    SPELL_IMPALING_THROW        = 156107,
    SPELL_THROW_SLAG_BOMBS      = 156030,
    SPELL_SHATTERING_SMASH      = 155992,
    SPELL_SLAG_ERUPTION         = 156928,
    SPELL_MASSIVE_SHATTERING_SMASH = 158054
};

enum BlackhandEvents
{
    EVENT_MARKED_FOR_DEATH      = 1,
    EVENT_SHATTERING_SMASH      = 2,
    EVENT_SLAG_BOMBS            = 3,
    EVENT_DEMOLITION            = 4,
    EVENT_SLAG_ERUPTION         = 5
};

enum BlackhandMisc
{
    BLACKHAND_STAGE_TWO_PCT     = 70,
    BLACKHAND_STAGE_THREE_PCT   = 30
};

class boss_blackhand : public CreatureScript
{
    public:
        boss_blackhand() : CreatureScript("boss_blackhand") { }

        struct boss_blackhandAI : public BossAI
        {
            boss_blackhandAI(Creature* creature) : BossAI(creature, DataBlackhand) { }

            void Reset()
            {
                _Reset();
                _stage = 1;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_MARKED_FOR_DEATH, urand(15000, 20000));
                events.ScheduleEvent(EVENT_SHATTERING_SMASH, urand(20000, 25000));
                events.ScheduleEvent(EVENT_SLAG_BOMBS, urand(10000, 14000));
                events.ScheduleEvent(EVENT_DEMOLITION, urand(25000, 30000));
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

            // At 70% and 30% the floor gives way and the fight continues a
            // level down. The drop itself is terrain work the script cannot do
            // here, so only what he brings with him changes.
            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_stage == 1 && HealthBelowPct(BLACKHAND_STAGE_TWO_PCT))
                {
                    _stage = 2;
                    Talk(SAY_SPECIAL);
                    events.CancelEvent(EVENT_DEMOLITION);
                }
                else if (_stage == 2 && HealthBelowPct(BLACKHAND_STAGE_THREE_PCT))
                {
                    _stage = 3;
                    Talk(SAY_SPECIAL);
                    events.ScheduleEvent(EVENT_SLAG_ERUPTION, 5000);
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
                        case EVENT_MARKED_FOR_DEATH:
                            // Whoever is marked has to get behind cover before
                            // the throw lands.
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            {
                                DoCast(target, SPELL_MARKED_FOR_DEATH);
                                events.ScheduleEvent(EVENT_MARKED_FOR_DEATH + 100, 5000);
                                _markedGuid = target->GetGUID();
                            }
                            events.ScheduleEvent(EVENT_MARKED_FOR_DEATH, urand(20000, 25000));
                            break;
                        case EVENT_MARKED_FOR_DEATH + 100:
                            if (Unit* marked = ObjectAccessor::GetUnit(*me, _markedGuid))
                                DoCast(marked, SPELL_IMPALING_THROW);
                            break;
                        case EVENT_SHATTERING_SMASH:
                            DoCastVictim(_stage == 3 ? SPELL_MASSIVE_SHATTERING_SMASH : SPELL_SHATTERING_SMASH);
                            events.ScheduleEvent(EVENT_SHATTERING_SMASH, urand(20000, 25000));
                            break;
                        case EVENT_SLAG_BOMBS:
                            DoCastAOE(SPELL_THROW_SLAG_BOMBS);
                            events.ScheduleEvent(EVENT_SLAG_BOMBS, urand(10000, 14000));
                            break;
                        case EVENT_DEMOLITION:
                            DoCastAOE(_stage == 1 ? SPELL_DEMOLITION : SPELL_MASSIVE_DEMOLITION);
                            DoCastAOE(SPELL_MOLTEN_SLAG);
                            events.ScheduleEvent(EVENT_DEMOLITION, urand(25000, 30000));
                            break;
                        case EVENT_SLAG_ERUPTION:
                            DoCastAOE(SPELL_SLAG_ERUPTION);
                            events.ScheduleEvent(EVENT_SLAG_ERUPTION, urand(15000, 20000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            ObjectGuid _markedGuid;
            uint8 _stage;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_blackhandAI(creature);
        }
};

void AddSC_brf_iron_assembly()
{
    new boss_beastlord_darmac();
    new boss_operator_thogar();
    new boss_iron_maidens();
    new boss_blackhand();
}
