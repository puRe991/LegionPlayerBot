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
SDName: Boss_Ghazan
SD%Complete: 90
SDComment: Acid Breath is a frontal cone, so the raid stays behind him; the
           tail sweep punishes standing there. Enrages near the end.
SDCategory: Coilfang Reservoir, The Underbog
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

// Entry and spell ids for this encounter. Check them against creature_template
// and the spell store of the world database before relying on them.
enum Spells
{
    SPELL_ACID_BREATH       = 34268,
    SPELL_ACID_BREATH_H     = 38116,
    SPELL_TAIL_SWEEP        = 34267,
    SPELL_TAIL_SWEEP_H      = 38737,
    SPELL_ACID_SPIT         = 34290,
    SPELL_ENRAGE            = 15716
};

enum Events
{
    EVENT_ACID_BREATH       = 1,
    EVENT_TAIL_SWEEP        = 2,
    EVENT_ACID_SPIT         = 3
};

enum Misc
{
    ENRAGE_HEALTH_PCT       = 20
};

class boss_ghazan : public CreatureScript
{
    public:
        boss_ghazan() : CreatureScript("boss_ghazan") { }

        struct boss_ghazanAI : public ScriptedAI
        {
            boss_ghazanAI(Creature* creature) : ScriptedAI(creature)
            {
                _heroic = creature->GetMap()->IsHeroic();
            }

            void Reset()
            {
                _events.Reset();
                _enraged = false;
                me->RemoveAurasDueToSpell(SPELL_ENRAGE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _events.ScheduleEvent(EVENT_ACID_BREATH, urand(8000, 12000));
                _events.ScheduleEvent(EVENT_TAIL_SWEEP, urand(12000, 16000));
                _events.ScheduleEvent(EVENT_ACID_SPIT, urand(5000, 9000));
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_enraged || !HealthBelowPct(ENRAGE_HEALTH_PCT))
                    return;

                _enraged = true;
                DoCast(me, SPELL_ENRAGE, true);
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
                        case EVENT_ACID_BREATH:
                            DoCastVictim(_heroic ? SPELL_ACID_BREATH_H : SPELL_ACID_BREATH);
                            _events.ScheduleEvent(EVENT_ACID_BREATH, urand(8000, 12000));
                            break;
                        case EVENT_TAIL_SWEEP:
                            DoCastAOE(_heroic ? SPELL_TAIL_SWEEP_H : SPELL_TAIL_SWEEP);
                            _events.ScheduleEvent(EVENT_TAIL_SWEEP, urand(12000, 16000));
                            break;
                        case EVENT_ACID_SPIT:
                            // Aimed at someone out of melee, so ranged cannot
                            // simply ignore him.
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                                DoCast(target, SPELL_ACID_SPIT);
                            _events.ScheduleEvent(EVENT_ACID_SPIT, urand(5000, 9000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
            bool _heroic;
            bool _enraged;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_ghazanAI(creature);
        }
};

void AddSC_boss_ghazan()
{
    new boss_ghazan();
}
