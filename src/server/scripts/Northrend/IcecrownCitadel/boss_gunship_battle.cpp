/*
 * Copyright (C) 2008-2017 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Gunship_Battle
SD%Complete: 60
SDComment: The battle between the Skybreaker and Orgrim's Hammer. The instance
           already builds both ships and their crews in PrepareGunshipEvent;
           what was missing was anything that ran the fight or ended it, so
           the encounter could never start, never finish and never drop its
           chest.

           Implemented here: the encounter state, the crews holding their deck
           and fighting boarders, the enemy commander deciding the outcome,
           and the captain's chest for the right faction and difficulty.

           NOT implemented: the cannons, the rocket packs players use to board
           the other ship, and the commanders' full ability rotations. Those
           depend on spell ids and transport paths that cannot be checked
           against a world database here, so they are left to the database
           rather than guessed.
SDCategory: Icecrown Citadel
EndScriptData */

#include "GameObject.h"
#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Transport.h"
#include "icecrown_citadel.h"

enum GunshipEvents
{
    EVENT_CREW_ATTACK           = 1
};

enum GunshipMisc
{
    CHEST_RESPAWN_TIME          = DAY,
    CREW_ATTACK_INTERVAL        = 2000,
    CREW_SEARCH_RANGE           = 80
};

namespace
{
    // The chest differs by faction and raid size; the ids are already carried
    // in icecrown_citadel.h.
    uint32 GetCaptainChestEntry(InstanceScript* instance)
    {
        bool const alliance = instance->GetData(DATA_TEAM_IN_INSTANCE) == ALLIANCE;

        // Same way the rest of this instance asks about raid size and mode.
        bool const large = instance->instance->ToInstanceMap()
            && instance->instance->ToInstanceMap()->GetMaxPlayers() == 25;
        bool const heroic = instance->instance->IsHeroic();

        if (heroic)
        {
            if (large)
                return alliance ? GO_CAPITAN_CHEST_A_25H : GO_CAPITAN_CHEST_H_25H;

            return alliance ? GO_CAPITAN_CHEST_A_10H : GO_CAPITAN_CHEST_H_10H;
        }

        if (large)
            return alliance ? GO_CAPITAN_CHEST_A_25N : GO_CAPITAN_CHEST_H_25N;

        return alliance ? GO_CAPITAN_CHEST_A_10N : GO_CAPITAN_CHEST_H_10N;
    }

    void FinishGunshipBattle(Creature* commander)
    {
        InstanceScript* instance = commander->GetInstanceScript();
        if (!instance)
            return;

        if (instance->GetBossState(DATA_GUNSHIP_EVENT) == DONE)
            return;

        instance->SetBossState(DATA_GUNSHIP_EVENT, DONE);

        // Takes the gossip NPC that started the battle back out of the world.
        instance->SetData(DATA_GUNSHIP_START, DONE);

        commander->SummonGameObject(GetCaptainChestEntry(instance),
            commander->GetPositionX(), commander->GetPositionY(), commander->GetPositionZ(),
            commander->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, CHEST_RESPAWN_TIME);
    }
}

/*######
## npc_gunship_enemy_commander -- killing this one wins the battle
######*/

class npc_gunship_enemy_commander : public CreatureScript
{
    public:
        npc_gunship_enemy_commander() : CreatureScript("npc_gunship_enemy_commander") { }

        struct npc_gunship_enemy_commanderAI : public ScriptedAI
        {
            npc_gunship_enemy_commanderAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void EnterCombat(Unit* /*who*/)
            {
                if (_instance && _instance->GetBossState(DATA_GUNSHIP_EVENT) != DONE)
                    _instance->SetBossState(DATA_GUNSHIP_EVENT, IN_PROGRESS);
            }

            void JustDied(Unit* /*killer*/)
            {
                FinishGunshipBattle(me);
            }

            void EnterEvadeMode()
            {
                // A wipe puts the encounter back, but the ships stay where
                // they are so the raid can pull again without a full reset.
                if (_instance && _instance->GetBossState(DATA_GUNSHIP_EVENT) == IN_PROGRESS)
                    _instance->SetBossState(DATA_GUNSHIP_EVENT, FAIL);

                ScriptedAI::EnterEvadeMode();
            }

        private:
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_gunship_enemy_commanderAI>(creature);
        }
};

/*######
## npc_gunship_crew -- axethrowers, riflemen, mages, marines
######*/

// The crews stand on a moving transport, so they must never chase anyone off
// the deck. They hold their post and hit whatever boards them.
class npc_gunship_crew : public CreatureScript
{
    public:
        npc_gunship_crew() : CreatureScript("npc_gunship_crew") { }

        struct npc_gunship_crewAI : public ScriptedAI
        {
            npc_gunship_crewAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
            }

            void Reset()
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_CREW_ATTACK, CREW_ATTACK_INTERVAL);
            }

            void AttackStart(Unit* who)
            {
                AttackStartNoMove(who);
            }

            void UpdateAI(uint32 diff)
            {
                _events.Update(diff);

                if (!UpdateVictim())
                {
                    if (_events.ExecuteEvent() == EVENT_CREW_ATTACK)
                    {
                        if (Unit* target = me->SelectNearestTarget(float(CREW_SEARCH_RANGE)))
                            AttackStart(target);

                        _events.ScheduleEvent(EVENT_CREW_ATTACK, CREW_ATTACK_INTERVAL);
                    }
                    return;
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_gunship_crewAI>(creature);
        }
};

void AddSC_boss_gunship_battle()
{
    new npc_gunship_enemy_commander();
    new npc_gunship_crew();
}
