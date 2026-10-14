
#include "Containers.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PlayerBotMgr.h"
#include "World.h"
#include "DB2Stores.h"
#include "Player.h"
#include "BattlegroundMgr.h"
#include "BotAI.h"
#include "BotFieldAI.h"
#include "BotGroupAI.h"
#include "BotDuelAI.h"
#include "BotArenaAI.h"
#include "OnlineMgr.h"
#include "Group.h"
#include "GroupMgr.h"
#include "SocialMgr.h"
#include "LFGMgr.h"
#include "Config.h"
#include "PlayerBotSession.h"
#include "AccountMgr.h"
#include "CharacterData.h"
#include "CharacterPackets.h"
#include "GlobalFunctional.h"

#include <type_traits>
#include <vector>

#include <boost/algorithm/string.hpp>
//#include <boost/format.hpp>

// Grace period before an AI that was swapped out is actually freed.
static uint32 const AI_DESTROY_DELAY = 5000;

namespace
{
    // Class of a random character of the given faction on a bot account,
    // limited to prof unless that is CLASS_NONE; 0 when there is none.
    //
    // The Online schedule silently does nothing when the account has no
    // matching character, and the next request then picks the very same idle
    // session again. Bot characters are created by hand, so many accounts can
    // be empty; the callers use this to skip those instead of stalling on the
    // first one.
    uint32 PickBotCharacterClass(PlayerBotBaseInfo* info, bool alliance, uint32 prof)
    {
        if (!info)
            return 0;
        std::vector<uint32> classes;
        for (auto const& entry : info->characters)
        {
            if (!info->MatchRaceByFuction(alliance, entry.second.race))
                continue;
            if (prof != CLASS_NONE && entry.second.profession != prof)
                continue;
            classes.push_back(entry.second.profession);
        }
        if (classes.empty())
            return 0;
        return classes[urand(0, uint32(classes.size() - 1))];
    }
}

PlayerBotCharBaseInfo PlayerBotBaseInfo::empty;
std::map<uint32, std::list<UnitAI*> > PlayerBotMgr::m_DelayDestroyAIs;
std::mutex PlayerBotMgr::g_uniqueLock;

std::string PlayerBotCharBaseInfo::GetNameANDClassesText()
{
    //std::string clsName;
    uint32 clsEntry = 620000;
    switch (profession)
    {
        case 1:
            //clsName = "  ս  ʿ : ";
            clsEntry += 1;
            break;
        case 2:
            //clsName = "  ʥ��ʿ : ";
            clsEntry += 2;
            break;
        case 3:
            //clsName = "  ��  �� : ";
            clsEntry += 3;
            break;
        case 4:
            //clsName = "  ��  �� : ";
            clsEntry += 4;
            break;
        case 5:
            //clsName = "  ��  ʦ : ";
            clsEntry += 5;
            break;
        case 6:
            //clsName = "  ��  �� : ";
            clsEntry += 6;
            break;
        case 7:
            //clsName = "  ��  �� : ";
            clsEntry += 7;
            break;
        case 8:
            //clsName = "  ��  ʦ : ";
            clsEntry += 8;
            break;
        case 9:
            //clsName = "  ��  ʿ : ";
            clsEntry += 9;
            break;
        case 11:
            //clsName = "  ��³�� : ";
            clsEntry += 10;
            break;
    }
    std::string clsText = sObjectMgr->GetTrinityStringForDBCLocale(clsEntry);
    //consoleToUtf8(clsName, clsText);
    return clsText + name;
}

uint32 PlayerBotBaseInfo::GetCharIDByNoArenaType(bool faction, uint32 prof, uint32 arenaType, std::vector<ObjectGuid>& fliters)
{
    for (ObjectGuid& guid : fliters)
    {
        if (ExistCharacterByGUID(guid))
            return 0;
    }
    if (characters.size() <= 0)
        return 0;
    for (CharInfoMap::iterator it = characters.begin();
        it != characters.end();
        it++)
    {
        if (!MatchRaceByFuction(faction, it->second.race))
            continue;
        if (it->second.profession != prof)
            continue;
    }
    return 0;
}

PlayerBotMgr::PlayerBotMgr() :
    m_BotAccountAmount(180),
    m_LastBotAccountIndex(0),
    m_MaxOnlineBot(10),
    m_BotOnlineCount(0),
    m_LFGSearchTick(0),
    m_ArenaSearchTick(0),
    m_CreateIssued(0),
    m_MinOnlineBot(0),
    m_MinOnlineWarned(false),
    m_Announce(true)
{
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::BattlegroundWarsongGulch);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::BattlegroundArathiBasin);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::BattlegroundEyeOfTheStorm);
#ifndef NON_SINGLE_GAME
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::BattlegroundAlteracValley);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::BattlegroundIsleOfConquest);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::BattlegroundRandom);

    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaOldNagrandArena);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaOldBladesEdgeArena);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaRuinsOfLordaeron);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaOldRingOfValor);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaBladesEdge);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaAll);
    //m_BGTypes.push_back(BattlegroundTypeId::BATTLEGROUND_SA);
#endif
}

PlayerBotMgr::~PlayerBotMgr()
{
    ClearBaseInfo();
}

PlayerBotMgr* PlayerBotMgr::instance()
{
    static PlayerBotMgr instance;
    return &instance;
}

