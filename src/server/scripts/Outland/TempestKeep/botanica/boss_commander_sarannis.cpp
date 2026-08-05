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
SDName: Boss_Commander_Sarannis
SD%Complete: 90
SDComment: Arcane Resonance stacks on whoever she hits, and Arcane Devastation
           is what makes the stacks hurt. At half health she calls in a squad
           that has to be dealt with before the tank gets overwhelmed.
SDCategory: Tempest Keep, The Botanica
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

// Entry and spell ids for this encounter. Check them against creature_template
// and the spell store of the world database before relying on them.
enum Spells
{
    SPELL_ARCANE_RESONANCE      = 34794,
    SPELL_ARCANE_DEVASTATION    = 34799,
    SPELL_SUMMON_REINFORCEMENTS = 34803
};

enum Creatures
{
    NPC_BLOODWARDER_RESERVIST   = 17950,
    NPC_BLOODWARDER_MENDER      = 17936
};

enum Texts
{
    SAY_AGGRO                   = 0,
    SAY_SLAY                    = 1,
    SAY_DEATH                   = 2,
    SAY_SUMMON                  = 3
};

enum Events
{
    EVENT_ARCANE_RESONANCE      = 1,
    EVENT_ARCANE_DEVASTATION    = 2
};

enum Misc
{
    REINFORCEMENT_HEALTH_PCT    = 50
};

// Where the squad walks in from, relative to her own position.
Position const ReinforcementOffsets[4] =
{
    { -5.0f,  5.0f, 0.0f, 0.0f },
    {  5.0f,  5.0f, 0.0f, 0.0f },
    { -5.0f, -5.0f, 0.0f, 0.0f },
    {  5.0f, -5.0f, 0.0f, 0.0f }
};

class boss_commander_sarannis : public CreatureScript
{
    public:
        boss_commander_sarannis() : CreatureScript("boss_commander_sarannis") { }

        struct boss_commander_sarannisAI : public ScriptedAI
        {
            boss_commander_sarannisAI(Creature* creature) : ScriptedAI(creature), _summons(creature) { }

            void Reset()
            {
                _events.Reset();
                _summons.DespawnAll();
                _calledReinforcements = false;
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);
                _events.ScheduleEvent(EVENT_ARCANE_RESONANCE, urand(5000, 9000));
                _events.ScheduleEvent(EVENT_ARCANE_DEVASTATION, urand(12000, 16000));
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _summons.DespawnAll();
            }

            void JustSummoned(Creature* summon)
            {
                _summons.Summon(summon);

                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                    summon->AI()->AttackStart(target);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_calledReinforcements || !HealthBelowPct(REINFORCEMENT_HEALTH_PCT))
                    return;

                _calledReinforcements = true;
                Talk(SAY_SUMMON);
                DoCast(me, SPELL_SUMMON_REINFORCEMENTS, true);

                // The spell does the work on a populated realm; summoning the
                // squad directly keeps the encounter honest if it is missing.
                if (_summons.empty())
                {
                    for (uint8 i = 0; i < 4; ++i)
                    {
                        Position pos = me->GetPosition();
                        pos.m_positionX += ReinforcementOffsets[i].GetPositionX();
                        pos.m_positionY += ReinforcementOffsets[i].GetPositionY();

                        me->SummonCreature(i == 0 ? NPC_BLOODWARDER_MENDER : NPC_BLOODWARDER_RESERVIST,
                            pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
                    }
                }
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
                        case EVENT_ARCANE_RESONANCE:
                            DoCastVictim(SPELL_ARCANE_RESONANCE);
                            _events.ScheduleEvent(EVENT_ARCANE_RESONANCE, urand(20000, 25000));
                            break;
                        case EVENT_ARCANE_DEVASTATION:
                            DoCastVictim(SPELL_ARCANE_DEVASTATION);
                            _events.ScheduleEvent(EVENT_ARCANE_DEVASTATION, urand(12000, 16000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
            SummonList _summons;
            bool _calledReinforcements;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_commander_sarannisAI(creature);
        }
};

void AddSC_boss_commander_sarannis()
{
    new boss_commander_sarannis();
}
