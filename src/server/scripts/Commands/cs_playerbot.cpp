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
Name: playerbot_commandscript
%Complete: 100
Comment: In-game administration of the playerbot subsystem
Category: commandscripts
EndScriptData */

#include "Chat.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerBotMgr.h"
#include "ScriptMgr.h"
#include "World.h"

#include <string>

namespace
{
    // Accepts a class by name or by numeric id. Returns CLASS_NONE for "any",
    // which is what the manager expects when it should pick one itself.
    bool ParseClass(std::string const& text, Classes& result)
    {
        struct ClassName
        {
            char const* Name;
            Classes Value;
        };

        static ClassName const classNames[] =
        {
            { "warrior",     CLASS_WARRIOR      },
            { "paladin",     CLASS_PALADIN      },
            { "hunter",      CLASS_HUNTER       },
            { "rogue",       CLASS_ROGUE        },
            { "priest",      CLASS_PRIEST       },
            { "deathknight", CLASS_DEATH_KNIGHT },
            { "dk",          CLASS_DEATH_KNIGHT },
            { "shaman",      CLASS_SHAMAN       },
            { "mage",        CLASS_MAGE         },
            { "warlock",     CLASS_WARLOCK      },
            { "monk",        CLASS_MONK         },
            { "druid",       CLASS_DRUID        },
            { "demonhunter", CLASS_DEMON_HUNTER },
            { "dh",          CLASS_DEMON_HUNTER },
        };

        if (text.empty() || text == "any" || text == "random")
        {
            result = CLASS_NONE;
            return true;
        }

        for (ClassName const& entry : classNames)
        {
            if (text == entry.Name)
            {
                result = entry.Value;
                return true;
            }
        }

        // Numeric form, so the ids used elsewhere in the bot code still work.
        char* end = nullptr;
        long const numeric = strtol(text.c_str(), &end, 10);
        if (end && !*end && numeric >= CLASS_WARRIOR && numeric < MAX_CLASSES)
        {
            result = Classes(numeric);
            return true;
        }

        return false;
    }

    bool ParseFaction(std::string const& text, bool& alliance)
    {
        if (text == "alliance" || text == "a")
        {
            alliance = true;
            return true;
        }

        if (text == "horde" || text == "h")
        {
            alliance = false;
            return true;
        }

        return false;
    }

    std::string NextToken(char const*& args)
    {
        while (*args == ' ')
            ++args;

        char const* start = args;
        while (*args && *args != ' ')
            ++args;

        return std::string(start, args - start);
    }
}

class playerbot_commandscript : public CommandScript
{
public:
    playerbot_commandscript() : CommandScript("playerbot_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> playerbotCommandTable =
        {
            { "add",    SEC_GAMEMASTER,    true, &HandlePlayerBotAddCommand,    "" },
            { "remove", SEC_GAMEMASTER,    true, &HandlePlayerBotRemoveCommand, "" },
            { "limit",  SEC_ADMINISTRATOR, true, &HandlePlayerBotLimitCommand,  "" },
            { "status", SEC_GAMEMASTER,    true, &HandlePlayerBotStatusCommand, "" },
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "playerbot", SEC_GAMEMASTER, true, NULL, "", playerbotCommandTable },
            { "pbot",      SEC_GAMEMASTER, true, NULL, "", playerbotCommandTable },
        };