void PlayerBotMgr::SwitchPlayerBotAI(Player* player, PlayerBotAIType aiType, bool force)
{
    if (!force && player->isInCombat())
        return;
    if (force && player->isInCombat())
        player->ClearInCombat();
    player->SetSelection(ObjectGuid::Empty);
    UnitAI* pAI = player->GetAI();
    player->IsAIEnabled = false;
    switch (aiType)
    {
        case PlayerBotAIType::PBAIT_FIELD:
            if (pAI)
            {
                if (dynamic_cast<BotFieldAI*>(pAI) != NULL)
                {
                    player->IsAIEnabled = true;
                    return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            pAI = BotFieldAI::CreateBotFieldAIByPlayerClass(player);
            if (pAI)
            {
                pAI->Reset();
                player->SetAI(pAI);
                player->IsAIEnabled = true;
            }
            break;
        case PlayerBotAIType::PBAIT_GROUP:
            if (pAI)
            {
                if (dynamic_cast<BotGroupAI*>(pAI) != NULL)
                {
                    player->IsAIEnabled = true;
                    return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            pAI = BotGroupAI::CreateBotGroupAIByPlayerClass(player);
            if (pAI)
            {
                pAI->Reset();
                player->SetAI(pAI);
                player->IsAIEnabled = true;
            }
            break;
        case PlayerBotAIType::PBAIT_DUEL:
            if (pAI)
            {
                if (dynamic_cast<BotDuelAI*>(pAI) != NULL)
                {
                    player->IsAIEnabled = true;
                    return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            pAI = BotDuelAI::CreateBotDuelAIByPlayerClass(player);
            if (pAI)
            {
                pAI->Reset();
                player->SetAI(pAI);
                player->IsAIEnabled = true;
                ((BotDuelAI*)pAI)->ResetBotAI();
            }
            break;
        case PlayerBotAIType::PBAIT_ARENA:
#ifndef CONVERT_ARENAAI_TOBG
            if (pAI)
            {
                if (dynamic_cast<BotArenaAI*>(pAI) != NULL)
                {
                    player->IsAIEnabled = true;
                    return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            pAI = BotArenaAI::CreateBotArenaAIByPlayerClass(player);
            if (pAI)
            {
                pAI->Reset();
                player->SetAI(pAI);
                player->IsAIEnabled = true;
            }
            break;
#endif
        case PlayerBotAIType::PBAIT_BG:
            if (pAI)
            {
                if (dynamic_cast<BotBGAI*>(pAI) != NULL)
                {
                    player->IsAIEnabled = true;
                    return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            player->SetAI(BotBGAI::CreateBotBGAIByPlayerClass(player));
            player->IsAIEnabled = true;
            break;
        case PlayerBotAIType::PBAIT_DUNGEON:
            if (pAI)
            {
                //if (dynamic_cast<BotFieldAI*>(pAI) != NULL)
                //{
                //	player->IsAIEnabled = true;
                //	return;
                //}
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            break;
    }
}

std::string PlayerBotMgr::GetPlayerLinkText(Player const* player) const
{
    const std::string& name = player->GetName();
    if (player->GetTeamId() == TeamId::TEAM_ALLIANCE)
        return "|cff0000ff|Hplayer:" + name + "|h[" + name + "]|h|r";
    else if (player->GetTeamId() == TeamId::TEAM_HORDE)
        return "|cffff0000|Hplayer:" + name + "|h[" + name + "]|h|r";
    return "|cffffffff|Hplayer:" + name + "|h[" + name + "]|h|r";
}

PlayerBotSession* PlayerBotMgr::GetBotSessionByCharGUID(ObjectGuid& guid)
{
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        PlayerBotBaseInfo* pInfo = itInfo->second;
        if (!(pInfo->ExistCharacterByGUID(guid)))
            continue;
        WorldSession* pWorldSession = sWorld->FindSession(pInfo->id).get();
        if (!pWorldSession)
            return NULL;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        return pSession;
    }
    return NULL;
}

TeamId PlayerBotMgr::GetTeamIDByPlayerBotGUID(ObjectGuid& guid)
{
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        PlayerBotBaseInfo* pInfo = itInfo->second;
        TeamId team = pInfo->GetTeamIDByChar(guid);
        if (team == TeamId::TEAM_NEUTRAL)
            continue;
        return team;
    }
    return TeamId::TEAM_NEUTRAL;
}

bool PlayerBotMgr::IsPlayerBot(WorldSession* pSession)
{
    if (pSession && dynamic_cast<PlayerBotSession*> (pSession))
        return true;
    return false;
}

bool PlayerBotMgr::IsBotAccuntName(std::string name)
{
    if (name.size() < 10)
        return false;
    std::string head = name.substr(0, 9);
    if (head != "playerbot")
        return false;
    std::string numText = name.substr(9);
    int num = atoi(numText.c_str());
    return num > 0;
}

bool PlayerBotMgr::IsIDLEPlayerBot(Player* player)
{
    if (!player || player->isBeingLoaded() || !player->IsInWorld())
        return false;
    if (player->isInCombat() || !player->isAlive())
        return false;
    if (player->InBattleground() || player->InArena() || player->GetMap()->IsDungeon() ||
        player->GetGroup() || player->isUsingLfg())
        return false;
    if (player->InBattlegroundQueue())
        return false;
    if (!player->IsSettingFinish())
        return false;
    if (player->HasAura(26013))
        return false;
    return true;
}

std::set<uint32> PlayerBotMgr::GetArenaTeamPlayerBotIDCountByTeam(TeamId team, int32 count, ArenaTeamTypes type)
{
    std::vector<uint32> allInfoEntrys;
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin(); itInfo != m_idPlayerBotBase.end(); itInfo++)
        allInfoEntrys.push_back(itInfo->first);
    Trinity::Containers::RandomShuffle(allInfoEntrys);
    //unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    //std::shuffle(allInfoEntrys.begin(), allInfoEntrys.end(), std::default_random_engine(seed));

    std::set<uint32> ids;
    if (allInfoEntrys.empty())
        return ids;
    for (uint32 infoEntry : allInfoEntrys)
    {
        PlayerBotBaseInfo* info = GetPlayerBotAccountInfo(infoEntry);
        if (!info)
            continue;
        std::vector<uint32> faildIDs = info->GetNoArenaTeamCharacterIDsByFuction((team == TEAM_ALLIANCE) ? true : false, type);
        if (faildIDs.empty())
            continue;
        for (uint32 id : faildIDs)
        {
            if (ids.find(id) != ids.end())
                continue;
            ids.insert(id);
            break;
        }
        if (ids.size() >= uint32(count))
            return ids;
    }
    ids.clear();
    return ids;
}

PlayerBotBaseInfo* PlayerBotMgr::GetPlayerBotAccountInfo(uint32 guid)
{
    std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.find(guid);
    if (it == m_idPlayerBotBase.end())
        return NULL;
    return it->second;
}

PlayerBotBaseInfo* PlayerBotMgr::GetAccountBotAccountInfo(uint32 guid)
{
    std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idAccountBotBase.find(guid);
    if (it == m_idAccountBotBase.end())
        return NULL;
    return it->second;
}

bool PlayerBotMgr::ExistClassByRace(uint8 race, uint8 prof)
{
    // The combinations used to be a hardcoded table that had not been updated
    // past Wrath: it knew no Monk or Demon Hunter, no Worgen, Goblin or
    // Pandaren, and was missing combinations added in Cataclysm and Mists.
    // playercreateinfo is what the server itself validates character creation
    // against, so ask it instead of keeping a second list in sync.
    return sObjectMgr->GetPlayerInfo(race, prof) != nullptr;
}
void PlayerBotMgr::InitializeCreatePlayerBotName()
{
    allName.clear();
    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ALLRANDOM_NAME);
    PreparedQueryResult result = LoginDatabase.Query(stmt);
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            std::string dbName = fields[0].GetString();
            if (dbName.size() > 0)
                allName.push_back(dbName);
        } while (result->NextRow());
    }

    allArenaName.clear();
    PreparedStatement* stmt2 = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ALLARENA_NAME);
    PreparedQueryResult result2 = LoginDatabase.Query(stmt2);
    if (result2)
    {
        do
        {
            Field* fields = result2->Fetch();
            std::string dbName = fields[0].GetString();
            if (dbName.size() > 0)
                allArenaName.push_back(dbName);
        } while (result2->NextRow());
    }
}
std::string PlayerBotMgr::RandomName(LocaleConstant locale)
{
    // A name is only handed out when the character create handler would
    // accept it: the same name rules, the reserved list, and not yet taken -
    // neither by a saved character nor by a creation still in flight.
    auto usable = [this, locale](std::string const& name) -> bool
    {
        if (m_HandedOutNames.find(name) != m_HandedOutNames.end())
            return false;
        if (sWorld->GetCharacterInfo(name))
            return false;
        if (sCharacterDataStore->CheckPlayerName(name, locale, true) != CHAR_NAME_SUCCESS)
            return false;
        if (sCharacterDataStore->IsReservedName(name))
            return false;
        return true;
    };

    if (allName.empty())
        InitializeCreatePlayerBotName();

    // The name pool in auth.playerbot_names comes first.
    std::vector<std::string> pool(allName);
    Trinity::Containers::RandomShuffle(pool);
    for (std::string& name : pool)
    {
        if (!normalizePlayerName(name))
            continue;
        if (usable(name))
        {
            m_HandedOutNames.insert(name);
            return name;
        }
    }

    // The pool is small, so make up further names from syllables. Lower case
    // after the first letter, as normalizePlayerName would leave them.
    static char const* const starts[] =
    {
        "Ael", "Bar", "Bel", "Cal", "Cor", "Dor", "Dra", "Eir", "Ela", "Fal",
        "Fen", "Gal", "Gor", "Hal", "Har", "Ith", "Jor", "Kar", "Kel", "Lia",
        "Lor", "Mal", "Mor", "Nor", "Nyl", "Ory", "Per", "Quel", "Ral", "Ros",
        "Sar", "Syl", "Tal", "Tor", "Ul", "Val", "Vor", "Wyn", "Yor", "Zan"
    };
    static char const* const middles[] =
    {
        "", "", "", "a", "e", "i", "o", "an", "en", "ar", "or", "il", "el", "ri", "na", "da", "ve"
    };
    static char const* const ends[] =
    {
        "dan", "dor", "ric", "wyn", "mir", "las", "ion", "ia", "ra", "eth", "an", "is",
        "os", "ius", "ara", "ena", "rin", "th", "mar", "nor", "vin", "lyn", "dis", "rel"
    };

    for (uint32 attempt = 0; attempt < 500; ++attempt)
    {
        std::string name = starts[urand(0, std::extent<decltype(starts)>::value - 1)];
        name += middles[urand(0, std::extent<decltype(middles)>::value - 1)];
        name += ends[urand(0, std::extent<decltype(ends)>::value - 1)];
        if (usable(name))
        {
            m_HandedOutNames.insert(name);
            return name;
        }
    }

    return "";
}
std::string PlayerBotMgr::RandomArenaName()
{
    return "Arena" + std::to_string(urand(0, 100));
}
uint8 PlayerBotMgr::RandomRace(bool alliance, uint8 prof)
{
    // Playable races per faction as of Legion. The previous version drew from
    // 1..11 only, so Worgen, Goblin and Pandaren never appeared, and when no
    // race matched - which was always the case for Monk and Demon Hunter - it
    // fell back to Human or Orc and produced an invalid combination.
    //
    // Pandaren are left out: a client never creates them with a faction, they
    // start neutral and pick one on the Wandering Isle, so the faction races
    // are not something the character create handler can be relied on for.
    static uint8 const allianceRaces[] =
    {
        RACE_HUMAN, RACE_DWARF, RACE_NIGHTELF, RACE_GNOME, RACE_DRAENEI,
        RACE_WORGEN
    };
    static uint8 const hordeRaces[] =
    {
        RACE_ORC, RACE_UNDEAD_PLAYER, RACE_TAUREN, RACE_TROLL, RACE_GOBLIN,
        RACE_BLOODELF
    };

    uint8 const* races = alliance ? allianceRaces : hordeRaces;
    std::size_t const raceCount = alliance ? std::extent<decltype(allianceRaces)>::value
                                           : std::extent<decltype(hordeRaces)>::value;

    // Collect what actually exists for this class rather than sampling blindly.
    std::vector<uint8> candidates;
    candidates.reserve(raceCount);
    // The create handler refuses a race without an unlock requirement entry.
    for (std::size_t i = 0; i < raceCount; ++i)
        if (ExistClassByRace(races[i], prof) && sObjectMgr->GetRaceUnlockRequirement(races[i]))
            candidates.push_back(races[i]);

    if (!candidates.empty())
        return candidates[urand(0, uint32(candidates.size() - 1))];

    TC_LOG_ERROR(LOG_FILTER_SERVER_LOADING, "PlayerBotMgr::RandomRace found no %s race for class %u.",
        alliance ? "alliance" : "horde", uint32(prof));
    return 0;
}
uint8 PlayerBotMgr::RandomSkinColor(uint8 race, uint8 gender, uint8 prof)
{
    uint8 skinColor = 0;
    for (int i = 0; i < 80; i++)
    {
        skinColor = irand(0, 9);
        CharSectionsEntry const* entry = sDB2Manager.GetCharSectionEntry(race, gender, CharBaseSectionVariation::Skin, 0, skinColor);
        if (entry)
        {
            if ((entry->Flags & SECTION_FLAG_DEATH_KNIGHT) && prof != CLASS_DEATH_KNIGHT)
                continue;
            if (!(entry->Flags & SECTION_FLAG_PLAYER))
                continue;
            return skinColor;
        }
    }
    TC_LOG_INFO(LOG_FILTER_PLAYER, "server.loading", ">> Random player custom SkinColor timeout!");
    return 1;
}
uint8 PlayerBotMgr::RandomFace(uint8 race, uint8 gender, uint8 skinColor, uint8 prof)
{
    uint8 faceID = 0;
    for (int i = 0; i < 80; i++)
    {
        faceID = irand(0, 9);
        CharSectionsEntry const* entry = sDB2Manager.GetCharSectionEntry(race, gender, CharBaseSectionVariation::Face, faceID, skinColor);
        if (entry)
        {
            if ((entry->Flags & SECTION_FLAG_DEATH_KNIGHT) && prof != CLASS_DEATH_KNIGHT)
                continue;
            if (!(entry->Flags & SECTION_FLAG_PLAYER))
                continue;
            return faceID;
        }
    }
    TC_LOG_INFO(LOG_FILTER_PLAYER, "server.loading", ">> Random player custom Face timeout!");
    return 1;
}
uint8 PlayerBotMgr::RandomHair(uint8 race, uint8 gender, uint8 prof)
{
    uint8 hairID = 0;
    for (uint8 hc = 0; hc < 80; hc++)
    {
        for (int i = 0; i < 80; i++)
        {
            hairID = irand(0, 8);
            CharSectionsEntry const* entry = sDB2Manager.GetCharSectionEntry(race, gender, CharBaseSectionVariation::Hair, hairID, hc);
            if (entry)
            {
                if ((entry->Flags & SECTION_FLAG_DEATH_KNIGHT) && prof != CLASS_DEATH_KNIGHT)
                    continue;
                if (!(entry->Flags & SECTION_FLAG_PLAYER))
                    continue;
                return hairID;
            }
        }
    }
    TC_LOG_INFO(LOG_FILTER_PLAYER, "server.loading", ">> Random player custom Hair timeout!");
    return 0;
}
uint8 PlayerBotMgr::RandomHairColor(uint8 race, uint8 gender, uint8 hairID, uint8 prof)
{
    uint8 hairColor = 0;
    for (int i = 0; i < 80; i++)
    {
        hairColor = irand(0, 8);
        CharSectionsEntry const* entry = sDB2Manager.GetCharSectionEntry(race, gender, CharBaseSectionVariation::Hair, hairID, hairColor);
        if (entry)
        {
            if ((entry->Flags & SECTION_FLAG_DEATH_KNIGHT) && prof != CLASS_DEATH_KNIGHT)
                continue;
            if (!(entry->Flags & SECTION_FLAG_PLAYER))
                continue;
            return hairColor;
        }
    }
    TC_LOG_INFO(LOG_FILTER_PLAYER, "server.loading", ">> Random player custom HairColor timeout!");
    return 1;
}
uint8 PlayerBotMgr::RandomFacialHair(uint8 race, uint8 gender, uint8 hairColor, uint8 prof)
{
    uint8 facialHair = 0;
    for (int i = 0; i < 80; i++)
    {
        facialHair = irand(0, 7);
        CharSectionsEntry const* entry = sDB2Manager.GetCharSectionEntry(race, gender, CharBaseSectionVariation::FacialHair, facialHair, hairColor);
        if (entry)
        {
            if ((entry->Flags & SECTION_FLAG_DEATH_KNIGHT) && prof != CLASS_DEATH_KNIGHT)
                continue;
            if (!(entry->Flags & SECTION_FLAG_PLAYER))
                continue;
            return facialHair;
        }
    }
    TC_LOG_INFO(LOG_FILTER_PLAYER, "server.loading", ">> Random player custom FacialHair timeout!");
    return 0;
}

std::shared_ptr<WorldPackets::Character::CharacterCreateInfo> PlayerBotMgr::BuildCreatePlayerData(bool alliance, uint8 prof, LocaleConstant locale)
{
    uint8 race = RandomRace(alliance, prof);
    if (!race)
        return nullptr;

    std::string name = RandomName(locale);
    if (name.empty())
    {
        TC_LOG_ERROR(LOG_FILTER_SERVER_LOADING, "PlayerBotMgr: found no free name for a new bot character.");
        return nullptr;
    }

    auto createInfo = std::make_shared<WorldPackets::Character::CharacterCreateInfo>();
    createInfo->Race = race;
    createInfo->Class = prof;
    createInfo->Sex = uint8(urand(GENDER_MALE, GENDER_FEMALE));
    createInfo->Skin = RandomSkinColor(race, createInfo->Sex, prof);
    createInfo->Face = RandomFace(race, createInfo->Sex, createInfo->Skin, prof);
    createInfo->HairStyle = RandomHair(race, createInfo->Sex, prof);
    createInfo->HairColor = RandomHairColor(race, createInfo->Sex, createInfo->HairStyle, prof);
    createInfo->FacialHairStyle = RandomFacialHair(race, createInfo->Sex, createInfo->HairColor, prof);
    createInfo->OutfitId = 0;
    createInfo->CustomDisplay.fill(0);
    createInfo->Name = name;
    return createInfo;
}

// Creates the next queued bot character, if any. Called from Update(), so at
// most one creation starts per manager tick. The bot session goes through
// WorldSession::HandleCharCreateOpcode exactly like a client would; on success
// the handler reports back through OnPlayerBotCreate.
void PlayerBotMgr::CreateOncePlayerBot()
{
    // Requests whose session is busy go to the back; look at each one at
    // most once per tick.
    size_t remaining = m_CreateRequests.size();
    while (remaining-- > 0)
    {
        BotCreateRequest request = m_CreateRequests.front();
        m_CreateRequests.pop();

        PlayerBotBaseInfo* pInfo = GetPlayerBotAccountInfo(request.account);
        if (!pInfo || pInfo->ExistClass(request.alliance, request.cls))
            continue;

        // No bot session: a real client logged in on this account and
        // replaced it. Drop the request; the next start queues it again.
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(sWorld->FindSession(request.account).get());
        if (!pSession)
            continue;

        if (pSession->PlayerLoading() || pSession->GetPlayer())
        {
            m_CreateRequests.push(request);
            continue;
        }

        std::shared_ptr<WorldPackets::Character::CharacterCreateInfo> createInfo = BuildCreatePlayerData(request.alliance, request.cls, pSession->GetSessionDbcLocale());
        if (!createInfo)
            continue;

        WorldPacket packet(CMSG_CREATE_CHARACTER);
        WorldPackets::Character::CreateChar charCreate(std::move(packet));
        charCreate.CreateInfo = createInfo;
        pSession->HandleCharCreateOpcode(charCreate);
        ++m_CreateIssued;
        break;
    }

    if (m_CreateIssued && m_CreateRequests.empty())
    {
        TC_LOG_INFO(LOG_FILTER_SERVER_LOADING, "PlayerBotMgr: finished creating bot characters, %u requests issued.", m_CreateIssued);
        m_CreateIssued = 0;
    }
}

void PlayerBotMgr::ClearBaseInfo()
{
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.begin();
        it != m_idPlayerBotBase.end();
        it++)
    {
        delete it->second;
    }
    m_idPlayerBotBase.clear();
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idAccountBotBase.begin();
        it != m_idAccountBotBase.end();
        it++)
    {
        delete it->second;
    }
    m_idAccountBotBase.clear();
}

void PlayerBotMgr::UpdateLastAccountIndex(std::string& username)
{
    //std::unique_lock<std::mutex> sessionGuard(PlayerBotMgr::g_uniqueLock);
    if (username.empty())
        return;
    std::string querySql = "SELECT id FROM account WHERE username='" + username + "'";
    QueryResult result = LoginDatabase.Query(querySql.c_str());
    if (result)
    {
        Field* fields = result->Fetch();
        if (fields)
        {
            uint32 id = fields[0].GetUInt32();
            m_LastBotAccountIndex = id;
        }
    }
}

void PlayerBotMgr::SupplementAccount()
{
    // Unsigned: with more bot accounts than wanted the subtraction used to wrap
    // around and the loop below tried to create billions of accounts.
    if (m_idPlayerBotBase.size() >= m_BotAccountAmount)
        return;
    uint32 needAccount = m_BotAccountAmount - m_idPlayerBotBase.size();

    for (uint32 i = 0; i < needAccount; i++)
    {
        ++m_LastBotAccountIndex;
        char indexText[25] ={ 0 };
        snprintf(indexText, sizeof(indexText), "%u", m_LastBotAccountIndex);
        std::string userName = "playerbot";
        userName += indexText;
        // Synchronous: the row is read back right below. Created asynchronously
        // it usually was not there yet, so on the first start the new accounts
        // were missing from the bot list until the next restart.
        if (AccountMgr::CreateAccount(userName, "botxxx", false) == AccountOpResult::AOR_OK)
        {
            std::string querySql = "SELECT id, username, sha_pass_hash FROM account WHERE username='";
            querySql += userName + "'";
            QueryResult result = LoginDatabase.Query(querySql.c_str());
            if (result)
            {
                Field* fields = result->Fetch();
                if (fields)
                {
                    uint32 id = fields[0].GetUInt32();
                    std::string username = fields[1].GetString();
                    std::string pass = fields[2].GetString();

                    std::string lowerName = boost::algorithm::to_lower_copy(username);
                    if (m_idPlayerBotBase.find(id) == m_idPlayerBotBase.end())
                    {
                        PlayerBotBaseInfo* pInfo = new PlayerBotBaseInfo(id, username.c_str(), pass, false);
                        m_idPlayerBotBase[id] = pInfo;
                    }
                    m_LastBotAccountIndex = id;
                }
            }
        }
    }
}

void PlayerBotMgr::DestroyBotMail(uint32 guid)
{
    char sql[256] ={ 0 };
    snprintf(sql, sizeof(sql), "DELETE FROM mail WHERE receiver = %d", guid);
    CharacterDatabase.Execute(sql);
    //memset(sql, 0, 256);
    //snprintf(sql, sizeof(sql), "DELETE FROM mail_items WHERE receiver = %d", guid);
    //CharacterDatabase.Execute(sql);
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEMS);
    stmt->setUInt32(0, guid);
    CharacterDatabase.Execute(stmt);
}

void PlayerBotMgr::AddNewAccountBotBaseInfo(std::string name)
{
    std::string upperName = boost::algorithm::to_upper_copy(name);
    std::string sql("SELECT id, username, sha_pass_hash FROM account WHERE `username`='"); sql += upperName + "'";
    QueryResult result = LoginDatabase.Query(sql.c_str());
    if (!result)
        return;
    Field* fields = result->Fetch();
    uint32 id = fields[0].GetUInt32();
    std::string username = fields[1].GetString();
    std::string pass = fields[2].GetString();
    if (m_idAccountBotBase.find(id) == m_idAccountBotBase.end())
    {
        PlayerBotBaseInfo* pInfo = new PlayerBotBaseInfo(id, username.c_str(), pass, true);
        m_idAccountBotBase[id] = pInfo;
    }
}

void PlayerBotMgr::LoadPlayerBotBaseInfo()
{
    uint32 oldMSTime = getMSTime();

    // The online limit used to be fixed at construction time. Pick it up from
    // the configuration here, which runs after the config has been read.
    m_MaxOnlineBot = int32(sWorld->getIntConfig(CONFIG_PLAYERBOT_MAX_ONLINE));

    // Read here rather than in World::LoadConfigSettings so that adding them
    // does not touch World.h, which nearly every file includes.
    int32 accounts = sConfigMgr->GetIntDefault("PlayerBot.Accounts", 180);
    if (accounts < 0 || accounts > 1000)
    {
        TC_LOG_ERROR(LOG_FILTER_SERVER_LOADING, "PlayerBot.Accounts (%i) must be between 0 and 1000. Using 180 instead.", accounts);
        accounts = 180;
    }
    m_BotAccountAmount = uint32(accounts);

    m_MinOnlineBot = sConfigMgr->GetIntDefault("PlayerBot.MinOnline", 0);
    if (m_MinOnlineBot < 0 || m_MinOnlineBot > 500)
    {
        TC_LOG_ERROR(LOG_FILTER_SERVER_LOADING, "PlayerBot.MinOnline (%i) must be between 0 and 500. Using 0 instead.", m_MinOnlineBot);
        m_MinOnlineBot = 0;
    }
    if (m_MinOnlineBot > m_MaxOnlineBot)
        TC_LOG_ERROR(LOG_FILTER_SERVER_LOADING, "PlayerBot.MinOnline (%i) is above PlayerBot.MaxOnline (%i); at most %i bots will be kept online.",
            m_MinOnlineBot, m_MaxOnlineBot, m_MaxOnlineBot);
    m_MinOnlineWarned = false;

    m_Announce = sConfigMgr->GetBoolDefault("PlayerBot.Announce", true);

    ClearBaseInfo();
    // Ordered, so that PlayerBot.Accounts always selects the same accounts.
    // An empty account table is no reason to stop: on a fresh auth database
    // the bot accounts below still have to be created.
    QueryResult result = LoginDatabase.Query("SELECT id, username, sha_pass_hash FROM account ORDER BY id");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 id = fields[0].GetUInt32();
            std::string username = fields[1].GetString();
            std::string pass = fields[2].GetString();
            sOnlineMgr->AddNewAccount(id, username); // Real player acc and bot acc all in

            std::string lowerName = boost::algorithm::to_lower_copy(username);
            if (IsBotAccuntName(lowerName))
            {
                // Bot accounts beyond PlayerBot.Accounts stay in the database but
                // are not used. Their index still counts, so a new account never
                // reuses one of their names.
                if (m_idPlayerBotBase.size() < m_BotAccountAmount && m_idPlayerBotBase.find(id) == m_idPlayerBotBase.end())
                {
                    PlayerBotBaseInfo* pInfo = new PlayerBotBaseInfo(id, username.c_str(), pass, false);
                    m_idPlayerBotBase[id] = pInfo;
                }
                m_LastBotAccountIndex = id;
            }
            else
            {
                if (m_idAccountBotBase.find(id) == m_idAccountBotBase.end())
                {
                    PlayerBotBaseInfo* pInfo = new PlayerBotBaseInfo(id, username.c_str(), pass, true);
                    m_idAccountBotBase[id] = pInfo;
                }
            }
        } while (result->NextRow());
    }

    if (sWorld->getBoolConfig(CONFIG_PLAYERBOT_ENABLE))
        SupplementAccount();

    if (m_idPlayerBotBase.size() > 0 || m_idAccountBotBase.size() > 0)
        LoadCharBaseInfo();

    TC_LOG_INFO(LOG_FILTER_SERVER_LOADING, ">> Loaded %u bot accounts (PlayerBot.Accounts %u) in %u ms",
        uint32(m_idPlayerBotBase.size()), m_BotAccountAmount, GetMSTimeDiffToNow(oldMSTime));
}

