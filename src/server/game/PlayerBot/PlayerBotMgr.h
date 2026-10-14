
#ifndef __PLAYERBOTMGR_H__
#define __PLAYERBOTMGR_H__

class WorldPacket;

#include "ObjectGuid.h"

#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>

//#include <chrono>

#include "Log.h"
#include "Common.h"
#include "SharedDefines.h"
#include "DatabaseEnv.h"
#include "Timer.h"
//#include "Callback.h"
#include "PlayerBotSession.h"
#include "BotAITool.h"
#include "LFGMgr.h"
//#include "ArenaTeamMgr.h"
#include "Containers.h"

#define CONVERT_ARENAAI_TOBG

class ObjectGuid;
struct LFGBotRequirement;
struct BotGlobleSchedule;
class PlayerBotSession;

namespace WorldPackets
{
    namespace Character
    {
        struct CharacterCreateInfo;
    }
}

enum ArenaTeamTypes
{
    ARENA_TEAM_2v2      = 2,
    ARENA_TEAM_3v3      = 3,
    ARENA_TEAM_5v5      = 5
};

enum PlayerBotAIType
{
	PBAIT_FIELD,
	PBAIT_BG,
	PBAIT_GROUP,
	PBAIT_DUNGEON,
	PBAIT_DUEL,
	PBAIT_ARENA,
	PBAIT_OVER
};

// Faction of a playable race. The lists this replaces stopped at Wrath and
// missed Worgen, Goblin, Pandaren and the allied races, so the manager never
// picked such characters for their faction.
inline TeamId PlayerBotRaceTeam(uint16 race)
{
	switch (race)
	{
		case RACE_HUMAN:
		case RACE_DWARF:
		case RACE_NIGHTELF:
		case RACE_GNOME:
		case RACE_DRAENEI:
		case RACE_WORGEN:
		case RACE_PANDAREN_ALLIANCE:
		case RACE_VOID_ELF:
		case RACE_LIGHTFORGED_DRAENEI:
			return TEAM_ALLIANCE;
		case RACE_ORC:
		case RACE_UNDEAD_PLAYER:
		case RACE_TAUREN:
		case RACE_TROLL:
		case RACE_GOBLIN:
		case RACE_BLOODELF:
		case RACE_PANDAREN_HORDE:
		case RACE_NIGHTBORNE:
		case RACE_HIGHMOUNTAIN_TAUREN:
			return TEAM_HORDE;
		default:
			return TEAM_NEUTRAL;
	}
}

struct PlayerBotCharBaseInfo
{
	uint64 guid;
	uint32 account;
	std::string name;
	uint16 race;
	uint16 profession;
	uint16 gender;
	uint16 level;

	PlayerBotCharBaseInfo()
	{
		guid = 0;
		account = 0;
		race = profession = gender = level = 0;
	}
	PlayerBotCharBaseInfo(uint64 id, uint32 acc, const std::string &na, uint16 ra, uint16 pro, uint16 gen, uint16 lv) :
		guid(id), account(acc), name(na), race(ra), profession(pro), gender(gen), level(lv)
	{
	}

	std::string GetNameANDClassesText();

	TeamId GetCamp()
	{
		return PlayerBotRaceTeam(race);
	}
};

struct PlayerBotBaseInfo
{
	static PlayerBotCharBaseInfo empty;
	bool isAccountInfo;
	uint32 id;
	std::string username;
	std::string pass;
	using CharInfoMap = std::map<uint64, PlayerBotCharBaseInfo>;
	CharInfoMap characters;

	PlayerBotBaseInfo(uint32 uid, const char *name, std::string &pa, bool isAcc) :
		isAccountInfo(isAcc), id(uid), pass(pa)
	{
		characters.clear();
		username = name;
	}

	bool MatchRaceByFuction(bool fuction, uint16 race)
	{
		return PlayerBotRaceTeam(race) == (fuction ? TEAM_ALLIANCE : TEAM_HORDE);
	}
	bool ExistClass(bool fuction, uint16 prof)
	{
#ifdef INCOMPLETE_BOT
		if (prof != 1 && prof != 5 && prof != 9)
			return false;
#endif
		for (CharInfoMap::iterator it = characters.begin();
			it != characters.end();
			it++)
		{
			if (it->second.profession == prof)
			{
				uint16 race = it->second.race;
				if (MatchRaceByFuction(fuction, race))
					return true;
			}
		}
		return false;
	}

