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
SDName: Boss_Tavarok
SD%Complete: 90
SDComment: Crystal Prison takes a player out of the fight until the crystal is
           broken, which is what makes the earthquake dangerous.
SDCategory: Auchindoun, Mana-Tombs
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

// Spell ids for this encounter. Check them against the spell store of the
// world database before relying on them.
enum Spells
{
    SPELL_CRYSTAL_PRISON    = 32361,
    SPELL_ARCANE_EXPLOSION  = 15453,
    SPELL_EARTHQUAKE        = 33919
};

enum Events
{
    EVENT_CRYSTAL_PRISON    = 1,
    EVENT_ARCANE_EXPLOSION  = 2,
    EVENT_EARTHQUAKE        = 3
};

class boss_tavarok : public CreatureScript
{
    public:
        boss_tavarok() : CreatureScript("boss_tavarok") { }

        struct boss_tavarokAI : public ScriptedAI
        {
            boss_tavarokAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                _events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                _events.ScheduleEvent(EVENT_CRYSTAL_PRISON, urand(15000, 20000));
                _events.ScheduleEvent(EVENT_ARCANE_EXPLOSION, urand(8000, 12000));
                _events.ScheduleEvent(EVENT_EARTHQUAKE, urand(25000, 35000));
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
                        case EVENT_CRYSTAL_PRISON:
                            // Locking the tank away would just be a wipe, so
                            // it goes to someone else.
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_CRYSTAL_PRISON);
                            _events.ScheduleEvent(EVENT_CRYSTAL_PRISON, urand(15000, 20000));
                            break;
                        case EVENT_ARCANE_EXPLOSION:
                            DoCastAOE(SPELL_ARCANE_EXPLOSION);
                            _events.ScheduleEvent(EVENT_ARCANE_EXPLOSION, urand(8000, 12000));
                            break;
                        case EVENT_EARTHQUAKE:
                            DoCastAOE(SPELL_EARTHQUAKE);
                            _events.ScheduleEvent(EVENT_EARTHQUAKE, urand(25000, 35000));
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
            return new boss_tavarokAI(creature);
        }
};

void AddSC_boss_tavarok()
{
    new boss_tavarok();
}