void PlayerBotMgr::LoadCharBaseInfo()
{
    QueryResult result = CharacterDatabase.Query("SELECT guid, account, name, race, class, gender, level FROM characters");

    if (!result)
    {
        TC_LOG_INFO(LOG_FILTER_PLAYER, "server.loading", ">> LoadPlayerBot Find 0 characters!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint64 guid = fields[0].GetUInt64();
        uint32 accID = fields[1].GetUInt32();
        PlayerBotBaseInfo* pInfo = GetPlayerBotAccountInfo(accID);
        if (pInfo)
        {
            if (pInfo->characters.find(guid) == pInfo->characters.end())
            {
                std::string charName = fields[2].GetString();
                uint16 race = fields[3].GetInt16();
                uint16 pro = fields[4].GetInt16();
                uint16 gender = fields[5].GetInt16();
                uint16 level = fields[6].GetInt16();
                pInfo->characters.emplace(guid, PlayerBotCharBaseInfo{ guid, accID, charName, race, pro, gender, level });
                DestroyBotMail(guid);
            }
        }
        else if (pInfo = GetAccountBotAccountInfo(accID))
        {
            if (pInfo->characters.find(guid) == pInfo->characters.end())
            {
                std::string charName = fields[2].GetString();
                uint16 race = fields[3].GetInt16();
                uint16 pro = fields[4].GetInt16();
                uint16 gender = fields[5].GetInt16();
                uint16 level = fields[6].GetInt16();
                pInfo->characters.emplace(guid, PlayerBotCharBaseInfo{ guid, accID, charName, race, pro, gender, level });
                //DestroyBotMail(guid);
            }
        }
    } while (result->NextRow());
}

void PlayerBotMgr::UpAllPlayerBotSession()
{
    PlayerBotSetting::Initialize();
    if (!sWorld->getBoolConfig(CONFIG_PLAYERBOT_ENABLE))
        return;
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.begin();
        it != m_idPlayerBotBase.end();
        it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        UpPlayerBotSessionByBaseInfo(pInfo, false);
    }
    SupplementPlayerBot();
}

PlayerBotSession* PlayerBotMgr::UpPlayerBotSessionByBaseInfo(PlayerBotBaseInfo* pAcc, bool accountInfo)
{
    if (!pAcc)
        return NULL;
    auto pSession = sWorld->FindSession(pAcc->id).get();
    if (pSession)
        return NULL;
    std::string name = pAcc->username.c_str();
    auto pBotSession = std::shared_ptr<PlayerBotSession>(new PlayerBotSession{ pAcc->id, name, AccountTypes::SEC_GAMEMASTER, 2, 0, LocaleConstant::LOCALE_zhTW, 0, false });
    //pbSession->ReadAddonsInfo("");
    if (accountInfo)
        pBotSession->SetAccountBotSession();

    sWorld->AddSession(pBotSession);
    return pBotSession.get();
}

void PlayerBotMgr::OnPlayerBotCreate(ObjectGuid const& guid, uint32 accountId, std::string const& name, uint8 gender, uint8 race, uint8 playerClass, uint8 level)
{
    PlayerBotBaseInfo* pInfo = GetPlayerBotAccountInfo(accountId);
    if (!pInfo)
        return;
    uint32 id = guid.GetGUIDLow();
    if (pInfo->characters.find(id) != pInfo->characters.end())
    {
        return;
    }
    pInfo->characters[id] = PlayerBotCharBaseInfo(id, accountId, name, uint16(race), uint16(playerClass), uint16(gender), uint16(level));
    m_HandedOutNames.erase(name);
}

void PlayerBotMgr::OnAccountBotCreate(ObjectGuid const& guid, uint32 accountId, std::string const& name, uint8 gender, uint8 race, uint8 playerClass, uint8 level)
{
    PlayerBotBaseInfo* pInfo = GetAccountBotAccountInfo(accountId);
    if (!pInfo)
        return;
    uint32 id = guid.GetGUIDLow();
    if (pInfo->characters.find(id) != pInfo->characters.end())
    {
        return;
    }
    pInfo->characters[id] = PlayerBotCharBaseInfo(id, accountId, name, uint16(race), uint16(playerClass), uint16(gender), uint16(level));
}