	PlayerBotCharBaseInfo& GetRandomCharacterByFuction(bool faction)
	{
		if (characters.size() <= 0)
		{
			return empty;
		}
#ifdef INCOMPLETE_BOT
		for (int i = 0; i < 20; i++)
#else
		for (int i = 0; i < 5; i++)
#endif
		{
			int16 select = irand(0, characters.size() / 2 - 1);
			for (CharInfoMap::iterator it = characters.begin();
				it != characters.end();
				it++)
			{
#ifdef INCOMPLETE_BOT
				if (it->second.profession != 1 && it->second.profession != 5 && it->second.profession != 9)
					continue;
#endif
				if (MatchRaceByFuction(faction, it->second.race))
				{
					if (select <= 0)
						return it->second;
					else
						--select;
				}
			}
		}
		return characters.begin()->second;
	}

	std::vector<uint32> GetNoArenaTeamCharacterIDsByFuction(bool faction, ArenaTeamTypes type)
	{
		std::vector<uint32> outIDs;
		if (characters.size() <= 0)
		{
			return outIDs;
		}
		for (CharInfoMap::iterator it = characters.begin();
			it != characters.end();
			it++)
		{
#ifdef INCOMPLETE_BOT
			if (it->second.profession != 1 && it->second.profession != 5 && it->second.profession != 9)
				continue;
#endif
			if (MatchRaceByFuction(faction, it->second.race))
			{
				//if (sArenaTeamMgr->ExistArenaTeamByType(ObjectGuid(uint64(it->second.guid)), type))
				//	continue;
				outIDs.push_back(it->second.guid);
			}
		}
		Trinity::Containers::RandomShuffle(outIDs);
		//unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		//std::shuffle(outIDs.begin(), outIDs.end(), std::default_random_engine(time(NULL)));
		return outIDs;
	}

	PlayerBotCharBaseInfo& GetCharacter(bool faction, uint32 prof)
	{
		if (characters.size() <= 0)
			return empty;
		for (CharInfoMap::iterator it = characters.begin();
			it != characters.end();
			it++)
		{
			if (!MatchRaceByFuction(faction, it->second.race))
				continue;
			if (it->second.profession == prof)
				return it->second;
		}
		return empty;
	}

	uint32 GetCharIDByNoArenaType(bool faction, uint32 prof, uint32 arenaType, std::vector<ObjectGuid>& fliters);

	bool ExistCharacterByGUID(ObjectGuid& guid)
	{
		for (CharInfoMap::iterator it = characters.begin();
			it != characters.end();
			it++)
		{
			if (it->second.guid == guid.GetCounter())
				return true;
		}
		return false;
	}

	TeamId GetTeamIDByChar(ObjectGuid& guid)
	{
		uint32 id = guid.GetCounter();
		for (CharInfoMap::iterator it = characters.begin();
			it != characters.end();
			it++)
		{
			if (it->second.guid != id)
				continue;
			return it->second.GetCamp();
		}
		return TeamId::TEAM_NEUTRAL;
	}

	std::string GetCharNameANDClassesText(ObjectGuid& guid)
	{
		uint32 id = guid.GetCounter();
		for (CharInfoMap::iterator it = characters.begin();
			it != characters.end();
			it++)
		{
			if (it->second.guid != id)
				continue;
			return it->second.GetNameANDClassesText();
		}
		return "";
	}

	bool RemoveCharacterByGUID(ObjectGuid& guid)
	{
		uint32 id = guid.GetCounter();
		for (CharInfoMap::iterator it = characters.begin();
			it != characters.end();
			it++)
		{
			if (it->second.guid != id)
				continue;
			characters.erase(it);
			return true;
		}
		return false;
	}
};

