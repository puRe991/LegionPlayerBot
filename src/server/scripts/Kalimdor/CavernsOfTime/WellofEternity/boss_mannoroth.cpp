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
SDName: Boss_Mannoroth
SD%Complete: 75
SDComment: The end of the Well of Eternity. Mannoroth is busy draining the Well
           and ignores the party until Varo'then falls; from then on he fights,
           and Varo'then's blade is what actually hurts him. Fel Firestorm
           covers the floor in flames the party has to keep moving through.

           The file shipped as zero bytes. Entry 54969, Fel Firestorm 103889
           and Fel Drain 104961 were checked against the Cataclysm database on
           Wowhead. The blade throw is driven from the instance rather than
           from a spell id, because the object behind it could not be verified.
SDCategory: Caverns of Time, Well of Eternity
EndScriptData */

#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "well_of_eternity.h"

enum MannorothSpells
{
    SPELL_FEL_FIRESTORM         = 103889,
    SPELL_FEL_DRAIN             = 104961
};

enum MannorothTexts
{
    SAY_VAROTHEN_DEAD           = 0,
    SAY_BLADE                   = 1,
    SAY_SLAY                    = 2,
    SAY_DEATH                   = 3
};

enum MannorothEvents
{
    EVENT_FEL_FIRESTORM         = 1,
    EVENT_FEL_DRAIN             = 2
};

enum MannorothMisc
{
    // Each blade throw takes this much of him. Three of them is the fight.
    BLADE_DAMAGE_PCT            = 30,

    // He is untouchable while Varo'then still stands.
    STAGE_VAROTHEN              = 0,
    STAGE_COMBAT                = 1
};

class boss_mannoroth : public CreatureScript
{
    public:
        boss_mannoroth() : CreatureScript("boss_mannoroth") { }

        struct boss_mannorothAI : public ScriptedAI
        {
            boss_mannorothAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                _events.Reset();
                _stage = STAGE_VAROTHEN;
                _bladesThrown = 0;

                // Draining the Well: present, hostile to look at, but not part
                // of the fight yet.
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case ACTION_VAROTHEN_DIED:
                        if (_stage != STAGE_VAROTHEN)
                            break;

                        _stage = STAGE_COMBAT;
                        Talk(SAY_VAROTHEN_DEAD);

                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        me->SetReactState(REACT_AGGRESSIVE);

                        _events.ScheduleEvent(EVENT_FEL_FIRESTORM, urand(12000, 16000));
                        _events.ScheduleEvent(EVENT_FEL_DRAIN, urand(20000, 25000));

                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            AttackStart(target);
                        break;

                    case ACTION_BLADE_THROWN:
                    {
                        if (_stage != STAGE_COMBAT)
                            break;

                        ++_bladesThrown;
                        Talk(SAY_BLADE);

                        // Ordinary weapons barely scratch him; the blade is
                        // what the encounter is built around.
                        uint32 const loss = uint32(me->CountPctFromMaxHealth(BLADE_DAMAGE_PCT));
                        if (me->GetHealth() > loss)
                            me->SetHealth(me->GetHealth() - loss);
                        else
                            me->Kill(me);
                        break;
                    }

                    default:
                        break;
                }
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);

                if (_instance)
                    _instance->SetBossState(DATA_MANNOROTH, DONE);
            }

            void EnterEvadeMode()
            {
                if (_instance)
                    _instance->SetBossState(DATA_MANNOROTH, FAIL);

                ScriptedAI::EnterEvadeMode();
            }

            void UpdateAI(uint32 diff)
            {
                if (_stage != STAGE_COMBAT)
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
                        case EVENT_FEL_FIRESTORM:
                            DoCast(me, SPELL_FEL_FIRESTORM);
                            _events.ScheduleEvent(EVENT_FEL_FIRESTORM, urand(25000, 30000));
                            break;
                        case EVENT_FEL_DRAIN:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_FEL_DRAIN);
                            _events.ScheduleEvent(EVENT_FEL_DRAIN, urand(20000, 25000));
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
            uint8 _stage;
            uint8 _bladesThrown;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_mannorothAI(creature);
        }
};

void AddSC_boss_mannoroth()
{
    new boss_mannoroth();
}