void PlayerBotMgr::OnAccountBotDelete(ObjectGuid& guid, uint32 accountId)
{
    PlayerBotBaseInfo* pInfo = GetAccountBotAccountInfo(accountId);
    if (!pInfo)
        return;
    pInfo->RemoveCharacterByGUID(guid);
}

void PlayerBotMgr::OnPlayerBotLogin(WorldSession* pSession, Player* pPlayer)
{
    ++m_BotOnlineCount;
    Group* pGroup = pPlayer->GetGroup();
    if (pGroup)
    {
        if (!pGroup->GroupExistRealPlayer())
        {
            pPlayer->RemoveFromGroup(RemoveMethod::GROUP_REMOVEMETHOD_LEAVE);
        }
        else
            SwitchPlayerBotAI(pPlayer, PlayerBotAIType::PBAIT_GROUP, false);
    }
    else
        SwitchPlayerBotAI(pPlayer, PlayerBotAIType::PBAIT_FIELD, false);

    if (pPlayer->GetBattleground())
    {
        pPlayer->LeaveBattleground();
        if (pSession)
            pSession->HandleWorldPortAck();
    }

    BotUtility::RemoveArenaBotSpellsByPlayer(pPlayer);
    sPlayerBotTalkMgr->JoinDefaultChannel(pPlayer);
    if (pSession && m_Announce)
    {
        std::string outString;
        outString = " has come online.";
        sWorld->SendGlobalText((GetPlayerLinkText(pPlayer) + outString).c_str(), NULL);
    }
    if (PlayerBotSession* pBotSession = dynamic_cast<PlayerBotSession*>(pSession))
    {
        if (!pSession->HasSchedules() && !pPlayer->EquipIsTidiness())
        {
            uint32 curTType = pPlayer->FindTalentType();
            uint32 newTType = curTType;
            while (newTType == curTType)
                newTType = urand(0, 2);
            BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
            schedule2.parameter1 = pPlayer->getLevel();
            schedule2.parameter2 = pPlayer->getLevel();
            schedule2.parameter3 = newTType + 1;
            pBotSession->PushScheduleToQueue(schedule2);
        }
    }
}

void PlayerBotMgr::OnPlayerBotLogout(WorldSession* pSession)
{
    --m_BotOnlineCount;
    if (m_BotOnlineCount < 0) m_BotOnlineCount = 0;

    if (m_Announce)
    {
        std::string outString;
        outString = "A bot has gone offline.";
        sWorld->SendGlobalText(outString.c_str(), NULL);
    }
    PlayerBotSession* pBotSession = dynamic_cast<PlayerBotSession*>(pSession);
    if (pBotSession && !pBotSession->HasScheduleByType(BotGlobleScheduleType::BGSType_Online) &&
        !pBotSession->HasScheduleByType(BotGlobleScheduleType::BGSType_Online_GUID))
        pBotSession->ClearAllSchedule();
}

void PlayerBotMgr::OnPlayerBotLeaveOriginalGroup(Player* pPlayer)
{
    SwitchPlayerBotAI(pPlayer, PlayerBotAIType::PBAIT_FIELD, true);
}

void PlayerBotMgr::LoginGroupBotByPlayer(Player* pPlayer)
{
#ifndef INCOMPLETE_BOT
    if (pPlayer->IsPlayerBot() || !sWorld->getBoolConfig(CONFIG_PLAYERBOT_ENABLE))
        return;
    int32 isok = sConfigMgr->GetIntDefault("pbot", 1);
    Group* pGroup = pPlayer->GetGroup();
    if (!pGroup || pGroup->isBFGroup() || pGroup->isBGGroup())
        return;
    Group::MemberSlotList const& memList = pGroup->GetMemberSlots();
    for (Group::MemberSlot const& slot : memList)
    {
        ObjectGuid guid = slot.Guid;
        if (guid == pPlayer->GetGUID())
            continue;
        Player* friendPlayer = ObjectAccessor::FindPlayer(guid);
        if (friendPlayer)
            continue;
        if (isok==0)
            AddNewPlayerBotByGUID2(guid);
        else
            DelayLoginPlayerBotByGUID(guid);
    }
#endif
}

void PlayerBotMgr::LoginFriendBotByPlayer(Player* pPlayer)
{
    //#ifndef INCOMPLETE_BOT
    //	if (pPlayer->IsPlayerBot())
    //		return;
    //	int32 isok = sConfigMgr->GetIntDefault("pbot", 1);
    //
    //	std::vector<ObjectGuid> friends = pPlayer->GetSocial()->Get();
    //	for (ObjectGuid& guid : friends)
    //	{
    //
    //	if (isok==0)
    //		AddNewPlayerBotByGUID2(guid);
    //		else
    //		DelayLoginPlayerBotByGUID(guid);
    //	}
    //#else
    //	std::string allonlineText;
    //	consoleToUtf8(std::string("|cffff8800Bots cannot be summoned; all accounts are already online.|r"), allonlineText);
    //	sWorld->SendGlobalText(allonlineText.c_str(), NULL);
    //#endif
}

void PlayerBotMgr::LogoutAllGroupPlayerBot(Group* pGroup, bool force)
{
    if (!pGroup)
        return;
    if (!force && pGroup->GroupExistRealPlayer())
        return;
    Group::MemberSlotList const& memList = pGroup->GetMemberSlots();
    for (Group::MemberSlot const& slot : memList)
    {
        ObjectGuid guid = slot.Guid;
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player || !player->IsPlayerBot())
            continue;
        WorldSession* pSession = player->GetSession();
        if (pSession)
            pSession->LogoutPlayer(false, "bot");
    }
}

bool PlayerBotMgr::AllPlayerLeaveBG(uint32 account)
{
    if (account == 0)
        return false;
    //std::unique_lock<std::mutex> sessionGuard(PlayerBotMgr::g_uniqueLock);
    WorldSession* pSession = sWorld->FindSession(account).get();
    if (pSession && pSession->IsBotSession() && !pSession->PlayerLoading())
    {
        Player* pPlayer = pSession->GetPlayer();
        if (!pPlayer || pPlayer->GetTransport() || !pPlayer->InBattleground())
            return false;
        const Battleground* bg = pPlayer->GetBattleground();
        if (!bg || bg->GetStatus() == BattlegroundStatus::STATUS_WAIT_LEAVE || bg->GetStatus() == BattlegroundStatus::STATUS_WAIT_JOIN)
            return false;
        ((Battleground*)bg)->EndBattleground((pPlayer->GetTeamId() == TEAM_ALLIANCE) ? HORDE : ALLIANCE);
        return true;
    }
    return false;
}

void PlayerBotMgr::AllPlayerBotRandomLogin(const char* name)
{
    int32 needOnline = m_MaxOnlineBot - m_BotOnlineCount;
    if (needOnline <= 0)
        return;
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.begin();
        it != m_idPlayerBotBase.end();
        it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        WorldSession* pSession = sWorld->FindSession(pInfo->id).get();
        if (pSession && pSession->IsBotSession() && !pSession->PlayerLoading() && !pSession->GetPlayer())
        {
            if (pInfo->characters.size() <= 0)
                continue;

            if (name[0] != '\0')
            {
                // characters is keyed by guid, not by position. Indexing it
                // with a counter inserted an empty entry for every guid below
                // the account's highest one, and the random login below then
                // picked those far more often than real characters.
                for (auto itChar = pInfo->characters.begin(); itChar != pInfo->characters.end(); ++itChar)
                {
                    if (strcmp(name, itChar->second.name.c_str()) == 0)
                    {
                        PlayerBotCharBaseInfo& charInfo = itChar->second;
                        WorldPacket _worldPacket(CMSG_PLAYER_LOGIN);
                        WorldPackets::Character::PlayerLogin cmd(std::move(_worldPacket));
                        cmd.Guid = ObjectGuid::Create<HighGuid::Player>(charInfo.guid);
                        cmd.FarClip = 0.0f;
                        pSession->HandlePlayerLoginOpcode(cmd);
                        pSession->HandleContinuePlayerLogin();
                        return;
                    }
                }
            }
            else
            {
                int sel = irand(0, pInfo->characters.size() - 1);
                for (auto itChar = pInfo->characters.begin();
                    itChar != pInfo->characters.end();
                    itChar++)
                {
                    if (sel <= 0)
                    {
                        PlayerBotCharBaseInfo& charInfo = itChar->second;
                        WorldPacket _worldPacket(CMSG_PLAYER_LOGIN);
                        WorldPackets::Character::PlayerLogin cmd(std::move(_worldPacket));
                        cmd.Guid = ObjectGuid::Create<HighGuid::Player>(charInfo.guid);
                        cmd.FarClip = 0.0f;
                        pSession->HandlePlayerLoginOpcode(cmd);
                        pSession->HandleContinuePlayerLogin();
                        --needOnline;
                        break;
                    }
                    --sel;
                }
            }
            if (needOnline <= 0)
                return;
        }
    }
}

void PlayerBotMgr::AllPlayerBotLogout()
{
    //std::unique_lock<std::mutex> sessionGuard(PlayerBotMgr::g_uniqueLock);
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.begin();
        it != m_idPlayerBotBase.end();
        it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        PlayerBotLogout(pInfo->id);
    }
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idAccountBotBase.begin();
        it != m_idAccountBotBase.end();
        it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        PlayerBotLogout(pInfo->id);
    }
}

bool PlayerBotMgr::PlayerBotLogout(uint32 account)
{
    if (account == 0)
        return false;
    //std::unique_lock<std::mutex> sessionGuard(PlayerBotMgr::g_uniqueLock);
    WorldSession* pSession = sWorld->FindSession(account).get();
    if (pSession && pSession->IsBotSession() && !pSession->PlayerLoading())
    {
        Player* pBot = pSession->GetPlayer();
        if (!pBot || !pBot->IsPlayerBot() || !pBot->IsSettingFinish())
            return false;
        PlayerBotSession* pBotSession = dynamic_cast<PlayerBotSession*>(pSession);
        if (pBotSession)
        {
            if (pBotSession->HasSchedules())
                pBotSession->ClearAllSchedule();

            pSession->LogoutPlayer(false, "bot");
            return true;
        }
    }
    return false;
}

// Queues the bot characters that should exist but do not yet. The first
// PlayerBot.CreateCharacters.Accounts bot accounts each get one character of
// every class, all of one faction: an account keeps the faction of the
// characters it already has, empty accounts are split evenly between the two
// sides. The requests run class by class across all accounts, so that early
// on many accounts have something to log in with, and CreateOncePlayerBot
// works them off one per manager tick.
void PlayerBotMgr::SupplementPlayerBot()
{
    std::queue<BotCreateRequest>().swap(m_CreateRequests);
    m_CreateIssued = 0;

    int32 accountLimit = sConfigMgr->GetIntDefault("PlayerBot.CreateCharacters.Accounts", 20);
    if (accountLimit <= 0)
        return;

    struct Target
    {
        PlayerBotBaseInfo* info;
        bool alliance;
        uint32 planned;
    };
    std::vector<Target> targets;
    uint32 allianceAccounts = 0;
    uint32 hordeAccounts = 0;

    // m_idPlayerBotBase is ordered by account id, so this is stable across restarts.
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end() && targets.size() < uint32(accountLimit);
        ++itInfo)
    {
        PlayerBotBaseInfo* pInfo = itInfo->second;
        uint32 allianceChars = 0;
        uint32 hordeChars = 0;
        for (auto const& entry : pInfo->characters)
        {
            if (pInfo->MatchRaceByFuction(true, entry.second.race))
                ++allianceChars;
            else if (pInfo->MatchRaceByFuction(false, entry.second.race))
                ++hordeChars;
        }

        bool alliance;
        if (allianceChars || hordeChars)
            alliance = allianceChars >= hordeChars;
        else
            alliance = allianceAccounts <= hordeAccounts;

        if (alliance)
            ++allianceAccounts;
        else
            ++hordeAccounts;
        targets.push_back({ pInfo, alliance, 0 });
    }

    uint32 const perAccount = sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM);
    for (uint8 cls = CLASS_WARRIOR; cls <= CLASS_DEMON_HUNTER; ++cls)
    {
        if (!sChrClassesStore.LookupEntry(cls))
            continue;
        for (Target& target : targets)
        {
            if (target.info->ExistClass(target.alliance, cls))
                continue;
            if (target.info->characters.size() + target.planned >= perAccount)
                continue;
            m_CreateRequests.push({ target.info->id, target.alliance, cls });
            ++target.planned;
        }
    }

    if (!m_CreateRequests.empty())
        TC_LOG_INFO(LOG_FILTER_SERVER_LOADING, "PlayerBotMgr: queued %u bot characters for creation on %u accounts (%u alliance, %u horde).",
            uint32(m_CreateRequests.size()), uint32(targets.size()), allianceAccounts, hordeAccounts);
}

