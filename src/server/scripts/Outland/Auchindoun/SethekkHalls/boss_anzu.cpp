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
SDName: Boss_Anzu
SD%Complete: 85
SDComment: The heroic-only raven god, summoned by a druid. Twice during the
           fight he banishes himself and sends his brood; the banish only
           breaks once the brood is dead. The instance already tracked
           TYPE_ANZU_ENCOUNTER but nothing ever drove it.
SDCategory: Auchindoun, Sethekk Halls
EndScriptData */

#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "sethekk_halls.h"

// Entry and spell ids for this encounter. Check them against creature_template
// and the spell store of the world database before relying on them.
enum Spells
{
    SPELL_PARALYZING_SCREECH    = 40184,
    SPELL_SPELL_BOMB            = 40303,
    SPELL_CYCLONE_OF_FEATHERS   = 40321,
    SPELL_BANISH_SELF           = 42354
};

enum Creatures
{
    NPC_BROOD_OF_ANZU           = 23132
};

enum Events
{
    EVENT_PARALYZING_SCREECH    = 1,
    EVENT_SPELL_BOMB            = 2,
    EVENT_CYCLONE_OF_FEATHERS   = 3
};

enum Misc
{
    // He goes untouchable at each of these, once only.
    FIRST_BANISH_PCT            = 66,
    SECOND_BANISH_PCT           = 33,
    BROOD_PER_BANISH            = 5,
    BROOD_DESPAWN_TIME          = 120000
};

class boss_anzu : public CreatureScript
{
    public:
        boss_anzu() : CreatureScript("boss_anzu") { }

        struct boss_anzuAI : public ScriptedAI
        {
            boss_anzuAI(Creature* creature) : ScriptedAI(creature), _summons(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                _events.Reset();
                _summons.DespawnAll();
                _banishesUsed = 0;
                _broodAlive = 0;
                EndBanish();
            }

            void EnterCombat(Unit* /*who*/)
            {
                if (_instance)
                    _instance->SetData(TYPE_ANZU_ENCOUNTER, IN_PROGRESS);

                ScheduleCombatEvents();
            }

            void JustDied(Unit* /*killer*/)
            {
                if (_instance)
                    _instance->SetData(TYPE_ANZU_ENCOUNTER, DONE);

                _summons.DespawnAll();
            }

            void EnterEvadeMode()
            {
                if (_instance)
                    _instance->SetData(TYPE_ANZU_ENCOUNTER, FAIL);

                _summons.DespawnAll();
                ScriptedAI::EnterEvadeMode();
            }

            void ScheduleCombatEvents()
            {
                _events.ScheduleEvent(EVENT_PARALYZING_SCREECH, urand(12000, 16000));
                _events.ScheduleEvent(EVENT_SPELL_BOMB, urand(8000, 12000));
                _events.ScheduleEvent(EVENT_CYCLONE_OF_FEATHERS, urand(20000, 25000));
            }

            void JustSummoned(Creature* summon)
            {
                _summons.Summon(summon);

                if (summon->GetEntry() != NPC_BROOD_OF_ANZU)
                    return;

                ++_broodAlive;

                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                    summon->AI()->AttackStart(target);
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
            {
                if (summon->GetEntry() != NPC_BROOD_OF_ANZU || !_broodAlive)
                    return;

                if (!--_broodAlive)
                    EndBanish();
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*dmgType*/)
            {
                if (_banished)
                {
                    // Nothing touches him until the brood is dealt with.
                    damage = 0;
                    return;
                }

                if (_banishesUsed == 0 && HealthBelowPct(FIRST_BANISH_PCT))
                    StartBanish();
                else if (_banishesUsed == 1 && HealthBelowPct(SECOND_BANISH_PCT))
                    StartBanish();
            }

            void StartBanish()
            {
                ++_banishesUsed;
                _banished = true;
                _broodAlive = 0;

                _events.Reset();
                me->AttackStop();
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                DoCast(me, SPELL_BANISH_SELF, true);

                for (uint8 i = 0; i < BROOD_PER_BANISH; ++i)
                {
                    Position pos;
                    me->GetRandomNearPosition(pos, 15.0f);
                    me->SummonCreature(NPC_BROOD_OF_ANZU, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, BROOD_DESPAWN_TIME);
                }

                // If the brood could not be spawned at all, do not leave him
                // permanently untouchable.
                if (!_broodAlive)
                    EndBanish();
            }

            void EndBanish()
            {
                _banished = false;

                me->RemoveAurasDueToSpell(SPELL_BANISH_SELF);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_AGGRESSIVE);

                if (!me->isInCombat())
                    return;

                ScheduleCombatEvents();

                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                    AttackStart(target);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (_banished)
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_PARALYZING_SCREECH:
                            DoCastAOE(SPELL_PARALYZING_SCREECH);
                            _events.ScheduleEvent(EVENT_PARALYZING_SCREECH, urand(12000, 16000));
                            break;
                        case EVENT_SPELL_BOMB:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_SPELL_BOMB);
                            _events.ScheduleEvent(EVENT_SPELL_BOMB, urand(8000, 12000));
                            break;
                        case EVENT_CYCLONE_OF_FEATHERS:
                            // Aimed at whoever is holding him, to force a
                            // tank swap or a cooldown.
                            DoCastVictim(SPELL_CYCLONE_OF_FEATHERS);
                            _events.ScheduleEvent(EVENT_CYCLONE_OF_FEATHERS, urand(20000, 25000));
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
            SummonList _summons;
            uint8 _banishesUsed{};
            uint8 _broodAlive{};
            bool _banished{};
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_anzuAI(creature);
        }
};

void AddSC_boss_anzu()
{
    new boss_anzu();
}
