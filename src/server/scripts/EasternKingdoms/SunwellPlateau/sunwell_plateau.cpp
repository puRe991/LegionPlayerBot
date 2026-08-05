/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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
SDName: Sunwell_Plateau
SD%Complete: 90
SDComment: Epilogue after Kil'jaeden and Captain Selana's gossip. Velen is
           summoned by Kil'jaeden's death and speaks his eight lines, Lady
           Liadrin answers with three, then both leave.
SDCategory: Sunwell Plateau
EndScriptData */

/* ContentData
npc_prophet_velen
npc_lady_liadrin
npc_captain_selana
EndContentData */

#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "sunwell_plateau.h"

/*######
## npc_prophet_velen
######*/

// creature_text group ids. The old negative script_texts ids this file used
// to carry are not resolvable by this core any more.
enum ProphetSpeeches
{
    PROPHET_SAY1 = 0,
    PROPHET_SAY2 = 1,
    PROPHET_SAY3 = 2,
    PROPHET_SAY4 = 3,
    PROPHET_SAY5 = 4,
    PROPHET_SAY6 = 5,
    PROPHET_SAY7 = 6,
    PROPHET_SAY8 = 7
};

enum LiadrinnSpeeches
{
    LIADRIN_SAY1 = 0,
    LIADRIN_SAY2 = 1,
    LIADRIN_SAY3 = 2
};

enum EpilogueMisc
{
    // Liadrin's entry is not carried in sunwell_plateau.h; the epilogue finds
    // her by script instead, so she only has to be given the ScriptName.
    ACTION_START_LIADRIN    = 1,
    EPILOGUE_LINE_DELAY     = 9000,
    EPILOGUE_DESPAWN_DELAY  = 15000
};

class npc_prophet_velen : public CreatureScript
{
    public:
        npc_prophet_velen() : CreatureScript("npc_prophet_velen") { }

        struct npc_prophet_velenAI : public ScriptedAI
        {
            npc_prophet_velenAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
            }

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
                _line = 0;
                _timer = EPILOGUE_LINE_DELAY;
                _finished = false;
            }

            void UpdateAI(uint32 diff)
            {
                if (_finished)
                    return;

                if (_timer > diff)
                {
                    _timer -= diff;
                    return;
                }

                _timer = EPILOGUE_LINE_DELAY;

                if (_line <= PROPHET_SAY8)
                {
                    Talk(_line);
                    ++_line;
                    return;
                }

                // Velen is done; Liadrin answers him.
                _finished = true;

                if (Creature* liadrin = FindLiadrin())
                    if (liadrin->AI())
                        liadrin->AI()->DoAction(ACTION_START_LIADRIN);

                me->DespawnOrUnsummon(EPILOGUE_DESPAWN_DELAY);
            }

            // Liadrin stands with him at the end of the fight; she is picked
            // out by her AI rather than by a hard-coded entry.
            Creature* FindLiadrin() const
            {
                std::list<Creature*> nearby;
                me->GetCreatureListWithEntryInGrid(nearby, 0, 60.0f);

                for (Creature* creature : nearby)
                    if (creature != me && creature->GetScriptName() == "npc_lady_liadrin")
                        return creature;

                return NULL;
            }

        private:
            uint8 _line;
            uint32 _timer;
            bool _finished;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_prophet_velenAI(creature);
        }
};

/*######
## npc_lady_liadrin
######*/

class npc_lady_liadrin : public CreatureScript
{
    public:
        npc_lady_liadrin() : CreatureScript("npc_lady_liadrin") { }

        struct npc_lady_liadrinAI : public ScriptedAI
        {
            npc_lady_liadrinAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
            }

            void Reset()
            {
                _line = 0;
                _timer = 0;
                _speaking = false;
            }

            void DoAction(int32 const action)
            {
                if (action != ACTION_START_LIADRIN || _speaking)
                    return;

                _speaking = true;
                _line = 0;
                _timer = EPILOGUE_LINE_DELAY;
            }

            void UpdateAI(uint32 diff)
            {
                if (!_speaking)
                    return;

                if (_timer > diff)
                {
                    _timer -= diff;
                    return;
                }

                if (_line > LIADRIN_SAY3)
                {
                    _speaking = false;
                    return;
                }

                Talk(_line);
                ++_line;
                _timer = EPILOGUE_LINE_DELAY;
            }

        private:
            uint8 _line;
            uint32 _timer;
            bool _speaking;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lady_liadrinAI(creature);
        }
};

/*######
## npc_captain_selana
######*/

#define CS_GOSSIP1 "Give me a situation report, Captain."
#define CS_GOSSIP2 "What went wrong?"
#define CS_GOSSIP3 "Why did they stop?"
#define CS_GOSSIP4 "Your insight is appreciated."

enum SelanaMenus
{
    // Menu ids from the client's gossip_menu table.
    MENU_SELANA_1   = 9531,
    MENU_SELANA_2   = 9532,
    MENU_SELANA_3   = 9533,
    MENU_SELANA_4   = 9534
};

class npc_captain_selana : public CreatureScript
{
    public:
        npc_captain_selana() : CreatureScript("npc_captain_selana") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, CS_GOSSIP1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->SEND_GOSSIP_MENU(MENU_SELANA_1, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
        {
            player->PlayerTalkClass->ClearMenus();

            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, CS_GOSSIP2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                    player->SEND_GOSSIP_MENU(MENU_SELANA_2, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 2:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, CS_GOSSIP3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                    player->SEND_GOSSIP_MENU(MENU_SELANA_3, creature->GetGUID());
                    break;
                case GOSSIP_ACTION_INFO_DEF + 3:
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, CS_GOSSIP4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                    player->SEND_GOSSIP_MENU(MENU_SELANA_4, creature->GetGUID());
                    break;
                default:
                    player->CLOSE_GOSSIP_MENU();
                    break;
            }

            return true;
        }
};

void AddSC_sunwell_plateau()
{
    new npc_prophet_velen();
    new npc_lady_liadrin();
    new npc_captain_selana();
}