void PlayerBotMgr::OnRealPlayerJoinBattlegroundQueue(uint32 bgTypeId, uint32 level)
{
}

void PlayerBotMgr::OnRealPlayerLeaveBattlegroundQueue(uint32 bgTypeId, uint32 level)
{
    uint8 bgQueueTypeId = MS::Battlegrounds::GetBgQueueTypeIdByBgTypeID(bgTypeId, 0);
    BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BattlegroundTypeId(bgTypeId));
    if (!bg)
        return;
    PVPDifficultyEntry const* bracketEntry = sDB2Manager.GetBattlegroundBracketByLevel(bg->GetMapId(), level);
    if (!bracketEntry)
        return;

    if (bgQueue.ExistRealPlayer(bracketEntry))
        return;
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
        if (!pSession)
            continue;
        Player* player = pSession->GetPlayer();
        if (!player || player->isBeingLoaded())
            continue;
        if (player->InBattleground() || player->InArena() || !player->IsInWorld())
            continue;
        uint32 lv = player->getLevel();
        if (lv < bracketEntry->MinLevel || lv > bracketEntry->MaxLevel)
            continue;
        if (player->InBattlegroundQueue())
        {
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_OutBGQueue, 0);
            schedule1.parameter1 = bgTypeId;
            pSession->PushScheduleToQueue(schedule1);
        }
    }
}

void PlayerBotMgr::OnRealPlayerLeaveArenaQueue(uint32 bgTypeId, uint32 level, uint32 aaType)
{
    uint8 bgQueueTypeId = MS::Battlegrounds::GetBgQueueTypeIdByBgTypeID(bgTypeId, aaType);
    BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BattlegroundTypeId(bgTypeId));
    if (!bg)
        return;
    PVPDifficultyEntry const* bracketEntry = sDB2Manager.GetBattlegroundBracketByLevel(bg->GetMapId(), level);
    if (!bracketEntry)
        return;

    if (bgQueue.ExistRealPlayer(bracketEntry))
        return;
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
        if (!pSession)
            continue;
        Player* player = pSession->GetPlayer();
        if (!player || player->isBeingLoaded())
            continue;
        if (player->InBattleground() || player->InArena() || !player->IsInWorld())
            continue;
        uint32 lv = player->getLevel();
        if (lv < bracketEntry->MinLevel || lv > bracketEntry->MaxLevel)
            continue;
        if (player->InBattlegroundQueue())
        {
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_OutAAQueue, 0);
            schedule1.parameter1 = bgTypeId;
            schedule1.parameter2 = bracketEntry->RangeIndex;
            schedule1.parameter3 = aaType;
            pSession->PushScheduleToQueue(schedule1);
        }
    }
}

void PlayerBotMgr::OnRealPlayerEnterBattleground(uint32 bgTypeId, uint32 level)
{
}

void PlayerBotMgr::OnRealPlayerLeaveBattleground(const Player* player)
{
    Battleground* bg = player->GetBattleground();
    if (!bg || bg->GetStatus() != BattlegroundStatus::STATUS_WAIT_LEAVE)
        return;
    PVPDifficultyEntry const* bracketEntry = sDB2Manager.GetBattlegroundBracketByLevel(bg->GetMapId(), player->getLevel());
    if (!bracketEntry)
        return;

    auto& allPlayerIDs = bg->GetPlayers();
    for (auto itIDs = allPlayerIDs.begin(); itIDs != allPlayerIDs.end(); itIDs++)
    {
        Player* otherPlayer = ObjectAccessor::FindPlayer(itIDs->first);
        if (!otherPlayer || otherPlayer->isBeingLoaded())
            continue;
        if (!otherPlayer->InBattleground())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)otherPlayer->GetSession());
        if (pSession)
        {
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_LeaveBG, 0);
            pSession->PushScheduleToQueue(schedule1);
        }
    }
}

bool PlayerBotMgr::LoginBotByAccountIndex(uint32 account, uint32 index)
{
#ifndef INCOMPLETE_BOT
    PlayerBotBaseInfo* botInfo = GetPlayerBotAccountInfo(account);
    if (!botInfo)
        return false;
    if (index >= botInfo->characters.size())
        return false;
    for (auto itChar = botInfo->characters.begin();
        itChar != botInfo->characters.end();
        itChar++)
    {
        if (index > 0)
        {
            --index;
            continue;
        }
        PlayerBotCharBaseInfo& charInfo = itChar->second;
        WorldSession* pWorldSession = sWorld->FindSession(account).get();
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->GetPlayer())
            return false;
        BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, charInfo.guid);
        pSession->PushScheduleToQueue(schedule1);
        return true;
    }
#endif
    return false;
}

bool PlayerBotMgr::AddNewPlayerBotByGUID(ObjectGuid& guid)
{
    int32 isok = sConfigMgr->GetIntDefault("pbot", 1);

    if (isok==0 && !guid)
        return false;

    // PlayerBot.MaxOnline counts bots only; real players used to be counted too.
    if (int32(GetOnlineBotCount(TEAM_NEUTRAL, false)) >= m_MaxOnlineBot)
        return false;


    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return false;
            if (player->GetSession()->PlayerLoading())
                return false;

        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return false;
            if (player->GetSession()->PlayerLoading())
                return false;

        }
    }


    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        if (!(itInfo->second->ExistCharacterByGUID(guid)))
            continue;
        WorldSession* pWorldSession = sWorld->FindSession(itInfo->first).get();
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules())
            return false;
        if (Player* player = pSession->GetPlayer())
        {
            if (!player->GetGroup() && !player->isInCombat() && pSession->GetPlayer()->GetGUID() != guid)
            {
                BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_Offline, 0);
                pSession->PushScheduleToQueue(schedule);
            }
            else
                return false;
        }
        BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
        pSession->PushScheduleToQueue(schedule1);
        return true;
    }

    // Characters of real player accounts (offline group members) only come
    // online as bots when PlayerBot.AccountAll allows it.
    if (!sWorld->getBoolConfig(CONFIG_PLAYERBOT_ACCOUNT_ALL))
        return false;

    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idAccountBotBase.begin();
        itInfo != m_idAccountBotBase.end();
        itInfo++)
    {
        if (!(itInfo->second->ExistCharacterByGUID(guid)))
            continue;
        WorldSession* pWorldSession = sWorld->FindSession(itInfo->first).get();
        if (pWorldSession)
        {
            PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
            if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules())
                return false;
            if (Player* player = pSession->GetPlayer())
            {
                if (!player->GetGroup() && !player->isInCombat() && pSession->GetPlayer()->GetGUID() != guid)
                {
                    BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_Offline, 0);
                    pSession->PushScheduleToQueue(schedule);
                }
                else
                    return false;
            }
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
            pSession->PushScheduleToQueue(schedule1);
            return true;
        }
        else
        {
            PlayerBotSession* pSession = UpPlayerBotSessionByBaseInfo(itInfo->second, true);
            if (!pSession)
                return false;
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
            pSession->PushScheduleToQueue(schedule1);
            return true;
        }
    }
    return false;
}


bool PlayerBotMgr::AddNewPlayerBotByGUID2(ObjectGuid& guid)
{
    // PlayerBot.MaxOnline counts bots only; real players used to be counted too.
    if (int32(GetOnlineBotCount(TEAM_NEUTRAL, false)) >= m_MaxOnlineBot) return false;


    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return false;
            if (player->GetSession()->PlayerLoading())
                return false;

        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return false;
            if (player->GetSession()->PlayerLoading())
                return false;

        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return false;
            if (player->GetSession()->PlayerLoading())
                return false;

        }
    }

    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        if (!(itInfo->second->ExistCharacterByGUID(guid)))
            continue;
        WorldSession* pWorldSession = sWorld->FindSession(itInfo->first).get();
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules())
            return false;
        if (Player* player = pSession->GetPlayer())
        {
            if (!player->GetGroup() && !player->isInCombat() && pSession->GetPlayer()->GetGUID() != guid)
            {
                BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_Offline, 0);
                pSession->PushScheduleToQueue(schedule);
            }
            else
                return false;
        }
        BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
        pSession->PushScheduleToQueue(schedule1);
        return true;
    }

    // Characters of real player accounts (offline group members) only come
    // online as bots when PlayerBot.AccountAll allows it.
    if (!sWorld->getBoolConfig(CONFIG_PLAYERBOT_ACCOUNT_ALL))
        return false;

    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idAccountBotBase.begin();
        itInfo != m_idAccountBotBase.end();
        itInfo++)
    {
        if (!(itInfo->second->ExistCharacterByGUID(guid)))
            continue;
        WorldSession* pWorldSession = sWorld->FindSession(itInfo->first).get();
        if (pWorldSession)
        {
            PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
            if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules())
                return false;
            if (Player* player = pSession->GetPlayer())
            {
                if (!player->GetGroup() && !player->isInCombat() && pSession->GetPlayer()->GetGUID() != guid)
                {
                    BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_Offline, 0);
                    pSession->PushScheduleToQueue(schedule);
                }
                else
                    return false;
            }
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
            pSession->PushScheduleToQueue(schedule1);
            return true;
        }
        else
        {
            PlayerBotSession* pSession = UpPlayerBotSessionByBaseInfo(itInfo->second, true);
            if (!pSession)
                return false;
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
            pSession->PushScheduleToQueue(schedule1);
            return true;
        }
    }
    return false;
}

void PlayerBotMgr::AddNewPlayerBot(bool faction, Classes prof, uint32 count)
{

    int32 isok = sConfigMgr->GetIntDefault("pbot", 1);
    if (isok==0 && prof == CLASS_NONE)
        return;

    // PlayerBot.MaxOnline counts bots only; real players used to be counted too.
    if (int32(GetOnlineBotCount(TEAM_NEUTRAL, false)) >= m_MaxOnlineBot)
        return;

    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }


    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }

    if (count <= 0)
        return;



    // CLASS_NONE stays "any": the class is taken from a character the account
    // actually has. Rolling one up front mostly picked classes nobody had.
#ifdef INCOMPLETE_BOT
    if (prof != 1 && prof != 5 && prof != 9)
    {
        if (prof == 2 || prof == 6)
            prof = Classes::CLASS_WARRIOR;
        else if (prof == 3 || prof == 4 || prof == 8 || prof == 10)
            prof = Classes::CLASS_WARLOCK;
        else if (prof == 7 || prof == 11)
            prof = Classes::CLASS_PRIEST;
    }
#endif
    //const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (count <= 0)
            return;
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (player)
            continue;
        uint32 botClass = PickBotCharacterClass(GetPlayerBotAccountInfo(pSession->GetAccountId()), faction, prof);
        if (!botClass)
            continue;
        BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
        if (faction)
        {
            schedule1.parameter1 = 1;
        }
        else
        {
            schedule1.parameter1 = 2;
        }
        schedule1.parameter2 = botClass;
        pSession->PushScheduleToQueue(schedule1);
        --count;
    }

    if (count > 0)
    {
        std::string allonlineText;
        allonlineText = "|cffff8800No further offline bot account has a matching character; create more bot characters to bring more bots online.|r";
        sWorld->SendGlobalText(allonlineText.c_str(), NULL);
    }
}

void PlayerBotMgr::AddNewAccountBot(bool faction, Classes prof)
{
    int32 isok = sConfigMgr->GetIntDefault("pbotall", 1);
    if (isok==0)
        return;

    // PlayerBot.MaxOnline counts bots only; real players used to be counted too.
    if (int32(GetOnlineBotCount(TEAM_NEUTRAL, false)) >= m_MaxOnlineBot)
        return;


    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }
    std::string allonlineText;
#ifdef INCOMPLETE_BOT
    allonlineText = "|cffff8800Bots cannot be summoned in this build.|r";
    sWorld->SendGlobalText(allonlineText.c_str(), NULL);
    return;
#endif
    if (prof == CLASS_NONE)
    {
        // Every playable class is eligible. The loop this replaces excluded
        // Death Knight, Monk and Demon Hunter.
        prof = Classes(urand(Classes::CLASS_WARRIOR, Classes::CLASS_DEMON_HUNTER));
    }
    //std::set<uint32> hasSessionIDs;