using BattlegroundTypeId = uint16;
class PlayerBotMgr
{
	typedef std::vector<BattlegroundTypeId> BattleGroundTypes;

private:
	PlayerBotMgr();
	~PlayerBotMgr();

public:
	PlayerBotMgr(PlayerBotMgr const&) = delete;
	PlayerBotMgr(PlayerBotMgr&&) = delete;

	PlayerBotMgr& operator= (PlayerBotMgr const&) = delete;
	PlayerBotMgr& operator= (PlayerBotMgr&&) = delete;

	static PlayerBotMgr* instance();
	static void SwitchPlayerBotAI(Player* player, PlayerBotAIType aiType, bool force);

	std::string GetPlayerLinkText(Player const* player) const;

	std::string RandomArenaName();
	PlayerBotSession* GetBotSessionByCharGUID(ObjectGuid& guid);
	TeamId GetTeamIDByPlayerBotGUID(ObjectGuid& guid);
	bool IsPlayerBot(WorldSession *pSession);
	bool IsBotAccuntName(std::string name);
	bool IsIDLEPlayerBot(Player* player);
	void DestroyBotMail(uint32 guid);
	void LoadPlayerBotBaseInfo();
	void AddNewAccountBotBaseInfo(std::string name);
	std::set<uint32> GetArenaTeamPlayerBotIDCountByTeam(TeamId team, int32 count, ArenaTeamTypes type);
	PlayerBotBaseInfo* GetPlayerBotAccountInfo(uint32 guid);
	PlayerBotBaseInfo* GetAccountBotAccountInfo(uint32 guid);

	void UpdateLastAccountIndex(std::string& username);
	void UpAllPlayerBotSession();

	void OnPlayerBotCreate(ObjectGuid const& guid, uint32 accountId, std::string const& name, uint8 gender, uint8 race, uint8 playerClass, uint8 level);
	void OnAccountBotCreate(ObjectGuid const& guid, uint32 accountId, std::string const& name, uint8 gender, uint8 race, uint8 playerClass, uint8 level);
	void OnAccountBotDelete(ObjectGuid& guid, uint32 accountId);
	void OnPlayerBotLogin(WorldSession* pSession, Player* pPlayer);
	void OnPlayerBotLogout(WorldSession* pSession);
	void OnPlayerBotLeaveOriginalGroup(Player* pPlayer);
	void LoginGroupBotByPlayer(Player* pPlayer);
	void LoginFriendBotByPlayer(Player* pPlayer);
	void LogoutAllGroupPlayerBot(Group* pGroup, bool force);

	void AllPlayerBotRandomLogin(const char* name = "");
	void AllPlayerBotLogout();
	bool PlayerBotLogout(uint32 account);
	bool AllPlayerLeaveBG(uint32 account);
	void SupplementPlayerBot();

	void OnRealPlayerJoinBattlegroundQueue(uint32 bgTypeId, uint32 level);
	void OnRealPlayerLeaveBattlegroundQueue(uint32 bgTypeId, uint32 level);
	void OnRealPlayerLeaveArenaQueue(uint32 bgTypeId, uint32 level, uint32 aaType);
	void OnRealPlayerEnterBattleground(uint32 bgTypeId, uint32 level);
	void OnRealPlayerLeaveBattleground(const Player* player);

	void Update();
	uint32 GetOnlineBotCount(TeamId team, bool hasReal);
	uint32 GetOnlineBotCount2(TeamId team, bool hasReal);