        return commandTable;
    }

    // .playerbot add <alliance|horde> [class] [count]
    static bool HandlePlayerBotAddCommand(ChatHandler* handler, char const* args)
    {
        if (!sWorld->getBoolConfig(CONFIG_PLAYERBOT_ENABLE))
        {
            handler->SendSysMessage("The playerbot subsystem is disabled (PlayerBot.Enable).");
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string const factionText = NextToken(args);
        bool alliance = true;
        if (!ParseFaction(factionText, alliance))
        {
            handler->SendSysMessage("Syntax: .playerbot add <alliance|horde> [class] [count]");
            handler->SetSentErrorMessage(true);
            return false;
        }

        Classes botClass = CLASS_NONE;
        std::string const classText = NextToken(args);
        if (!ParseClass(classText, botClass))
        {
            handler->PSendSysMessage("Unknown class '%s'.", classText.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 count = 1;
        std::string const countText = NextToken(args);
        if (!countText.empty())
        {
            long const parsed = strtol(countText.c_str(), nullptr, 10);
            if (parsed < 1 || parsed > 100)
            {
                handler->SendSysMessage("Count has to be between 1 and 100.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            count = uint32(parsed);
        }

        uint32 const online = sPlayerBotMgr->GetOnlineBotCount(TEAM_ALLIANCE, true)
                            + sPlayerBotMgr->GetOnlineBotCount(TEAM_HORDE, true);
        uint32 const limit = sWorld->getIntConfig(CONFIG_PLAYERBOT_MAX_ONLINE);
        if (online >= limit)
        {
            handler->PSendSysMessage("The online limit of %u is already reached (PlayerBot.MaxOnline).", limit);
            handler->SetSentErrorMessage(true);
            return false;
        }

        sPlayerBotMgr->AddNewPlayerBot(alliance, botClass, count);

        handler->PSendSysMessage("Queued %u %s bot(s)%s%s. %u of %u online.",
            count,
            alliance ? "alliance" : "horde",
            botClass != CLASS_NONE ? " of class " : "",
            botClass != CLASS_NONE ? classText.c_str() : "",
            online, limit);
        return true;
    }

    // .playerbot remove <all|account id>
    static bool HandlePlayerBotRemoveCommand(ChatHandler* handler, char const* args)
    {
        std::string const target = NextToken(args);
        if (target.empty())
        {
            handler->SendSysMessage("Syntax: .playerbot remove <all|account id>");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (target == "all")
        {
            sPlayerBotMgr->AllPlayerBotLogout();
            handler->SendSysMessage("All playerbots have been logged out.");
            return true;
        }

        char* end = nullptr;
        long const accountId = strtol(target.c_str(), &end, 10);
        if (!end || *end || accountId <= 0)
        {
            handler->SendSysMessage("Syntax: .playerbot remove <all|account id>");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!sPlayerBotMgr->PlayerBotLogout(uint32(accountId)))
        {
            handler->PSendSysMessage("No playerbot online on account %ld.", accountId);
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Logged out the playerbot on account %ld.", accountId);
        return true;
    }

    // .playerbot limit <count>
    static bool HandlePlayerBotLimitCommand(ChatHandler* handler, char const* args)
    {
        std::string const countText = NextToken(args);
        if (countText.empty())
        {
            handler->PSendSysMessage("Current limit: %u. Syntax: .playerbot limit <count>",
                sWorld->getIntConfig(CONFIG_PLAYERBOT_MAX_ONLINE));
            return true;
        }

        char* end = nullptr;
        long const limit = strtol(countText.c_str(), &end, 10);
        if (!end || *end || limit < 0 || limit > 500)
        {
            handler->SendSysMessage("The limit has to be between 0 and 500.");
            handler->SetSentErrorMessage(true);
            return false;
        }

        sWorld->setIntConfig(CONFIG_PLAYERBOT_MAX_ONLINE, uint32(limit));
        sPlayerBotMgr->SetMax(int(limit));

        handler->PSendSysMessage("Playerbot online limit set to %ld for this session. "
            "Change PlayerBot.MaxOnline to make it permanent.", limit);
        return true;
    }

    // .playerbot status
    static bool HandlePlayerBotStatusCommand(ChatHandler* handler, char const* /*args*/)
    {
        uint32 const alliance = sPlayerBotMgr->GetOnlineBotCount(TEAM_ALLIANCE, true);
        uint32 const horde = sPlayerBotMgr->GetOnlineBotCount(TEAM_HORDE, true);

        handler->PSendSysMessage("Playerbots: %s", sWorld->getBoolConfig(CONFIG_PLAYERBOT_ENABLE) ? "enabled" : "disabled");
        handler->PSendSysMessage("Online: %u alliance, %u horde, %u of %u total.",
            alliance, horde, alliance + horde, sWorld->getIntConfig(CONFIG_PLAYERBOT_MAX_ONLINE));

        char const* lootBehaviour;
        switch (sWorld->getIntConfig(CONFIG_PLAYERBOT_LOOT_NEEDROLL))
        {
            case 0:  lootBehaviour = "always pass"; break;
            case 1:  lootBehaviour = "greed only"; break;
            default: lootBehaviour = "need on upgrades"; break;
        }
        handler->PSendSysMessage("Loot: %s, minimum quality %u.",
            lootBehaviour, sWorld->getIntConfig(CONFIG_PLAYERBOT_LOOT_MINQUALITY));
        return true;
    }
};

void AddSC_playerbot_commandscript()
{
    new playerbot_commandscript();
}
