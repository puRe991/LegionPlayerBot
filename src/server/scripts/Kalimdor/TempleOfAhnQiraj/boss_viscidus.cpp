/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: Boss_Viscidus
SD%Complete: 85
SDComment: Frost damage freezes Viscidus solid, physical damage then shatters
           him into globs. Every glob that crawls back restores part of him,
           so the raid has to kill them before they arrive.
SDCategory: Temple of Ahn'Qiraj
EndScriptData */

#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "temple_of_ahnqiraj.h"

enum Texts
{
    EMOTE_SLOW          = 0,    // "Viscidus begins to slow."
    EMOTE_FREEZE        = 1,    // "Viscidus is frozen solid."
    EMOTE_CRACK         = 2,    // "Viscidus begins to crack."
    EMOTE_SHATTER       = 3     // "Viscidus explodes!"
};

enum Spells
{
    SPELL_TOXIN                 = 26575,
    SPELL_POISON_SHOCK          = 25993,
    SPELL_POISONBOLT_VOLLEY     = 25991,
    SPELL_TOXIC_CLOUD           = 25989,

    SPELL_VISCIDUS_SLOWED       = 26034,
    SPELL_VISCIDUS_SLOWED_MORE  = 26036,
    SPELL_VISCIDUS_FREEZE       = 25937,
    SPELL_MEMBRANE_VISCIDUS     = 25994,
    SPELL_REJOIN_VISCIDUS       = 25896
};

enum Creatures
{
    NPC_GLOB_OF_VISCIDUS        = 15667
};

enum Events
{
    EVENT_TOXIN                 = 1,
    EVENT_POISON_SHOCK          = 2,
    EVENT_POISONBOLT_VOLLEY     = 3,
    EVENT_TOXIC_CLOUD           = 4
};

enum Actions
{
    ACTION_GLOB_REJOINED        = 1,
    ACTION_GLOB_DIED            = 2
};

enum Phases
{
    PHASE_NORMAL                = 0,
    PHASE_FROZEN                = 1,
    PHASE_SHATTERED             = 2
};

// Hit counts that drive the freeze and the shatter. These are the thresholds
// the encounter is tuned around; adjust them here rather than in the code
// below if a realm wants a softer fight.
enum HitCounters
{
    HITS_TO_SLOW                = 100,
    HITS_TO_SLOW_MORE           = 150,
    HITS_TO_FREEZE              = 200,
    HITS_TO_CRACK               = 15,
    HITS_TO_SHATTER             = 35
};

enum Misc
{
    // Each glob carries this much of him back, so the number of globs follows
    // from how much health he had when he shattered.
    HEALTH_PCT_PER_GLOB         = 5,
    FROZEN_DURATION             = 15000,
    GLOB_REJOIN_DISTANCE        = 3
};

class boss_viscidus : public CreatureScript
{
    public:
        boss_viscidus() : CreatureScript("boss_viscidus") { }

        struct boss_viscidusAI : public ScriptedAI
        {
            boss_viscidusAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                events.Reset();
                _phase = PHASE_NORMAL;
                _frostHits = 0;
                _meleeHits = 0;
                _globsAlive = 0;
                _globsRejoined = 0;

                RemoveFreezeAuras();
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetVisible(true);
                me->SetReactState(REACT_AGGRESSIVE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoCast(me, SPELL_MEMBRANE_VISCIDUS, true);

                events.ScheduleEvent(EVENT_TOXIN, 10000);
                events.ScheduleEvent(EVENT_POISON_SHOCK, urand(7000, 12000));
                events.ScheduleEvent(EVENT_POISONBOLT_VOLLEY, urand(10000, 15000));
                events.ScheduleEvent(EVENT_TOXIC_CLOUD, urand(15000, 20000));
            }

            void JustDied(Unit* /*killer*/)
            {
                summons.DespawnAll();
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
            }

            void RemoveFreezeAuras()
            {
                me->RemoveAurasDueToSpell(SPELL_VISCIDUS_SLOWED);
                me->RemoveAurasDueToSpell(SPELL_VISCIDUS_SLOWED_MORE);
                me->RemoveAurasDueToSpell(SPELL_VISCIDUS_FREEZE);
            }

            // Frost is the only thing that stiffens him, and only while he is
            // still liquid.
            void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
            {
                if (_phase != PHASE_NORMAL || !spell)
                    return;

                if (!(spell->GetSchoolMask() & SPELL_SCHOOL_MASK_FROST))
                    return;

                ++_frostHits;

                if (_frostHits == HITS_TO_SLOW)
                {
                    Talk(EMOTE_SLOW);
                    DoCast(me, SPELL_VISCIDUS_SLOWED, true);
                }
                else if (_frostHits == HITS_TO_SLOW_MORE)
                {
                    me->RemoveAurasDueToSpell(SPELL_VISCIDUS_SLOWED);
                    DoCast(me, SPELL_VISCIDUS_SLOWED_MORE, true);
                }
                else if (_frostHits >= HITS_TO_FREEZE)
                    Freeze();
            }

            void Freeze()
            {
                _phase = PHASE_FROZEN;
                _meleeHits = 0;
                _frozenTimer = FROZEN_DURATION;

                me->RemoveAurasDueToSpell(SPELL_VISCIDUS_SLOWED);
                me->RemoveAurasDueToSpell(SPELL_VISCIDUS_SLOWED_MORE);
                DoCast(me, SPELL_VISCIDUS_FREEZE, true);

                Talk(EMOTE_FREEZE);
            }

            void Thaw()
            {
                _phase = PHASE_NORMAL;
                _frostHits = 0;
                _meleeHits = 0;
                RemoveFreezeAuras();
            }

