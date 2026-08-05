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
SDName: Chess_Event
SD%Complete: 40
SDComment: The encounter state only. instance_karazhan already carries
           TYPE_CHESS, both doors and the Dust Covered Chest, but nothing
           ever set the state -- so however the game in the room ended, the
           exit stayed shut and the chest never appeared. The two king
           scripts below close that loop: the game is won when Medivh's king
           falls and lost when the raid's king does.

           What is deliberately NOT here is the playable puzzle -- possessing
           a piece, restricting it to legal moves, the opposing side moving
           its own pieces, and Medivh's interventions. That needs the board
           squares, the piece entries and the movement spells from the world
           database, none of which this repository ships. Rather than guess
           them, the two scripts here bind by ScriptName only, so a realm
           points them at its own creatures without this file inventing ids.

           Assign these in creature_template.ScriptName:
             npc_chess_king_player_side  -- the king the raid plays
             npc_chess_king_medivh_side  -- the king Medivh plays
SDCategory: Karazhan
EndScriptData */

#include "GameObject.h"
#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "karazhan.h"

namespace
{
    // Both kings settle the encounter the same way, only the outcome differs.
    void FinishChessEvent(Creature* king, bool raidWon)
    {
        InstanceScript* instance = king->GetInstanceScript();
        if (!instance)
            return;

        if (instance->GetData(TYPE_CHESS) == DONE)
            return;

        instance->SetData(TYPE_CHESS, raidWon ? DONE : FAIL);

        // The instance stores the doors but never operates them, so the
        // encounter has to open its own way out.
        if (raidWon)
            instance->HandleGameObject(instance->GetGuidData(DATA_GO_GAME_EXIT_DOOR), true);
    }
}

/*######
## npc_chess_king_player_side
######*/

class npc_chess_king_player_side : public CreatureScript
{
    public:
        npc_chess_king_player_side() : CreatureScript("npc_chess_king_player_side") { }

        struct npc_chess_king_player_sideAI : public ScriptedAI
        {
            npc_chess_king_player_sideAI(Creature* creature) : ScriptedAI(creature) { }

            void EnterCombat(Unit* /*who*/)
            {
                if (InstanceScript* instance = me->GetInstanceScript())
                    if (instance->GetData(TYPE_CHESS) != DONE)
                        instance->SetData(TYPE_CHESS, IN_PROGRESS);
            }

            void JustDied(Unit* /*killer*/)
            {
                FinishChessEvent(me, false);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_chess_king_player_sideAI(creature);
        }
};

/*######
## npc_chess_king_medivh_side
######*/

class npc_chess_king_medivh_side : public CreatureScript
{
    public:
        npc_chess_king_medivh_side() : CreatureScript("npc_chess_king_medivh_side") { }

        struct npc_chess_king_medivh_sideAI : public ScriptedAI
        {
            npc_chess_king_medivh_sideAI(Creature* creature) : ScriptedAI(creature) { }

            void JustDied(Unit* /*killer*/)
            {
                FinishChessEvent(me, true);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_chess_king_medivh_sideAI(creature);
        }
};

void AddSC_chess_event()
{
    new npc_chess_king_player_side();
    new npc_chess_king_medivh_side();
}
