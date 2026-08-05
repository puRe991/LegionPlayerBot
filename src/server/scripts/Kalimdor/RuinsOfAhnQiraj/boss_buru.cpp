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
SDName: Boss_Buru
SD%Complete: 90
SDComment: Buru chases one player at a time and takes his real damage from the
           eggs scattered around the pit. Below 20% he abandons the chase and
           fights normally.
SDCategory: Ruins of Ahn'Qiraj
EndScriptData */

#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ruins_of_ahnqiraj.h"

enum Texts
{
    EMOTE_TARGET            = 0     // "%s focuses on $N!"
};

enum Spells
{
    SPELL_CREEPING_PLAGUE   = 20512,
    SPELL_DISMEMBER         = 96,
    SPELL_GATHERING_SPEED   = 25207,
    SPELL_FULL_SPEED        = 1557,
    SPELL_THORNS            = 25640,
    SPELL_BURU_TRANSFORM    = 24721,
    SPELL_EXPLODE           = 19593
};

enum Events
{
    EVENT_DISMEMBER         = 1,
    EVENT_GATHERING_SPEED   = 2,
    EVENT_FULL_SPEED        = 3,
    EVENT_CREEPING_PLAGUE   = 4
};

enum Misc
{
    // How long Buru hunts one player before giving up and picking someone
    // else. He normally catches his target long before this expires.
    FIXATE_DURATION         = 30000,
    GATHERING_SPEED_PERIOD  = 9000,
    FULL_SPEED_DELAY        = 60000
};

class boss_buru : public CreatureScript
{
    public:
        boss_buru() : CreatureScript("boss_buru") { }

        struct boss_buruAI : public BossAI
        {
            boss_buruAI(Creature* creature) : BossAI(creature, BOSS_BURU), _fixateTimer(FIXATE_DURATION), _transformed(false)
            {
            }

            void Reset()
            {
                _Reset();
                _transformed = false;
                _fixateGUID = ObjectGuid::Empty;
                _fixateTimer = FIXATE_DURATION;
                me->RemoveAurasDueToSpell(SPELL_GATHERING_SPEED);
                me->RemoveAurasDueToSpell(SPELL_FULL_SPEED);
                me->RemoveAurasDueToSpell(SPELL_BURU_TRANSFORM);
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                DoCast(me, SPELL_THORNS, true);

                events.ScheduleEvent(EVENT_DISMEMBER, urand(5000, 8000));
                events.ScheduleEvent(EVENT_GATHERING_SPEED, GATHERING_SPEED_PERIOD);
                events.ScheduleEvent(EVENT_FULL_SPEED, FULL_SPEED_DELAY);

                Fixate(who);
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
                summons.DespawnAll();
            }

            // While hunting, Buru ignores the threat table: he walks towards
            // one player and nothing the tank does changes that.
            void Fixate(Unit* preferred)
            {
                if (_transformed)
                    return;

                Unit* target = preferred;
                if (!target || !target->isAlive() || target->GetTypeId() != TYPEID_PLAYER)
                    target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true);

                if (!target)
                    return;

                _fixateGUID = target->GetGUID();
                _fixateTimer = FIXATE_DURATION;

                // The speed stacks belong to the chase, not to the boss.
                me->RemoveAurasDueToSpell(SPELL_GATHERING_SPEED);
                me->RemoveAurasDueToSpell(SPELL_FULL_SPEED);
                events.RescheduleEvent(EVENT_GATHERING_SPEED, GATHERING_SPEED_PERIOD);
                events.RescheduleEvent(EVENT_FULL_SPEED, FULL_SPEED_DELAY);

                DoResetThreat();
                me->AddThreat(target, 1000000.0f);
                AttackStart(target);

                Talk(EMOTE_TARGET, target->GetGUID());
            }

            // A destroyed egg is worth a fresh chase — that is how the raid
            // drags him from one egg to the next.
            void SetData(uint32 type, uint32 /*value*/)
            {
                if (type == NPC_BURU_EGG)
                    Fixate(NULL);
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetGUID() == _fixateGUID)
                    Fixate(NULL);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*dmgType*/)
            {
                if (_transformed || !HealthBelowPct(20))
                    return;

                _transformed = true;
                _fixateGUID = ObjectGuid::Empty;

                me->RemoveAurasDueToSpell(SPELL_GATHERING_SPEED);
                me->RemoveAurasDueToSpell(SPELL_FULL_SPEED);
                DoCast(me, SPELL_BURU_TRANSFORM, true);

                events.CancelEvent(EVENT_GATHERING_SPEED);
                events.CancelEvent(EVENT_FULL_SPEED);
                events.ScheduleEvent(EVENT_CREEPING_PLAGUE, 6000);

                // Back to ordinary threat handling.
                DoResetThreat();
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                    AttackStart(target);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (!_transformed)
                {
                    Unit* fixated = ObjectAccessor::GetUnit(*me, _fixateGUID);
                    if (!fixated || !fixated->isAlive() || _fixateTimer <= diff)
                        Fixate(NULL);
                    else
                        _fixateTimer -= diff;
                }

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DISMEMBER:
                            DoCastVictim(SPELL_DISMEMBER);
                            events.ScheduleEvent(EVENT_DISMEMBER, urand(5000, 8000));
                            break;
                        case EVENT_GATHERING_SPEED:
                            DoCast(me, SPELL_GATHERING_SPEED, true);
                            events.ScheduleEvent(EVENT_GATHERING_SPEED, GATHERING_SPEED_PERIOD);
                            break;
                        case EVENT_FULL_SPEED:
                            DoCast(me, SPELL_FULL_SPEED, true);
                            break;
                        case EVENT_CREEPING_PLAGUE:
                            DoCastAOE(SPELL_CREEPING_PLAGUE);
                            events.ScheduleEvent(EVENT_CREEPING_PLAGUE, 6000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            ObjectGuid _fixateGUID;
            uint32 _fixateTimer;
            bool _transformed;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_buruAI(creature);
        }
};

// The eggs sit in the pit and do nothing until destroyed. Killing one next to
// Buru is the only reliable way to hurt him.
class npc_buru_egg : public CreatureScript
{
    public:
        npc_buru_egg() : CreatureScript("npc_buru_egg") { }

        struct npc_buru_eggAI : public ScriptedAI
        {
            npc_buru_eggAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
            }

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
            }

            void JustDied(Unit* /*killer*/)
            {
                DoCastAOE(SPELL_EXPLODE, true);

                me->SummonCreature(NPC_HIVEZARA_HATCHLING, me->GetPositionX(), me->GetPositionY(),
                    me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);

                if (Creature* buru = me->FindNearestCreature(NPC_BURU, 150.0f))
                    if (buru->AI())
                        buru->AI()->SetData(NPC_BURU_EGG, 0);
            }

            void UpdateAI(uint32 /*diff*/) { }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_buru_eggAI(creature);
        }
};

void AddSC_boss_buru()
{
    new boss_buru();
    new npc_buru_egg();
}