//	const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || !pSession->IsAccountBotSession())
            continue;
        //hasSessionIDs.insert(pSession->GetAccountId());
        Player* player = pSession->GetPlayer();
        if (player)
            continue;
        PlayerBotBaseInfo* pBaseInfo = GetAccountBotAccountInfo(pSession->GetAccountId());
        if (!pBaseInfo)
            continue;
        if (!pBaseInfo->ExistClass(faction, prof))
            continue;
        BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
        if (faction)
            schedule1.parameter1 = 1;
        else
            schedule1.parameter1 = 2;
        schedule1.parameter2 = prof;
        pSession->PushScheduleToQueue(schedule1);
        return;
    }

    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idAccountBotBase.begin();
        it != m_idAccountBotBase.end();
        it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        WorldSession* pSession = sWorld->FindSession(pInfo->id).get();
        if (pSession)
            continue;
        if (!pInfo->ExistClass(faction, prof))
            continue;
        PlayerBotSession* pBotSession = UpPlayerBotSessionByBaseInfo(pInfo, true);
        if (pBotSession)
        {
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
            if (faction)
                schedule1.parameter1 = 1;
            else
                schedule1.parameter1 = 2;
            schedule1.parameter2 = prof;
            pBotSession->PushScheduleToQueue(schedule1);
            return;
        }
    }

    allonlineText = "|cffff8800No bot character of that class was found for your faction.|r";
    sWorld->SendGlobalText(allonlineText.c_str(), NULL);
}

void PlayerBotMgr::AddNewPlayerBotByClass(uint32 count, Classes prof)
{
    int32 isok = sConfigMgr->GetIntDefault("pbotall", 1);
    if (isok==0)
        return;

    int32 allianceCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_ALLIANCE, true);
    int32 hordeCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_HORDE, true);

    if ((allianceCount+hordeCount) >= m_MaxOnlineBot) return;

    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }


    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }

#ifdef INCOMPLETE_BOT
    if (prof != 1 && prof != 5 && prof != 9)
    {
        if (prof == 2 || prof == 6)
            prof = Classes::CLASS_WARRIOR;
        else if (prof == 3 || prof == 4 || prof == 8 || prof == 10)
            prof = Classes::CLASS_WARLOCK;
        else if (prof == 7 || prof == 11)
            prof = Classes::CLASS_PRIEST;
    }
#endif

    //	const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (allianceCount <= 0 && hordeCount <= 0)
            return;
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (player && player->getClass() != prof)
            continue;
        if (player)
        {
            if (player->GetTeamId() == TeamId::TEAM_ALLIANCE)
                --allianceCount;
            else
                --hordeCount;
        }
        else
        {
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
            if (allianceCount > 0)
            {
                schedule1.parameter1 = 1;
                --allianceCount;
            }
            else if (hordeCount > 0)
            {
                schedule1.parameter1 = 2;
                --hordeCount;
            }
            else
                continue;
            schedule1.parameter2 = prof;
            pSession->PushScheduleToQueue(schedule1);
        }
    }

    if (allianceCount > 0 || hordeCount > 0)
    {
        std::string allonlineText;
        allonlineText = "|cffff8800All bot accounts are already online; no further bots can be brought online.|r";
        sWorld->SendGlobalText(allonlineText.c_str(), NULL);
    }
}

bool PlayerBotMgr::ChangePlayerBotSetting(uint32 account, uint32 minLV, uint32 maxLV, uint32 talent)
{
    if (account == 0)
        return false;
    maxLV = PlayerBotSetting::CheckMaxLevel(maxLV);
    if (maxLV < minLV)
        maxLV = minLV;
    WorldSession* pWorldSession = sWorld->FindSession(account).get();
    if (pWorldSession && pWorldSession->IsBotSession() && !pWorldSession->PlayerLoading())
    {
        Player* player = pWorldSession->GetPlayer();
        if (!player || player->InBattlegroundQueue() || player->InBattleground() ||
            player->GetBattleground() || player->isInCombat())
            return false;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->HasSchedules())
            return false;
        BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
        schedule2.parameter1 = minLV;
        schedule2.parameter2 = maxLV;
        schedule2.parameter3 = talent + 1;
        pSession->PushScheduleToQueue(schedule2);
        return true;
    }
    return false;
}

void PlayerBotMgr::AddNewPlayerBotToBG(TeamId team, uint32 minLV, uint32 maxLV, BattlegroundTypeId bgTypeID)
{
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return;
            if (player->GetSession()->PlayerLoading())
                return;
        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }

    maxLV = PlayerBotSetting::CheckMaxLevel(maxLV);
    if (maxLV < minLV)
        maxLV = minLV;
    //	const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (player)
        {
            if (!player || player->isBeingLoaded() || !player->IsInWorld())
                continue;
            if (player->InBattleground() || player->InArena() || player->GetMap()->IsDungeon() || player->isUsingLfg())
                continue;
            if (player->InBattlegroundQueue())
                continue;
            if (!player->IsSettingFinish())
                continue;
            if (player->GetTeamId() != team)
                continue;

            BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
            schedule2.parameter1 = maxLV;
            schedule2.parameter2 = maxLV;
            schedule2.parameter3 = 4;
            pSession->PushScheduleToQueue(schedule2);

            BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InBGQueue, 0);
            schedule3.parameter1 = bgTypeID;
            pSession->PushScheduleToQueue(schedule3);
            return;
        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (!player)
        {
            uint32 botClass = PickBotCharacterClass(GetPlayerBotAccountInfo(pSession->GetAccountId()), team == TeamId::TEAM_ALLIANCE, CLASS_NONE);
            if (!botClass)
                continue;

            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
            schedule1.parameter1 = (team == TeamId::TEAM_ALLIANCE) ? 1 : 2;
            schedule1.parameter2 = botClass;
            pSession->PushScheduleToQueue(schedule1);

            BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
            schedule2.parameter1 = minLV;
            schedule2.parameter2 = maxLV;
            schedule2.parameter3 = 4;
            pSession->PushScheduleToQueue(schedule2);

            BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InBGQueue, 0);
            schedule3.parameter1 = bgTypeID;
            pSession->PushScheduleToQueue(schedule3);
            return;
        }
    }

    // Runs from Update() every two seconds, so this must not go to the world
    // chat: it used to be broadcast to every player on each tick.
    TC_LOG_DEBUG(LOG_FILTER_PLAYER, "PlayerBotMgr: no idle bot account with a %s character left for the battleground.",
        team == TeamId::TEAM_ALLIANCE ? "alliance" : "horde");
}

// Brings one bot online (or repurposes an idle one) to fill the role the
// dungeon finder is short of. Driven from Update() via
// LFGMgr::SearchLFGBotRequirement().
void PlayerBotMgr::AddNewPlayerBotToLFG(lfg::LFGBotRequirement* botRequirement)
{
    if (!botRequirement || botRequirement->selectedDungeons.empty())
        return;
    if (botRequirement->needTeam != TEAM_ALLIANCE && botRequirement->needTeam != TEAM_HORDE)
        return;

    std::vector<uint32> dungeons(botRequirement->selectedDungeons.begin(), botRequirement->selectedDungeons.end());
    uint32 const dungeonCount = std::min<uint32>(dungeons.size(), 3);

    auto pushQueueSchedule = [&](PlayerBotSession* pSession)
    {
        BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_InLFGQueue, ObjectGuid::Empty);
        schedule.parameter1 = uint32(botRequirement->needRole);
        schedule.parameter2 = dungeonCount;
        if (dungeonCount >= 1)
            schedule.parameter3 = dungeons[0];
        if (dungeonCount >= 2)
            schedule.parameter4 = dungeons[1];
        if (dungeonCount >= 3)
            schedule.parameter5 = dungeons[2];
        pSession->PushScheduleToQueue(schedule);
    };

    SessionMap const& allSession = sWorld->GetAllSessions();

    // First pass: an idle bot that is already online and matches team and role.
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); ++itSession)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (!player || !IsIDLEPlayerBot(player))
            continue;
        if (uint8(player->GetTeamId()) != botRequirement->needTeam)
            continue;
        if (uint8(player->GetSpecializationRoleMaskForGroup()) != uint8(botRequirement->needRole))
            continue;

        pushQueueSchedule(pSession);
        return;
    }

    // Second pass: bring a fresh bot online with a class that can fill the role.
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); ++itSession)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        if (pSession->GetPlayer())
            continue;

        BotGlobleSchedule loginSchedule(BotGlobleScheduleType::BGSType_Online, ObjectGuid::Empty);
        if (!FillOnlineBotScheduleByLFGRequirement(botRequirement, &loginSchedule))
            return;
        // The class above is rolled from the ones that can fill the role;
        // move on to the next account when this one has no such character.
        if (!PickBotCharacterClass(GetPlayerBotAccountInfo(pSession->GetAccountId()), loginSchedule.parameter1 == 1, loginSchedule.parameter2))
            continue;
        pSession->PushScheduleToQueue(loginSchedule);

        BotGlobleSchedule setupSchedule(BotGlobleScheduleType::BGSType_Settting, ObjectGuid::Empty);
        setupSchedule.parameter1 = botRequirement->needLevel;
        setupSchedule.parameter2 = botRequirement->needLevel;
        setupSchedule.parameter3 = GetScheduleTalentByLFGRequirement(botRequirement->needRole, loginSchedule.parameter2) + 1;
        pSession->PushScheduleToQueue(setupSchedule);

        pushQueueSchedule(pSession);
        return;
    }
}

void PlayerBotMgr::AddNewPlayerBotToAA(TeamId team, BattlegroundTypeId bgTypeID, uint32 bracketID, uint32 aaType)
{
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return;
            if (player->GetSession()->PlayerLoading())
                return;
            //if (player->isInCombat())
            //return;
        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->isBeingLoaded())
                return;
            if (player->GetSession()->PlayerLoading())
                return;
            //if (player->isInCombat())
            //return;
        }
    }


    if (aaType != 2 && aaType != 3 && aaType != 5)
        return;
    //	const SessionMap& allSession = sWorld->GetAllSessions();
    std::vector<uint32> rndIDs;
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
        rndIDs.push_back(itSession->first);
    Trinity::Containers::RandomShuffle(rndIDs);
    for (uint32 account : rndIDs)
    {
        WorldSession* pWorldSession = sWorld->FindSession(account).get();
        if (!pWorldSession || !pWorldSession->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (player)
        {
            if (!player || player->isBeingLoaded() || !player->IsInWorld())
                continue;
            if (player->InBattleground() || player->InArena() || player->GetMap()->IsDungeon() || player->isUsingLfg())
                continue;
            if (player->InBattlegroundQueue())
                continue;
            if (!player->IsSettingFinish())
                continue;
            if (player->GetTeamId() != team)
                continue;

            BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
            schedule2.parameter1 = 110;
            schedule2.parameter2 = 110;
            schedule2.parameter3 = 4;
            pSession->PushScheduleToQueue(schedule2);

            BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InAAQueue, 0);
            schedule3.parameter1 = 14;
            schedule3.parameter2 = 4;
            pSession->PushScheduleToQueue(schedule3);

            return;
        }
    }

    for (uint32 account : rndIDs)
    {
        WorldSession* pWorldSession = sWorld->FindSession(account).get();
        if (!pWorldSession || !pWorldSession->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (!player)
        {
            uint32 botClass = PickBotCharacterClass(GetPlayerBotAccountInfo(pSession->GetAccountId()), team == TeamId::TEAM_ALLIANCE, CLASS_NONE);
            if (!botClass)
                continue;

            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
            schedule1.parameter1 = (team == TeamId::TEAM_ALLIANCE) ? 1 : 2;
            schedule1.parameter2 = botClass;
            pSession->PushScheduleToQueue(schedule1);

            BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
            schedule2.parameter1 = 110;
            schedule2.parameter2 = 110;
            schedule2.parameter3 = 4;
            pSession->PushScheduleToQueue(schedule2);

            BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InAAQueue, 0);
            schedule3.parameter1 = 14;
            schedule3.parameter2 = 4;
            pSession->PushScheduleToQueue(schedule3);

            return;
        }
    }

    // Runs from Update() every two seconds, so this must not go to the world
    // chat: it used to be broadcast to every player on each tick.
    TC_LOG_DEBUG(LOG_FILTER_PLAYER, "PlayerBotMgr: no idle bot account with a %s character left for the arena.",
        team == TeamId::TEAM_ALLIANCE ? "alliance" : "horde");
}

