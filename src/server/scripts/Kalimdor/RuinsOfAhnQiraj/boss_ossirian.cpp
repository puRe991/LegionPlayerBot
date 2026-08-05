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
SDName: Boss_Ossirian
SD%Complete: 85
SDComment: Ossirian is untouchable while Strength of Ossirian holds. Clicking a
           sand crystal near him strips the buff and leaves him vulnerable to
           one school for half a minute. The whole fight is that cycle.
SDCategory: Ruins of Ahn'Qiraj
EndScriptData */

#include "GameObject.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ruins_of_ahnqiraj.h"

enum Texts
{
    SAY_AGGRO           = 0,
    SAY_SLAY            = 1,
    SAY_DEATH           = 2,
    SAY_SUPREME         = 3
};

enum Spells
{
    SPELL_SUPREME       = 25176,    // Strength of Ossirian
    SPELL_SAND_STORM    = 25160,
    SPELL_WAR_STOMP     = 25188,
    SPELL_CURSE_TONGUES = 25195
};

// One per magic school. A crystal picks one at random; while it holds,
// Ossirian takes real damage from that school and nothing else matters.
uint32 const WeaknessSpells[] =
{
    25177,      // Fire Weakness
    25178,      // Frost Weakness
    25180,      // Nature Weakness
    25181,      // Arcane Weakness
    25183       // Shadow Weakness
};

uint32 const WeaknessCount = sizeof(WeaknessSpells) / sizeof(WeaknessSpells[0]);

enum Events
{
    EVENT_SAND_STORM    = 1,
    EVENT_WAR_STOMP     = 2,
    EVENT_CURSE_TONGUES = 3,
    EVENT_SUPREME       = 4,
    EVENT_SPAWN_CRYSTAL = 5
};

enum Actions
{
    ACTION_TRIGGER_WEAKNESS = 1
};

enum Misc
{
    WEAKNESS_DURATION   = 45000,    // how long a crystal keeps him vulnerable
    CRYSTAL_RESPAWN     = 900,      // seconds the summoned crystal lives
    CRYSTAL_MIN_RANGE   = 15,
    CRYSTAL_MAX_RANGE   = 40
};

class boss_ossirian : public CreatureScript
{
    public:
        boss_ossirian() : CreatureScript("boss_ossirian") { }

        struct boss_ossirianAI : public BossAI
        {
            boss_ossirianAI(Creature* creature) : BossAI(creature, BOSS_OSSIRIAN)
            {
            }

            void Reset()
            {
                _Reset();
                DespawnCrystal();
                RemoveWeaknesses();
                me->RemoveAurasDueToSpell(SPELL_SUPREME);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                Talk(SAY_AGGRO);

                ApplySupreme();

                events.ScheduleEvent(EVENT_SAND_STORM, urand(20000, 25000));
                events.ScheduleEvent(EVENT_WAR_STOMP, urand(30000, 45000));
                events.ScheduleEvent(EVENT_CURSE_TONGUES, urand(20000, 30000));

                SpawnCrystal();
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* /*killer*/)
            {
                Talk(SAY_DEATH);
                _JustDied();
                summons.DespawnAll();
                DespawnCrystal();
            }

            void RemoveWeaknesses()
            {
                for (uint32 i = 0; i < WeaknessCount; ++i)
                    me->RemoveAurasDueToSpell(WeaknessSpells[i]);
            }

            void ApplySupreme()
            {
                RemoveWeaknesses();
                DoCast(me, SPELL_SUPREME, true);
                events.CancelEvent(EVENT_SUPREME);
            }

            // Called by the crystal when a player clicks it.
            void DoAction(int32 const action)
            {
                if (action != ACTION_TRIGGER_WEAKNESS)
                    return;

                me->RemoveAurasDueToSpell(SPELL_SUPREME);
                RemoveWeaknesses();

                DoCast(me, WeaknessSpells[urand(0, WeaknessCount - 1)], true);

                Talk(SAY_SUPREME);

                // He rearms himself once the crystal wears off, and the raid
                // needs a fresh one waiting by then.
                events.RescheduleEvent(EVENT_SUPREME, WEAKNESS_DURATION);
                events.RescheduleEvent(EVENT_SPAWN_CRYSTAL, WEAKNESS_DURATION / 2);
            }

            // Only one crystal is ever up; a stale one would let the raid
            // bank vulnerability windows.
            void DespawnCrystal()
            {
                if (GameObject* old = ObjectAccessor::GetGameObject(*me, _crystalGUID))
                    old->Delete();

                _crystalGUID = ObjectGuid::Empty;
            }

            // No fixed crystal spawn points are shipped with this core, so the
            // crystal is placed on open ground around Ossirian's own pillar.
            void SpawnCrystal()
            {
                DespawnCrystal();

                Position pos;
                me->GetRandomPoint(me->GetHomePosition(), float(urand(CRYSTAL_MIN_RANGE, CRYSTAL_MAX_RANGE)), pos);

                if (GameObject* crystal = me->SummonGameObject(GO_OSSIRIAN_CRYSTAL,
                        pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f, CRYSTAL_RESPAWN))
                    _crystalGUID = crystal->GetGUID();
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SAND_STORM:
                            DoCast(me, SPELL_SAND_STORM);
                            events.ScheduleEvent(EVENT_SAND_STORM, urand(20000, 25000));
                            break;
                        case EVENT_WAR_STOMP:
                            DoCastAOE(SPELL_WAR_STOMP);
                            events.ScheduleEvent(EVENT_WAR_STOMP, urand(30000, 45000));
                            break;
                        case EVENT_CURSE_TONGUES:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_CURSE_TONGUES);
                            events.ScheduleEvent(EVENT_CURSE_TONGUES, urand(20000, 30000));
                            break;
                        case EVENT_SUPREME:
                            ApplySupreme();
                            break;
                        case EVENT_SPAWN_CRYSTAL:
                            SpawnCrystal();
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            ObjectGuid _crystalGUID;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_ossirianAI(creature);
        }
};

// The crystal is the whole encounter: clicking it is what makes Ossirian
// killable. It is consumed in the process.
class go_ossirian_crystal : public GameObjectScript
{
    public:
        go_ossirian_crystal() : GameObjectScript("go_ossirian_crystal") { }

        bool OnGossipHello(Player* player, GameObject* go)
        {
            InstanceScript* instance = go->GetInstanceScript();
            if (!instance)
                return false;

            Creature* ossirian = ObjectAccessor::GetCreature(*go, instance->GetGuidData(BOSS_OSSIRIAN));
            if (!ossirian || !ossirian->isAlive() || !ossirian->isInCombat())
                return false;

            // Out of range the crystal does nothing — dragging him to it is
            // the point of the fight.
            if (!ossirian->IsWithinDistInMap(player, 50.0f))
                return false;

            ossirian->AI()->DoAction(ACTION_TRIGGER_WEAKNESS);

            go->SetRespawnTime(0);
            go->Delete();
            return true;
        }
};

void AddSC_boss_ossirian()
{
    new boss_ossirian();
    new go_ossirian_crystal();
}