	bool LoginBotByAccountIndex(uint32 account, uint32 index);
	void DelayLoginPlayerBotByGUID(ObjectGuid& guid) { m_DelayOnlineBots.push(guid); }
	bool AddNewPlayerBotByGUID(ObjectGuid& guid);
	bool AddNewPlayerBotByGUID2(ObjectGuid& guid);
	void AddNewPlayerBot(bool faction, Classes prof, uint32 count);
	void AddNewAccountBot(bool faction, Classes prof);
	void AddNewPlayerBotByClass(uint32 count, Classes prof);
	bool ChangePlayerBotSetting(uint32 account, uint32 minLV, uint32 maxLV, uint32 talent);
	lfg::LfgRoles GetPlayerBotCurrentLFGRoles(Player* player);
	ObjectGuid GetNoArenaMatchCharacter(TeamId team, uint32 arenaType, Classes cls, std::vector<ObjectGuid>& fliters);
	std::string GetNameANDClassesText(ObjectGuid& guid);
	bool CanReadyArenaByArenaTeamID(uint32 arenaTeamId);
    void SetMax(int max) { m_MaxOnlineBot = max; }
    int32 m_MaxOnlineBot;
    int32 m_BotOnlineCount;

private:
	bool ExistClassByRace(uint8 race, uint8 prof);
	void InitializeCreatePlayerBotName();
	std::string RandomName(LocaleConstant locale);
	uint8 RandomRace(bool alliance, uint8 prof);
	uint8 RandomSkinColor(uint8 race, uint8 gender, uint8 prof);
	uint8 RandomFace(uint8 race, uint8 gender, uint8 skinColor, uint8 prof);
	uint8 RandomHair(uint8 race, uint8 gender, uint8 prof);
	uint8 RandomHairColor(uint8 race, uint8 gender, uint8 hairID, uint8 prof);
	uint8 RandomFacialHair(uint8 race, uint8 gender, uint8 hairColor, uint8 prof);
	PlayerBotSession* UpPlayerBotSessionByBaseInfo(PlayerBotBaseInfo *pAcc, bool accountInfo);
	std::shared_ptr<WorldPackets::Character::CharacterCreateInfo> BuildCreatePlayerData(bool alliance, uint8 prof, LocaleConstant locale);
	void CreateOncePlayerBot();

	void ClearBaseInfo();
	void SupplementAccount();
	void LoadCharBaseInfo();
	void LoadSessionPermissionsCallback(PreparedQueryResult result);

	void ClearEmptyNeedPlayer();
	void ClearNeedPlayer(uint32 bgTypeID, uint32 bracketID);
	void AddNewPlayerBotToBG(TeamId team, uint32 minLV, uint32 maxLV, BattlegroundTypeId bgTypeID);
	void AddNewPlayerBotToLFG(lfg::LFGBotRequirement* botRequirement);
	void AddNewPlayerBotToAA(TeamId team, BattlegroundTypeId bgTypeID, uint32 bracketID, uint32 aaType);
	void AddTeamBotToRatedArena(uint32 arenaType);   ///< bracket size: 2 or 3
	bool FillOnlineBotScheduleByLFGRequirement(lfg::LFGBotRequirement* botRequirement, BotGlobleSchedule* botSchedule);
	uint32 GetScheduleTalentByLFGRequirement(lfg::LfgRoles roles, uint32 botCls);
	void QueryBattlegroundRequirement();
	void QueryRatedArenaRequirement();
	void QueryNonRatedArenaRequirement();
	void OnlinePlayerBotByGUIDQueue();
	bool ExistUnBGPlayerBot();
	PVPDifficultyEntry const* FindBGBracketEntry(Battleground* bg_template, uint32 level);

private:
	uint32 m_BotAccountAmount;
	uint32 m_LastBotAccountIndex;
	uint32 m_LFGSearchTick;
	uint32 m_ArenaSearchTick;
	std::map<uint32, PlayerBotBaseInfo*> m_idPlayerBotBase;
	std::map<uint32, PlayerBotBaseInfo*> m_idAccountBotBase;

	BattleGroundTypes m_BGTypes;
	std::vector<std::string> allName;
	std::vector<std::string> allArenaName;
	std::queue<ObjectGuid> m_DelayOnlineBots;

	// Characters still to be created on bot accounts, see SupplementPlayerBot.
	struct BotCreateRequest
	{
		uint32 account;
		bool alliance;
		uint8 cls;
	};
	std::queue<BotCreateRequest> m_CreateRequests;
	std::set<std::string> m_HandedOutNames;   ///< names given to a creation that may not be saved yet
	uint32 m_CreateIssued;

public:
	static std::map<uint32, std::list<UnitAI*> > m_DelayDestroyAIs;
	static std::mutex g_uniqueLock;
};

#define sPlayerBotMgr PlayerBotMgr::instance()

#endif // __PLAYERBOTMGR_H__