void PlayerBotMgr::AddTeamBotToRatedArena(uint32 arenaType)
{
    // Legion no longer has arena teams: rated arena is queued by the leader of
    // an ordinary group of the right size -- WorldSession::JoinBracket only
    // checks the group. The previous body targeted the Mists-era ArenaTeam API
    // and was commented out together with the helpers it needed
    // (CheckPlayerBotArenaTeam, SearchEnemyArenaTeam,
    // FindArenaTeamPlayerBotTalent), none of which exist in this tree.
    //
    // arenaType is the bracket size: 2 or 3.
    if (arenaType != 2 && arenaType != 3)
        return;

    uint8 bracketType = (arenaType == 2) ? uint8(MS::Battlegrounds::BracketType::Arena2v2)
                                         : uint8(MS::Battlegrounds::BracketType::Arena3v3);

    for (uint8 pass = 0; pass < 2; ++pass)
    {
        TeamId team = (pass == 0) ? TEAM_ALLIANCE : TEAM_HORDE;

        std::vector<PlayerBotSession*> candidates;
        SessionMap const& allSession = sWorld->GetAllSessions();
        for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); ++itSession)
        {
            if (!itSession->second->IsBotSession())
                continue;
            PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
            if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
                continue;
            Player* player = pSession->GetPlayer();
            if (!player || !IsIDLEPlayerBot(player))
                continue;
            if (player->GetTeamId() != team || player->GetGroup())
                continue;
            if (player->getLevel() < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
                continue;

            candidates.push_back(pSession);
            if (candidates.size() >= arenaType)
                break;
        }

        if (candidates.size() < arenaType)
            continue;

        Player* leader = candidates[0]->GetPlayer();
        Group* group = new Group();
        if (!group->Create(leader))
        {
            delete group;
            continue;
        }
        sGroupMgr->AddGroup(group);

        bool complete = true;
        for (size_t i = 1; i < candidates.size(); ++i)
        {
            Player* member = candidates[i]->GetPlayer();
            if (!member || !group->AddMember(member))
            {
                complete = false;
                break;
            }
        }

        if (!complete)
        {
            group->Disband(true);
            return;
        }

        // Only the leader queues; the rest of the group rides along.
        BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_InAAQueue, ObjectGuid::Empty);
        schedule.parameter1 = uint32(leader->GetSpecializationRoleMaskForGroup());
        schedule.parameter2 = bracketType;
        schedule.parameter3 = 1;                 // rated
        candidates[0]->PushScheduleToQueue(schedule);
        return;
    }
}

bool PlayerBotMgr::FillOnlineBotScheduleByLFGRequirement(lfg::LFGBotRequirement* botRequirement, BotGlobleSchedule* botSchedule)
{
	if (!botRequirement || !botSchedule)
		return false;
	botSchedule->parameter1 = 0;
	if (botRequirement->needTeam == uint8(TEAM_ALLIANCE))
		botSchedule->parameter1 = 1;
	else if (botRequirement->needTeam == uint8(TEAM_HORDE))
		botSchedule->parameter1 = 2;
	else
		return false;
	std::vector<uint32> matchClasses;
	botSchedule->parameter2 = 0;
	if (botRequirement->needRole == lfg::LfgRoles::PLAYER_ROLE_TANK)
	{
		matchClasses.push_back(1);
#ifndef INCOMPLETE_BOT
		matchClasses.push_back(2);
#endif
		//matchClasses.push_back(6);
	}
	else if (botRequirement->needRole == lfg::LfgRoles::PLAYER_ROLE_HEALER)
	{
		matchClasses.push_back(5);
#ifndef INCOMPLETE_BOT
		matchClasses.push_back(2);
		matchClasses.push_back(7);
		matchClasses.push_back(11);
#endif
	}
	else if (botRequirement->needRole == lfg::LfgRoles::PLAYER_ROLE_DAMAGE)
	{
#ifndef INCOMPLETE_BOT
		matchClasses.push_back(3);
		matchClasses.push_back(4);
		matchClasses.push_back(8);
#endif
		matchClasses.push_back(9);
	}
	if (matchClasses.empty())
		return false;
	botSchedule->parameter2 = matchClasses[urand(0, matchClasses.size() - 1)];
	return true;
}

uint32 PlayerBotMgr::GetScheduleTalentByLFGRequirement(lfg::LfgRoles roles, uint32 botCls)
{
    uint32 talentType = 3;
    switch (botCls)
    {
        case 1:
            if (roles == lfg::LfgRoles::PLAYER_ROLE_TANK)
                talentType = 2;
            else
                talentType = urand(0, 1);
            break;
        case 2:
            if (roles == lfg::LfgRoles::PLAYER_ROLE_TANK)
                talentType = 1;
            else if (roles == lfg::LfgRoles::PLAYER_ROLE_HEALER)
                talentType = 0;
            else
                talentType = 2;
            break;
        case 6:
            if (roles == lfg::LfgRoles::PLAYER_ROLE_TANK)
                talentType = 1;
            else
            {
                if (urand(0, 1) == 0)
                    talentType = 0;
                else
                    talentType = 2;
            }
            break;
        case 5:
            if (roles == lfg::LfgRoles::PLAYER_ROLE_DAMAGE)
                talentType = 2;
            else
                talentType = urand(0, 1);
            break;
        case 7:
            if (roles == lfg::LfgRoles::PLAYER_ROLE_HEALER)
                talentType = 2;
            else
                talentType = urand(0, 1);
            break;
        case 11:
            if (roles == lfg::LfgRoles::PLAYER_ROLE_HEALER)
                talentType = 2;
            else
                talentType = urand(0, 1);
            break;
    }
    return talentType;
}

lfg::LfgRoles PlayerBotMgr::GetPlayerBotCurrentLFGRoles(Player* player)
{
    if (!player)
        return lfg::LfgRoles::PLAYER_ROLE_NONE;
    uint32 talentType = player->FindTalentType();
    switch (player->getClass())
    {
        case 1:
            if (talentType == 2)
                return lfg::LfgRoles::PLAYER_ROLE_TANK;
            else
                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
            break;
        case 2:
            if (talentType == 0)
                return lfg::LfgRoles::PLAYER_ROLE_HEALER;
            else if (talentType == 1)
                return lfg::LfgRoles::PLAYER_ROLE_TANK;
            else
                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
            break;
        case 6:
            if (talentType == 1)
                return lfg::LfgRoles::PLAYER_ROLE_TANK;
            else
                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
            break;
        case 3:
        case 4:
        case 8:
        case 9:
            return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
            break;
        case 5:
            if (talentType == 2)
                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
            else
                return lfg::LfgRoles::PLAYER_ROLE_HEALER;
            break;
        case 7:
            if (talentType == 2)
                return lfg::LfgRoles::PLAYER_ROLE_HEALER;
            else
                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
            break;
        case 11:
            if (talentType == 2)
                return lfg::LfgRoles::PLAYER_ROLE_HEALER;
            else
                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
            break;
        default:
            return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
    }
    return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
}

ObjectGuid PlayerBotMgr::GetNoArenaMatchCharacter(TeamId team, uint32 arenaType, Classes cls, std::vector<ObjectGuid>& fliters)
{
    if (team != TEAM_ALLIANCE && team != TEAM_HORDE)
        return ObjectGuid::Empty;
    if (arenaType != 2 && arenaType != 3 && arenaType != 5)
        return ObjectGuid::Empty;
    if (m_idPlayerBotBase.empty())
        return ObjectGuid::Empty;
    std::vector<PlayerBotBaseInfo*> allBotBaseInfos;
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
        allBotBaseInfos.push_back(itInfo->second);
    for (uint32 i = allBotBaseInfos.size() - 1; i >= 0; --i)
    {
        PlayerBotBaseInfo* pInfo = allBotBaseInfos[i];
        uint32 id = pInfo->GetCharIDByNoArenaType((team == TEAM_ALLIANCE) ? true : false, cls, arenaType, fliters);
        if (id == 0)
            continue;
        bool exist = false;
        for (ObjectGuid& guid : fliters)
        {
            if (guid.GetCounter() == id)
            {
                exist = true;
                break;
            }
        }
        if (!exist)
            return ObjectGuid::Create<HighGuid::Player>(id);
    }

    //for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
    //	itInfo != m_idPlayerBotBase.end();
    //	itInfo++)
    //{
    //	PlayerBotBaseInfo* pInfo = itInfo->second;
    //	uint32 id = pInfo->GetCharIDByNoArenaType((team == TEAM_ALLIANCE) ? true : false, cls, arenaType, fliters);
    //	if (id == 0)
    //		continue;
    //	bool exist = false;
    //	for (ObjectGuid& guid : fliters)
    //	{
    //		if (guid.GetCounter() == id)
    //		{
    //			exist = true;
    //			break;
    //		}
    //	}
    //	if (!exist)
    //		return ObjectGuid(uint64(id));
    //}
    return ObjectGuid::Empty;
}

std::string PlayerBotMgr::GetNameANDClassesText(ObjectGuid& guid)
{
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        PlayerBotBaseInfo* pInfo = itInfo->second;
        std::string text = pInfo->GetCharNameANDClassesText(guid);
        if (!text.empty())
            return text;
    }
    return "";
}

bool PlayerBotMgr::CanReadyArenaByArenaTeamID(uint32 arenaTeamId)
{

    return true;
}

void PlayerBotMgr::QueryBattlegroundRequirement()
{
    for (BattleGroundTypes::iterator itType = m_BGTypes.begin(); itType != m_BGTypes.end(); itType++)
    {
        BattlegroundTypeId bgTypeID = *itType;
        Battleground* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeID);
        if (!bg_template)
            continue;
        auto& bgFreeSlotQueues = sBattlegroundMgr->BGFreeSlotQueue;
        for (auto itr = bgFreeSlotQueues.begin(); itr != bgFreeSlotQueues.end(); itr++)
        {
            Battleground* bg = *itr;
            if (!bg->ExistRealPlayer())
                continue;
            uint32 minLV = bg->GetMinLevel();
            uint32 maxLV = bg->GetMaxLevel();
            if (!bg->IsRated() && bg->GetStatus() > STATUS_WAIT_QUEUE && bg->GetStatus() < STATUS_WAIT_LEAVE)
            {
                if (bg->GetFreeSlotsForTeam(Team::ALLIANCE) > 0 && bg->GetFreeSlotsForTeam(Team::ALLIANCE) >= bg->GetFreeSlotsForTeam(Team::HORDE))
                    AddNewPlayerBotToBG(TeamId::TEAM_ALLIANCE, minLV, maxLV, bgTypeID);
                else if (bg->GetFreeSlotsForTeam(Team::HORDE) > 0)
                    AddNewPlayerBotToBG(TeamId::TEAM_HORDE, minLV, maxLV, bgTypeID);
                else
                    continue;
                return;
            }
        }

        uint8 bgQueueTypeID = MS::Battlegrounds::GetBgQueueTypeIdByBgTypeID(bgTypeID, 0);
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeID);
        for (uint8 j = 0; j < MS::Battlegrounds::MaxBrackets; j++)
        {
            uint8 bracket = j;
            PVPDifficultyEntry const* bracketEntry = sDB2Manager.GetBattlegroundBracketById(bg_template->GetMapId(), bracket);
            if (!bracketEntry)
                continue;
            if (!bgQueue.ExistRealPlayer(bracketEntry))
                continue;
            int32 needAlliancePlayerCount = 0;
            int32 needHordePlayerCount = 0;
            if (bgQueue.QueryNeedPlayerCount(bgTypeID, bracket, 0, needAlliancePlayerCount, needHordePlayerCount))
            {
                if (needAlliancePlayerCount > 0 && needAlliancePlayerCount >= needHordePlayerCount)
                    AddNewPlayerBotToBG(TeamId::TEAM_ALLIANCE, bracketEntry->MinLevel, bracketEntry->MaxLevel, bgTypeID);
                else if (needHordePlayerCount > 0)
                    AddNewPlayerBotToBG(TeamId::TEAM_HORDE, bracketEntry->MinLevel, bracketEntry->MaxLevel, bgTypeID);
                else
                    continue;
                return;
            }
        }
    }
}

