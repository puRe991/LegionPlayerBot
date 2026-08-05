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
SDName: Boss_Captain_Varothen
SD%Complete: 85
SDComment: The first half of the last encounter of the Well of Eternity. He
           holds the party off while Mannoroth drains the Well; Magistrike arcs
           between anyone standing together, so the group has to spread. His
           death hands the fight over to Mannoroth rather than ending it.

           The file shipped as zero bytes. Entry 55419 and Magistrike 103669
           were checked against the Cataclysm database on Wowhead.
SDCategory: Caverns of Time, Well of Eternity
EndScriptData */

#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "well_of_eternity.h"

enum VarothenSpells
{
    SPELL_MAGISTRIKE            = 103669
};

enum VarothenTexts
{
    SAY_AGGRO                   = 0,
    SAY_SLAY                    = 1,
    SAY_DEATH                   = 2
};

enum VarothenEvents
{
    EVENT_MAGISTRIKE            = 1
};

class boss_varothen : public CreatureScript
{
    public:
        boss_varothen() : CreatureScript("boss_varothen") { }

        struct boss_varothenAI : public ScriptedAI
        {
            boss_varothenAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                _events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);

                if (_instance)
                    _instance->SetBossState(DATA_MANNOROTH, IN_PROGRESS);

                _events.ScheduleEvent(EVENT_MAGISTRIKE, urand(8000, 12000));
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            // His death is the handover, not the end: Mannoroth becomes
            // attackable and the blade is left on the floor.
            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);

                if (!_instance)
                    return;

                if (Creature* mannoroth = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(DATA_MANNOROTH_GUID)))
                    if (mannoroth->AI())
                        mannoroth->AI()->DoAction(ACTION_VAROTHEN_DIED);
            }

            void EnterEvadeMode()
            {
                if (_instance)
                    _instance->SetBossState(DATA_MANNOROTH, FAIL);

                ScriptedAI::EnterEvadeMode();
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
                        case EVENT_MAGISTRIKE:
                            DoCastVictim(SPELL_MAGISTRIKE);
                            _events.ScheduleEvent(EVENT_MAGISTRIKE, urand(8000, 12000));
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

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_varothenAI(creature);
        }
};

void AddSC_boss_varothen()
{
    new boss_varothen();
}
