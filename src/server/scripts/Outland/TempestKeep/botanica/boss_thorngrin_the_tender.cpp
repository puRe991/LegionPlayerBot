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
SDName: Boss_Thorngrin_the_Tender
SD%Complete: 90
SDComment: Sacrifice heals him off whoever it lands on, so the fight is a race
           between dispelling it and the Hellfire ticking on everyone.
SDCategory: Tempest Keep, The Botanica
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

// Spell ids for this encounter. Check them against the spell store of the
// world database before relying on them.
enum Spells
{
    SPELL_HELLFIRE          = 34659,
    SPELL_SACRIFICE         = 34661,
    SPELL_ENRAGE            = 34670
};

enum Texts
{
    SAY_AGGRO               = 0,
    SAY_SACRIFICE           = 1,
    SAY_ENRAGE              = 2,
    SAY_SLAY                = 3,
    SAY_DEATH               = 4
};

enum Events
{
    EVENT_HELLFIRE          = 1,
    EVENT_SACRIFICE         = 2,
    EVENT_ENRAGE            = 3
};

class boss_thorngrin_the_tender : public CreatureScript
{
    public:
        boss_thorngrin_the_tender() : CreatureScript("boss_thorngrin_the_tender") { }

        struct boss_thorngrin_the_tenderAI : public ScriptedAI
        {
            boss_thorngrin_the_tenderAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                _events.Reset();
                me->RemoveAurasDueToSpell(SPELL_ENRAGE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);
                _events.ScheduleEvent(EVENT_HELLFIRE, urand(12000, 18000));
                _events.ScheduleEvent(EVENT_SACRIFICE, urand(20000, 25000));
                _events.ScheduleEvent(EVENT_ENRAGE, urand(30000, 40000));
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
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
                        case EVENT_HELLFIRE:
                            DoCastAOE(SPELL_HELLFIRE);
                            _events.ScheduleEvent(EVENT_HELLFIRE, urand(12000, 18000));
                            break;
                        case EVENT_SACRIFICE:
                            // Anyone but the tank — the point is that the
                            // group has to react to it.
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            {
                                Talk(SAY_SACRIFICE);
                                DoCast(target, SPELL_SACRIFICE);
                            }
                            _events.ScheduleEvent(EVENT_SACRIFICE, urand(20000, 25000));
                            break;
                        case EVENT_ENRAGE:
                            Talk(SAY_ENRAGE);
                            DoCast(me, SPELL_ENRAGE);
                            _events.ScheduleEvent(EVENT_ENRAGE, urand(30000, 40000));
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
            return new boss_thorngrin_the_tenderAI(creature);
        }
};

void AddSC_boss_thorngrin_the_tender()
{
    new boss_thorngrin_the_tender();
}