void PlayerBotMgr::QueryRatedArenaRequirement()
{
    if (m_ArenaSearchTick > 0)
    {
        --m_ArenaSearchTick;
        return;
    }
    Battleground* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(MS::Battlegrounds::BattlegroundTypeId::ArenaAll);
    if (!bg_template)
        return;
    PVPDifficultyEntry const* bracketEntry = FindBGBracketEntry(bg_template, 80);
    if (!bracketEntry)
        return;

    uint32 arenaType[3] ={ 2, 3, 5 };
    for (uint32 i = 0; i < 3; i++)
    {
        uint8 bgQueueTypeID = MS::Battlegrounds::GetBgQueueTypeIdByBgTypeID(MS::Battlegrounds::BattlegroundTypeId::ArenaAll, arenaType[i]);
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeID);
        if (!bgQueue.ExistRealPlayer(bracketEntry, true))
        {
            bgQueue.AllPlayerBotLeaveQueueFromRatedArena(bracketEntry->RangeIndex);
            continue;
        }
        uint32 allianceGroupID = 0;
        uint32 hordeGroupID = 0;
        GroupQueueInfo* allianceGroupInfo = bgQueue.GetFirstRealPlayerGroupInfo(bracketEntry->RangeIndex, MS::Battlegrounds::QueueGroupTypes::PremadeAlliance);
        GroupQueueInfo* hordeGroupInfo = bgQueue.GetFirstRealPlayerGroupInfo(bracketEntry->RangeIndex, MS::Battlegrounds::QueueGroupTypes::PremadeHorde);
        if (!allianceGroupInfo && !hordeGroupInfo)
            continue;
        if (allianceGroupInfo && hordeGroupInfo)
        {
            // Both sides already have a real group waiting; nothing to fill in.
            allianceGroupID = allianceGroupInfo->GroupId;
            hordeGroupID = hordeGroupInfo->GroupId;
        }
        else if (!BotUtility::DownBotArenaTeam && (arenaType[i] == 2 || arenaType[i] == 3))
        {
            // One side is waiting -- put a bot group of the same size in so the
            // match can start. The branch that used to sit here looked for an
            // opposing ArenaTeam, which Legion no longer has.
            AddTeamBotToRatedArena(arenaType[i]);
        }
        else
            continue;
        m_ArenaSearchTick = 14;
        break;
    }
}

void PlayerBotMgr::QueryNonRatedArenaRequirement()
{
    Battleground* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(MS::Battlegrounds::BattlegroundTypeId::ArenaAll);
    if (!bg_template)
        return;
    PVPDifficultyEntry const* bracketEntry = FindBGBracketEntry(bg_template, 110);
    if (!bracketEntry)
        return;
    auto& bgFreeSlotQueues = sBattlegroundMgr->BGFreeSlotQueue;
    for (auto itr = bgFreeSlotQueues.begin(); itr != bgFreeSlotQueues.end(); itr++)
    {
        Battleground* bg = *itr;
        if (!bg->ExistRealPlayer())
            continue;
        if (bg->GetStatus() > STATUS_WAIT_QUEUE && bg->GetStatus() < STATUS_WAIT_LEAVE)
        {
            if (bg->GetFreeSlotsForTeam(Team::ALLIANCE) > 0)
                AddNewPlayerBotToAA(TeamId::TEAM_ALLIANCE, MS::Battlegrounds::BattlegroundTypeId::ArenaAll, bracketEntry->RangeIndex, bg->GetMaxPlayers() / 2);
            else if (bg->GetFreeSlotsForTeam(Team::HORDE) > 0)
                AddNewPlayerBotToAA(TeamId::TEAM_HORDE, MS::Battlegrounds::BattlegroundTypeId::ArenaAll, bracketEntry->RangeIndex, bg->GetMaxPlayers() / 2);
            else
                continue;
            return;
        }
    }

    uint32 arenaType[3] ={ 2, 3, 5 };
    for (uint32 i = 0; i < 3; i++)
    {
        uint8 bgQueueTypeID = MS::Battlegrounds::GetBgQueueTypeIdByBgTypeID(MS::Battlegrounds::BattlegroundTypeId::ArenaAll, arenaType[i]);
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeID);
        if (!bgQueue.ExistRealPlayer(bracketEntry, false))
            continue;
        int32 needAlliancePlayerCount = 0;
        int32 needHordePlayerCount = 0;
        if (bgQueue.QueryNeedPlayerCount(MS::Battlegrounds::BattlegroundTypeId::ArenaAll, bracketEntry->RangeIndex, arenaType[i], needAlliancePlayerCount, needHordePlayerCount))
        {
            if (needAlliancePlayerCount > 0 && needAlliancePlayerCount >= needHordePlayerCount)
                AddNewPlayerBotToAA(TeamId::TEAM_ALLIANCE, MS::Battlegrounds::BattlegroundTypeId::ArenaAll, bracketEntry->RangeIndex, arenaType[i]);
            else if (needHordePlayerCount > 0)
                AddNewPlayerBotToAA(TeamId::TEAM_HORDE, MS::Battlegrounds::BattlegroundTypeId::ArenaAll, bracketEntry->RangeIndex, arenaType[i]);
            else
                continue;
            return;
        }
    }
}

void PlayerBotMgr::OnlinePlayerBotByGUIDQueue()
{
    while (!m_DelayOnlineBots.empty())
    {
        ObjectGuid& guid = m_DelayOnlineBots.front();
        m_DelayOnlineBots.pop();
        if (AddNewPlayerBotByGUID(guid))
            return;
    }
}

// Keeps at least PlayerBot.MinOnline bots logged in, never more than
// PlayerBot.MaxOnline. One bot per manager tick, from the faction that has
// fewer bots online. These bots stay in the open world; the battleground,
// arena and dungeon finder code takes idle online bots before it logs in new
// ones, so they are also the first to fill a queue.
void PlayerBotMgr::TopUpOnlineBots()
{
    int32 const wanted = std::min(m_MinOnlineBot, m_MaxOnlineBot);
    if (wanted <= 0)
        return;

    uint32 allianceOnline = 0;
    uint32 hordeOnline = 0;
    uint32 total = 0;
    std::vector<PlayerBotSession*> offline;

    SessionMap const& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); ++itSession)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(itSession->second.get());
        if (!pSession || pSession->IsAccountBotSession())
            continue;

        // A login under way is not finished yet; wait for it rather than
        // starting the next one on top.
        if (pSession->PlayerLoading() || pSession->HasScheduleByType(BGSType_Online) || pSession->HasScheduleByType(BGSType_Online_GUID))
            return;

        if (Player* player = pSession->GetPlayer())
        {
            ++total;
            if (player->GetTeamId() == TEAM_ALLIANCE)
                ++allianceOnline;
            else
                ++hordeOnline;
        }
        else if (!pSession->HasSchedules())
            offline.push_back(pSession);
    }

    if (int32(total) >= wanted)
    {
        m_MinOnlineWarned = false;
        return;
    }

    bool const preferAlliance = allianceOnline <= hordeOnline;
    for (uint8 pass = 0; pass < 2; ++pass)
    {
        bool const alliance = (pass == 0) ? preferAlliance : !preferAlliance;
        for (PlayerBotSession* pSession : offline)
        {
            uint32 botClass = PickBotCharacterClass(GetPlayerBotAccountInfo(pSession->GetAccountId()), alliance, CLASS_NONE);
            if (!botClass)
                continue;

            BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_Online, 0);
            schedule.parameter1 = alliance ? 1 : 2;
            schedule.parameter2 = botClass;
            pSession->PushScheduleToQueue(schedule);
            return;
        }
    }

    // Nothing left to log in. While characters are still being created that
    // is expected; afterwards say so once.
    if (!m_MinOnlineWarned && m_CreateRequests.empty())
    {
        TC_LOG_ERROR(LOG_FILTER_SERVER_LOADING, "PlayerBot.MinOnline is %i, but only %u bots can be online: no further bot account has a character. Raise PlayerBot.CreateCharacters.Accounts.",
            wanted, total);
        m_MinOnlineWarned = true;
    }
}

bool PlayerBotMgr::ExistUnBGPlayerBot()
{
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
        if (!pSession)
            continue;
        if (pSession->PlayerLoading() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (!player || !player->IsInWorld())
            continue;
        if (player->InBattleground() || player->InBattlegroundQueue() || player->isUsingLfg() || player->GetGroup())
            continue;
        if (player->HasAura(26013))
            continue;
        if (!pSession->HasSchedules())
            continue;
        return true;
    }
    return false;
}

PVPDifficultyEntry const* PlayerBotMgr::FindBGBracketEntry(Battleground* bg_template, uint32 level)
{
    if (!bg_template)
        return NULL;
    for (uint32 j = 0; j < MS::Battlegrounds::MaxBrackets; j++)
    {
        uint8 bracket = j;
        PVPDifficultyEntry const* bracketEntry = sDB2Manager.GetBattlegroundBracketById(bg_template->GetMapId(), bracket);
        if (!bracketEntry)
            continue;
        if (bracketEntry->MinLevel <= level && bracketEntry->MaxLevel >= level)
            return bracketEntry;
    }
    return NULL;
}

uint32 PlayerBotMgr::GetOnlineBotCount(TeamId team, bool hasReal)
{
    uint32 onlineCount = 0;
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (itSession->second->IsBotSession())
        {
            PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
            if (!pSession)
                continue;
            // A bot still loading has no faction yet: it counts towards the
            // total only, not twice as both alliance and horde.
            if (pSession->PlayerLoading())
            {
                if (team == TEAM_NEUTRAL)
                    ++onlineCount;
                continue;
            }
            Player* player = pSession->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;
            if (team != TEAM_NEUTRAL)
            {
                if (player->GetTeamId() != team)
                    continue;
            }
            ++onlineCount;
        }
        else if (hasReal)
        {
            WorldSession* pSession = itSession->second.get();
            if (pSession->PlayerLoading())
                continue;
            Player* player = pSession->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;
            if (team != TEAM_NEUTRAL)
            {
                if (player->GetTeamId() != team)
                    continue;
            }
            ++onlineCount;
        }
    }
    return onlineCount;
}


uint32 PlayerBotMgr::GetOnlineBotCount2(TeamId team, bool hasReal)
{
    uint32 onlineCount = 0;
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (itSession->second->IsBotSession())
        {
            PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
            if (!pSession)
                continue;
            if (pSession->PlayerLoading())
            {
                //++onlineCount;
                continue;
            }
            Player* player = pSession->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;

            if (!player->InBattleground() &&  !player->InBattlegroundQueue())
                continue;

            if (team != TEAM_NEUTRAL)
            {
                if (player->GetTeamId() != team)
                    continue;
            }
            ++onlineCount;
        }
        else if (hasReal)
        {
            WorldSession* pSession = itSession->second.get();
            if (pSession->PlayerLoading())
                continue;
            Player* player = pSession->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;

            if (!player->InBattleground() &&  !player->InBattlegroundQueue())
                continue;

            if (team != TEAM_NEUTRAL)
            {
                if (player->GetTeamId() != team)
                    continue;
            }
            ++onlineCount;
        }
    }
    return onlineCount;
}

void PlayerBotMgr::Update()
{
    // PlayerBot.Enable stops bots from being brought online and queued. Freeing
    // swapped out AIs below keeps running so a config reload cannot strand them.
    if (sWorld->getBoolConfig(CONFIG_PLAYERBOT_ENABLE))
    {
        CreateOncePlayerBot();
        OnlinePlayerBotByGUIDQueue();

        //if (!ExistUnBGPlayerBot())
        QueryBattlegroundRequirement();
#ifndef INCOMPLETE_BOT
        //sArenaTeamMgr->CheckPlayerBotArenaTeam();
        QueryRatedArenaRequirement();
        QueryNonRatedArenaRequirement();
        if (m_LFGSearchTick > 1)
        {
            m_LFGSearchTick = 0;
            if (lfg::LFGBotRequirement* pbotReq = sLFGMgr->SearchLFGBotRequirement())
            {
                AddNewPlayerBotToLFG(pbotReq);
                delete pbotReq;
            }
        }
        else
            ++m_LFGSearchTick;
#endif
        TopUpOnlineBots();
    }

    std::list<std::map<uint32, std::list<UnitAI*> >::iterator > delITer;
    uint32 currentTick = getMSTime();
    for (std::map<uint32, std::list<UnitAI*> >::iterator itDelayAi = m_DelayDestroyAIs.begin();
        itDelayAi != m_DelayDestroyAIs.end();
        itDelayAi++)
    {
        uint32 delayTick = itDelayAi->first;
        // The delay exists so an AI that was swapped out is not freed while the
        // current update cycle may still be holding it. Free it once the grace
        // period has actually passed. getMSTime wraps around, so compare the
        // difference rather than the raw values.
        if (getMSTimeDiff(delayTick, currentTick) >= AI_DESTROY_DELAY)
        {
            for (std::list<UnitAI*>::iterator itAI = itDelayAi->second.begin();
                itAI != itDelayAi->second.end();
                itAI++)
            {
                UnitAI* pAI = (*itAI);
                if (pAI)
                    delete pAI;
            }
            itDelayAi->second.clear();
            delITer.push_back(itDelayAi);
        }
    }
    for (std::list<std::map<uint32, std::list<UnitAI*> >::iterator >::iterator itDel = delITer.begin();
        itDel != delITer.end();
        itDel++)
    {
        m_DelayDestroyAIs.erase(*itDel);
    }
}