            // Only weapon swings crack the shell — spell damage does nothing
            // here, which is why the fight wants melee stacked up.
            void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType dmgType)
            {
                if (_phase != PHASE_FROZEN || dmgType != DIRECT_DAMAGE)
                    return;

                ++_meleeHits;

                if (_meleeHits == HITS_TO_CRACK)
                    Talk(EMOTE_CRACK);
                else if (_meleeHits >= HITS_TO_SHATTER)
                {
                    damage = 0;
                    Shatter();
                }
            }

            void Shatter()
            {
                Talk(EMOTE_SHATTER);

                _phase = PHASE_SHATTERED;
                RemoveFreezeAuras();

                // He is gone until the globs settle it.
                me->AttackStop();
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetVisible(false);

                uint32 const healthPct = uint32(me->GetHealthPct());
                _globsAlive = std::max<uint32>(1, healthPct / HEALTH_PCT_PER_GLOB);
                _globsRejoined = 0;

                for (uint32 i = 0; i < _globsAlive; ++i)
                {
                    Position pos;
                    me->GetRandomNearPosition(pos, 20.0f);
                    me->SummonCreature(NPC_GLOB_OF_VISCIDUS, pos.GetPositionX(), pos.GetPositionY(),
                        pos.GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
                }
            }

            // Reported by the globs: one made it home, or one was killed.
            void DoAction(int32 const action)
            {
                if (_phase != PHASE_SHATTERED)
                    return;

                if (action == ACTION_GLOB_REJOINED)
                    ++_globsRejoined;
                else if (action != ACTION_GLOB_DIED)
                    return;

                if (_globsAlive)
                    --_globsAlive;

                if (_globsAlive)
                    return;

                if (!_globsRejoined)
                {
                    // Nothing came back — there is nothing left of him.
                    me->SetVisible(true);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->Kill(me);
                    return;
                }

                Reform();
            }

            void Reform()
            {
                me->SetVisible(true);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetHealth(me->CountPctFromMaxHealth(int32(_globsRejoined * HEALTH_PCT_PER_GLOB)));

                _phase = PHASE_NORMAL;
                _frostHits = 0;
                _meleeHits = 0;
                _globsRejoined = 0;

                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                    AttackStart(target);
            }

            void UpdateAI(uint32 diff)
            {
                if (_phase == PHASE_SHATTERED)
                    return;

                if (!UpdateVictim())
                    return;

                // Frozen wears off on its own if the raid cannot break him
                // open in time.
                if (_phase == PHASE_FROZEN)
                {
                    if (_frozenTimer <= diff)
                        Thaw();
                    else
                        _frozenTimer -= diff;
                    return;
                }

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_TOXIN:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_TOXIN);
                            events.ScheduleEvent(EVENT_TOXIN, urand(15000, 20000));
                            break;
                        case EVENT_POISON_SHOCK:
                            DoCastAOE(SPELL_POISON_SHOCK);
                            events.ScheduleEvent(EVENT_POISON_SHOCK, urand(7000, 12000));
                            break;
                        case EVENT_POISONBOLT_VOLLEY:
                            DoCastAOE(SPELL_POISONBOLT_VOLLEY);
                            events.ScheduleEvent(EVENT_POISONBOLT_VOLLEY, urand(10000, 15000));
                            break;
                        case EVENT_TOXIC_CLOUD:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_TOXIC_CLOUD);
                            events.ScheduleEvent(EVENT_TOXIC_CLOUD, urand(15000, 20000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
            SummonList summons{ me };
            uint32 _frostHits{};
            uint32 _meleeHits{};
            uint32 _frozenTimer{};
            uint32 _globsAlive{};
            uint32 _globsRejoined{};
            uint8 _phase{ PHASE_NORMAL };
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_viscidusAI(creature);
        }
};

// A piece of Viscidus crawling back to where he stood. Let it arrive and he
// gets that piece back.
class npc_glob_of_viscidus : public CreatureScript
{
    public:
        npc_glob_of_viscidus() : CreatureScript("npc_glob_of_viscidus") { }

        struct npc_glob_of_viscidusAI : public ScriptedAI
        {
            npc_glob_of_viscidusAI(Creature* creature) : ScriptedAI(creature)
            {
                _reported = false;
            }

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);

                if (Unit* owner = me->GetAnyOwner())
                {
                    _viscidusGUID = owner->GetGUID();
                    me->GetMotionMaster()->MovePoint(0, owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ());
                }
            }

            void Report(int32 action)
            {
                if (_reported)
                    return;

                _reported = true;

                if (Creature* viscidus = ObjectAccessor::GetCreature(*me, _viscidusGUID))
                    if (viscidus->AI())
                        viscidus->AI()->DoAction(action);
            }

            void JustDied(Unit* /*killer*/)
            {
                Report(ACTION_GLOB_DIED);
            }

            void UpdateAI(uint32 /*diff*/)
            {
                if (_reported)
                    return;

                Creature* viscidus = ObjectAccessor::GetCreature(*me, _viscidusGUID);
                if (!viscidus)
                    return;

                if (!me->IsWithinDistInMap(viscidus, float(GLOB_REJOIN_DISTANCE)))
                    return;

                DoCast(me, SPELL_REJOIN_VISCIDUS, true);
                Report(ACTION_GLOB_REJOINED);
                me->DespawnOrUnsummon();
            }

        private:
            ObjectGuid _viscidusGUID;
            bool _reported;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_glob_of_viscidusAI(creature);
        }
};

void AddSC_boss_viscidus()
{
    new boss_viscidus();
    new npc_glob_of_viscidus();
}
