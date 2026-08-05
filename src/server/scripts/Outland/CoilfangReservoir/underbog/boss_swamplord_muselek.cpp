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
SDName: Boss_Swamplord_Muselek
SD%Complete: 85
SDComment: A hunter with a bear. Pulling one pulls the other, he traps whoever
           comes close and knocks away his own tank to get back to range.
SDCategory: Coilfang Reservoir, The Underbog
EndScriptData */

#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"

// Entry and spell ids for this encounter. Check them against creature_template
// and the spell store of the world database before relying on them.
enum Spells
{
    SPELL_AIMED_SHOT        = 31623,
    SPELL_MULTI_SHOT        = 31942,
    SPELL_SHOOT             = 31941,
    SPELL_FREEZING_TRAP     = 31932,
    SPELL_KNOCK_AWAY        = 18813
};

enum Creatures
{
    NPC_CLAW                = 17827
};

enum Events
{
    EVENT_AIMED_SHOT        = 1,
    EVENT_MULTI_SHOT        = 2,
    EVENT_FREEZING_TRAP     = 3,
    EVENT_KNOCK_AWAY        = 4,
    EVENT_SHOOT             = 5
};

class boss_swamplord_muselek : public CreatureScript
{
    public:
        boss_swamplord_muselek() : CreatureScript("boss_swamplord_muselek") { }

        struct boss_swamplord_muselekAI : public ScriptedAI
        {
            boss_swamplord_muselekAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                _events.Reset();
            }

            void EnterCombat(Unit* who)
            {
                // The bear never lets him fight alone.
                if (Creature* claw = me->FindNearestCreature(NPC_CLAW, 60.0f))
                    if (claw->isAlive() && !claw->isInCombat())
                        claw->AI()->AttackStart(who);

                _events.ScheduleEvent(EVENT_AIMED_SHOT, urand(6000, 10000));
                _events.ScheduleEvent(EVENT_MULTI_SHOT, urand(10000, 14000));
                _events.ScheduleEvent(EVENT_FREEZING_TRAP, urand(15000, 20000));
                _events.ScheduleEvent(EVENT_KNOCK_AWAY, urand(12000, 18000));
                _events.ScheduleEvent(EVENT_SHOOT, 2000);
            }

            void UpdateAI(uint32 diff)
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
                        case EVENT_AIMED_SHOT:
                            DoCastVictim(SPELL_AIMED_SHOT);
                            _events.ScheduleEvent(EVENT_AIMED_SHOT, urand(6000, 10000));
                            break;
                        case EVENT_MULTI_SHOT:
                            DoCastVictim(SPELL_MULTI_SHOT);
                            _events.ScheduleEvent(EVENT_MULTI_SHOT, urand(10000, 14000));
                            break;
                        case EVENT_FREEZING_TRAP:
                            // Never on the tank — the trap is meant to take a
                            // second player out of the fight.
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 30.0f, true))
                                DoCast(target, SPELL_FREEZING_TRAP);
                            _events.ScheduleEvent(EVENT_FREEZING_TRAP, urand(15000, 20000));
                            break;
                        case EVENT_KNOCK_AWAY:
                            DoCastVictim(SPELL_KNOCK_AWAY);
                            _events.ScheduleEvent(EVENT_KNOCK_AWAY, urand(12000, 18000));
                            break;
                        case EVENT_SHOOT:
                            // Only while nobody is in his face.
                            if (!me->IsWithinMeleeRange(me->getVictim()))
                                DoCastVictim(SPELL_SHOOT);
                            _events.ScheduleEvent(EVENT_SHOOT, 2000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_swamplord_muselekAI(creature);
        }
};

void AddSC_boss_swamplord_muselek()
{
    new boss_swamplord_muselek();
}
