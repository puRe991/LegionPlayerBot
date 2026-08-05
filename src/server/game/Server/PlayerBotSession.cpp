
#include "PlayerBotSession.h"
#include "Player.h"
#include "BattlegroundMgr.h"
#include "BotAI.h"
#include "BotGroupAI.h"
#include "BotFieldAI.h"
#include "LFGMgr.h"
#include "CharacterPackets.h"
#include "DB2Structure.h"
#include "LFGPacketsCommon.h"
#include "Group.h"

PlayerBotSession::PlayerBotSession(uint32 id, std::string &name, AccountTypes sec, uint8 expansion, time_t mute_time, LocaleConstant locale, uint32 recruiter, bool isARecruiter) :
m_LastCastTime(CAST_SCHEDULE_TICK),
m_NoWorldTick(0),
m_AccountBot(false),
WorldSession(id, std::move(name), nullptr, SEC_PLAYER, 6, 0, "Wn64", LOCALE_zhCN, 0, false, AT_AUTH_FLAG_NONE, 0)
{
    SetAddress("playbot");
}

bool PlayerBotSession::IsBotSession()
{
	return true;
}

bool PlayerBotSession::HasBGSchedule()
{
	for (BotSchedules::iterator itSc = m_Schedules.begin(); itSc != m_Schedules.end(); itSc++)
	{
		BotGlobleScheduleType bgsType = (*itSc).bbgType;
		if (bgsType == BGSType_EnterBG || bgsType == BGSType_InBGQueue ||
			bgsType == BGSType_LeaveBG || bgsType == BGSType_OutBGQueue ||
			bgsType == BGSType_EnterAA || bgsType == BGSType_InAAQueue ||
			bgsType == BGSType_LeaveAA || bgsType == BGSType_OutAAQueue)
			return true;
	}
	return false;
}

bool PlayerBotSession::Update(uint32 diff, Map *map)
{
	bool updateResult = WorldSession::Update(diff, map);
	ProcessNoWorld(diff);
	m_LastCastTime -= diff;
	if (m_LastCastTime <= 0)
	{
		//std::lock_guard<std::mutex> lock(m_optQueueLock);
		CastSchedule(CAST_SCHEDULE_TICK);
		m_LastCastTime = CAST_SCHEDULE_TICK;
	}
	return updateResult;
}

void PlayerBotSession::PushScheduleToQueue(BotGlobleSchedule& schedule)
{
	//std::lock_guard<std::mutex> lock(m_optQueueLock);
	for (BotSchedules::iterator itSc = m_Schedules.begin(); itSc != m_Schedules.end(); itSc++)
	{
		if ((*itSc).bbgType == schedule.bbgType)
			return;
	}
	if (schedule.bbgType == BGSType_DelayLevelup)
	{
		if (PlayerLoading())
			return;
		if (Player* player = GetPlayer())
		{
			if (!player->IsSettingFinish())
				return;
		}
		else
			return;
	}

	if (schedule.bbgType == BotGlobleScheduleType::BGSType_OutBGQueue)
	{
		RemoveScheduleByType(BotGlobleScheduleType::BGSType_InBGQueue);
		RemoveScheduleByType(BotGlobleScheduleType::BGSType_EnterBG);
	}
	if (schedule.bbgType == BotGlobleScheduleType::BGSType_OutAAQueue)
	{
		RemoveScheduleByType(BotGlobleScheduleType::BGSType_InAAQueue);
		RemoveScheduleByType(BotGlobleScheduleType::BGSType_EnterAA);
	}
	m_Schedules.push_back(schedule);
}

void PlayerBotSession::RemoveScheduleByType(BotGlobleScheduleType eType)
{
	for (BotSchedules::iterator itSc = m_Schedules.begin(); itSc != m_Schedules.end(); itSc++)
	{
		if ((*itSc).bbgType == eType)
		{
			m_Schedules.erase(itSc);
			return;
		}
	}
}

bool PlayerBotSession::HasScheduleByType(BotGlobleScheduleType eType)
{
	for (BotSchedules::iterator itSc = m_Schedules.begin(); itSc != m_Schedules.end(); itSc++)
	{
		if ((*itSc).bbgType == eType)
		{
			return true;
		}
	}
	return false;
}

bool PlayerBotSession::HasSchedules()
{
	return m_Schedules.size() > 0;
}

bool PlayerBotSession::IsAccountBotSession()
{
	return m_AccountBot;
}

bool PlayerBotSession::PlayerIsReady()
{
	Player* player = GetPlayer();
	if (!player)
		return true;
	if (!player->IsSettingFinish())
		return false;
	BotFieldAI* pFieldAI = dynamic_cast<BotFieldAI*>(player->GetAI());
	if (!pFieldAI)
		return true;
	if (pFieldAI->HasTeleport())
		return false;

	return true;
}

void PlayerBotSession::ProcessNoWorld(uint32 diff)
{
	if (PlayerLoading())
		return;
	Player* player = GetPlayer();
	if (!player)
		return;

	if (player->IsInWorld())
	{
		m_NoWorldTick = 0;
		return;
	}

	if (player->IsBeingTeleported())
	{
        HandleWorldPortAck();
        m_NoWorldTick = 1000;
		return;
	}

	if (m_NoWorldTick == 0)
	{
		m_NoWorldTick = 2000;
		return;
	}

	m_NoWorldTick -= int32(diff);
	if (m_NoWorldTick > 0)
		return;
	if (BotGroupAI* pGroupAI = dynamic_cast<BotGroupAI*>(player->GetAI()))
	{
		if (pGroupAI->HasTeleport())
			pGroupAI->UpdateTeleport(diff);
		else
			pGroupAI->SetTeleportToMaster();
		m_NoWorldTick = 500;
	}
	else if (BotBGAI* pGroupAI = dynamic_cast<BotBGAI*>(player->GetAI()))
	{
		if (player->InBattleground())
		{
			PlayerBotMgr::SwitchPlayerBotAI(player, PlayerBotAIType::PBAIT_FIELD, true);
			WorldPacket opcode(CMSG_BATTLEFIELD_LEAVE);
			WorldPackets::Battleground::NullCmsg msg(std::move(opcode));
			HandleLeaveBattlefield(msg);
		}
		HandleWorldPortAck();
		m_NoWorldTick = 500;
	}
	else// if (BotFieldAI* pGroupAI = dynamic_cast<BotFieldAI*>(player->GetAI()))
	{
		HandleWorldPortAck();
		m_NoWorldTick = 500;
	}
}

void PlayerBotSession::CastSchedule(uint32 diff)
{
	if (m_Schedules.empty())
		return;
	bool result = false;
	BotGlobleSchedule& schedule = *m_Schedules.begin();
	schedule.processTick += diff;
	if (schedule.processTick > 60000 * 3)
	{
		m_Schedules.erase(m_Schedules.begin());
		return;
	}
	if (!PlayerIsReady())
		return;
	switch (schedule.bbgType)
	{
	case BGSType_Online:
		result = ProcessOnline(schedule);
		break;
	case BGSType_Online_GUID:
		result = ProcessOnlineByGUID(schedule);
		break;
	case BGSType_Offline:
		result = ProcessOffline(schedule);
		break;
	case BGSType_Settting:
		result = ProcessSetting(schedule);
		break;
	case BGSType_InBGQueue:
		result = ProcessInBGQueue(schedule);
		break;
	case BGSType_OutBGQueue:
		result = ProcessOutBGQueue(schedule);
		break;
	case BGSType_EnterBG:
		result = ProcessEnterBG(schedule);
		break;
	case BGSType_LeaveBG:
		result = ProcessLeaveBG(schedule);
		break;
	case BGSType_InAAQueue:
		result = ProcessInAAQueue(schedule);
		break;
	case BGSType_OutAAQueue:
		result = ProcessOutAAQueue(schedule);
		break;
	case BGSType_EnterAA:
		result = ProcessEnterAA(schedule);
		break;
	case BGSType_LeaveAA:
		result = ProcessLeaveAA(schedule);
		break;
	case BGSType_DelayLevelup:
		result = ProcessDelayLevelup(schedule);
		break;
	case BGSType_InLFGQueue:
		result = ProcessInLFGQueue(schedule);
		break;
	case BGSType_OutLFGQueue:
		result = ProcessOutLFGQueue(schedule);
		break;
	case BGSType_AcceptLFGProposal:
		result = ProcessAcceptLFGProposal(schedule);
		break;
	case BGSType_OfferPetitionSign:
		result = ProcessOfferPetitionSign(schedule);
		break;
	default:
		result = true;
		break;
	}
	if (result)
	{
		m_Schedules.erase(m_Schedules.begin());
	}
}

bool PlayerBotSession::ProcessOnline(BotGlobleSchedule& schedule)
{
	if (sPlayerBotMgr->m_MaxOnlineBot <= sPlayerBotMgr->m_BotOnlineCount)
	{
		LogoutPlayer(false, "playerbot max");
		return true;
	}

	if (schedule.parameter1 <= 0)
		return true;
	if (PlayerLoading())
		return false;
	if (GetPlayer())
		return true;
	PlayerBotBaseInfo* pInfo = sPlayerBotMgr->GetPlayerBotAccountInfo(GetAccountId());
	if (!pInfo)
	{
		pInfo = sPlayerBotMgr->GetAccountBotAccountInfo(GetAccountId());
		if (!pInfo)
		{
			ClearAllSchedule();
			return false;
		}
	}

	bool fuction = true;
	if (schedule.parameter1 > 1)
		fuction = false;
	PlayerBotCharBaseInfo& charInfo = (schedule.parameter2 == 0) ? pInfo->GetRandomCharacterByFuction(fuction) : pInfo->GetCharacter(fuction, schedule.parameter2);
	if (charInfo.guid == 0)
		return true;

    WorldPacket _worldPacket(CMSG_PLAYER_LOGIN);
    WorldPackets::Character::PlayerLogin cmd(std::move(_worldPacket));
    cmd.Guid = ObjectGuid::Create<HighGuid::Player>(charInfo.guid);
    cmd.FarClip = 0.0f;
    HandlePlayerLoginOpcode(cmd);
	HandleContinuePlayerLogin();
	return false;
}

bool PlayerBotSession::ProcessOnlineByGUID(BotGlobleSchedule& schedule)
{
    if (sPlayerBotMgr->m_MaxOnlineBot <= sPlayerBotMgr->m_BotOnlineCount)
    {
        LogoutPlayer(false, "playerbot max");
        return true;
    }

	if (schedule.playerGUID.IsEmpty())
		return true;
	if (PlayerLoading())
		return false;
	if (GetPlayer())
		return true;
    WorldPacket _worldPacket(CMSG_PLAYER_LOGIN);
    WorldPackets::Character::PlayerLogin cmd(std::move(_worldPacket));
    cmd.Guid = schedule.playerGUID;
    cmd.FarClip = 0.0f;
    HandlePlayerLoginOpcode(cmd);
	HandleContinuePlayerLogin();
	return false;
}

bool PlayerBotSession::ProcessOffline(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
		return true;
	if (schedule.scheduleState > 0)
		return false;
	LogoutRequest(time(NULL) - 18);
	//LogoutPlayer(false);
	schedule.scheduleState = 1;
	return false;
}

bool PlayerBotSession::ProcessSetting(BotGlobleSchedule& schedule)
{
	if (schedule.parameter3 == 0 || schedule.parameter3 > 4)
		schedule.parameter3 = 4;
    if (schedule.parameter2 > 110 || schedule.parameter2 == 0)
        schedule.parameter2 = 110;
	if (schedule.parameter1 > schedule.parameter2)
		schedule.parameter2 = schedule.parameter1;
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->IsInWorld() || !player->IsSettingFinish())
		return false;
	if (schedule.scheduleState != 0)
		return true;
	bool needTenacity = (schedule.parameter4 != 0) ? true : false;
	if (needTenacity)
		needTenacity = player->CheckNeedTenacityFlush();
	if (!needTenacity && player->CalculateTalentsPoints() < 10)
	{
		if (player->IsSettingFinish() && player->getLevel() >= schedule.parameter1 && player->getLevel() <= schedule.parameter2)
		{
			if (schedule.parameter3 >= 4 || (player->FindTalentType() + 1 == schedule.parameter3))
				return true;
		}
	}

	uint32 flushTalent = (schedule.parameter3 > 0 && schedule.parameter3 < 4) ? schedule.parameter3 - 1 : 3;
	player->ResetPlayerToLevel(schedule.parameter2, flushTalent, needTenacity);
	schedule.scheduleState = 1;
	return false;
}

bool PlayerBotSession::ProcessInBGQueue(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->IsInWorld())
		return false;
	if (player->InBattlegroundQueue())
		return true;
	if (player->InBattleground() || player->InArena() || player->GetMap()->IsDungeon())
	{
		ClearAllSchedule();
		return false;
	}

	Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BattlegroundTypeId(schedule.parameter1));
	if (!bg)
	{
		ClearAllSchedule();
		return false;
	}
	PVPDifficultyEntry const* bracketEntry = DB2Manager::GetBattlegroundBracketByLevel(bg->GetMapId(), player->getLevel());
	if (!bracketEntry)
	{
		ClearAllSchedule();
		return false;
	}

	WorldPacket cmd(CMSG_BATTLEMASTER_JOIN);
	WorldPackets::Battleground::Join packet(std::move(cmd));
	packet.QueueID = MS::Battlegrounds::QueueOffsets::Battleground + MS::Battlegrounds::BattlegroundTypeId::BattlegroundRandom;
	packet.RolesMask = 14;
	HandleBattlemasterJoin(packet);
	return false;
}

bool PlayerBotSession::ProcessOutBGQueue(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->IsInWorld() || player->InBattleground() || player->InArena() || !player->InBattlegroundQueue() || player->GetMap()->IsDungeon())
		return true;

	LogoutPlayer(false, "playerbot");
	return false;
}

bool PlayerBotSession::ProcessEnterBG(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->IsInWorld())
		return false;

	if (player->isInCombat())
		player->CombatStop(true);

	if (player->GetMap()->IsDungeon())
	{
		BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_OutBGQueue, 0);
		schedule1.parameter1 = schedule.parameter1;
		ClearAllSchedule();
		PushScheduleToQueue(schedule1);
		return false;
	}
	if (player->InBattleground() || player->InArena())
		return true;

	if (player->InBattlegroundQueue())
	{
		for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
		{
			uint8 bgQueueTypeId = player->GetBattlegroundQueueTypeId(i);
			if (!bgQueueTypeId)
				continue;
			if (player->IsInvitedForBattlegroundQueueType(bgQueueTypeId))
			{
				PlayerBotMgr::SwitchPlayerBotAI(player, PlayerBotAIType::PBAIT_BG, true);

                WorldPacket cmd(CMSG_BATTLEFIELD_PORT);
                WorldPackets::Battleground::Port packet(std::move(cmd));
				packet.Ticket.Id = schedule.parameter1;
				packet.Ticket.RequesterGuid = schedule.playerGUID;
				packet.Ticket.Type = WorldPackets::LFG::RideType::Battlegrounds;
				packet.Ticket.Time = time(0);
				packet.AcceptedInvite = true;
                HandleBattleFieldPort(packet);
				//HandleWorldPortAck();
				break;
			}
		}
	}
	return false;
}

bool PlayerBotSession::ProcessLeaveBG(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->InBattleground())
		return true;

	PlayerBotMgr::SwitchPlayerBotAI(player, PlayerBotAIType::PBAIT_FIELD, true);

	WorldPackets::Battleground::NullCmsg cmd(std::move(WorldPacket()));
	HandleLeaveBattlefield(cmd);
	//HandleWorldPortAck();
	return false;
}

bool PlayerBotSession::ProcessInAAQueue(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->IsInWorld())
		return false;
	if (player->InBattlegroundQueue())
		return true;
	if (player->HasAura(26013) || player->InBattleground() || player->InArena() || player->GetMap()->IsDungeon())
	{
		ClearAllSchedule();
		return false;
	}

	// parameter3 != 0 selects rated arena. Rated goes through JoinBracket, which
	// requires the bot to lead a group of the right size -- PlayerBotMgr forms
	// that group before pushing this schedule. Unrated stays on the skirmish
	// path, which a single bot can take on its own.
	if (schedule.parameter3)
	{
		Group* group = player->GetGroup();
		if (!group || group->GetLeaderGUID() != player->GetGUID())
		{
			ClearAllSchedule();
			return false;
		}

		WorldPacket packet(CMSG_BATTLEMASTER_JOIN_ARENA);
		WorldPackets::Battleground::JoinArena cmd(std::move(packet));
		cmd.TeamSizeIndex = uint8(schedule.parameter2);
		cmd.Roles = uint8(schedule.parameter1);
		HandleBattlemasterJoinArena(cmd);
		return false;
	}

	WorldPacket packet(CMSG_BATTLEMASTER_JOIN_SKIRMISH);
	WorldPackets::Battleground::JoinSkirmish cmd(std::move(packet));
	cmd.RolesMask = schedule.parameter1;
	cmd.Bracket = schedule.parameter2;
	HandleJoinSkirmish(cmd);
	return false;
}

bool PlayerBotSession::ProcessOutAAQueue(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->IsInWorld() || player->InBattleground() || player->InArena() || !player->InBattlegroundQueue() || player->GetMap()->IsDungeon())
		return true;

	LogoutPlayer(false, "playerbot");
	return true;
}

bool PlayerBotSession::ProcessEnterAA(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->IsInWorld())
		return false;
	if (player->GetMap()->IsDungeon())
	{
		BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_OutAAQueue, 0);
		schedule1.parameter1 = schedule.parameter1;
		schedule1.parameter2 = schedule.parameter2;
		schedule1.parameter3 = schedule.parameter3;
		ClearAllSchedule();
		PushScheduleToQueue(schedule1);
		return false;
	}
	if (player->InBattleground() || player->InArena())
		return true;

	if (player->InBattlegroundQueue())
	{
		for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
		{
			uint8 bgQueueTypeId = player->GetBattlegroundQueueTypeId(i);
			if (!bgQueueTypeId)
				continue;
			if (player->IsInvitedForBattlegroundQueueType(bgQueueTypeId))
			{
				PlayerBotMgr::SwitchPlayerBotAI(player, PlayerBotAIType::PBAIT_ARENA, true);

                WorldPacket cmd(CMSG_BATTLEFIELD_PORT);
                WorldPackets::Battleground::Port packet(std::move(cmd));
                packet.Ticket.Id = schedule.parameter1;
                packet.Ticket.RequesterGuid = schedule.playerGUID;
                packet.Ticket.Type = WorldPackets::LFG::RideType::Battlegrounds;
                packet.Ticket.Time = time(0);
                packet.AcceptedInvite = true;
                HandleBattleFieldPort(packet);
				break;
			}
			else
			{
				Battleground* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(BattlegroundTypeId(schedule.parameter1));
				if (!bg_template)
					continue;
				BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
				PVPDifficultyEntry const* bracketEntry = DB2Manager::GetBattlegroundBracketById(bg_template->GetMapId(), uint8(schedule.parameter2));
				if (bgQueue.ExistRealPlayer(bracketEntry, (schedule.parameter4 != 0) ? true : false))
					continue;
				BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_OutAAQueue, 0);
				schedule1.parameter1 = schedule.parameter1;
				schedule1.parameter2 = schedule.parameter2;
				schedule1.parameter3 = schedule.parameter3;
				ClearAllSchedule();
				PushScheduleToQueue(schedule1);
				return false;
			}
		}
	}
	return false;
}

bool PlayerBotSession::ProcessLeaveAA(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->InBattleground())
		return true;

	PlayerBotMgr::SwitchPlayerBotAI(player, PlayerBotAIType::PBAIT_FIELD, true);
	//uint8 un8 = 0;
	//uint16 un16 = 0;
	//uint32 un32 = 0;
	//WorldPacket opcode(1);
	//opcode << un8;
	//opcode << un8;
	//opcode << un32;
	//opcode << un16;
	//HandleBattlefieldLeaveOpcode(opcode);
	//HandleMoveWorldportAckOpcode();
    WorldPackets::Battleground::NullCmsg cmd(std::move(WorldPacket()));
    HandleLeaveBattlefield(cmd);
    //HandleWorldPortAck();
	return false;
}

bool PlayerBotSession::ProcessDelayLevelup(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	player->OnLevelupToBotAI();
	return true;
}

// Queue the bot for the dungeon finder.
//
//   parameter1  role mask; 0 means "derive it from the active specialisation"
//   parameter2  number of dungeon ids supplied in parameter3..5;
//               0 means "pick every dungeon the bot is eligible for"
//   parameter3
//   parameter4  explicit dungeon ids, used when parameter2 is 1..3
//   parameter5
//
// The old body built a hand-rolled packet against an opcode layout that no
// longer exists. sLFGMgr is called directly instead -- that is what
// WorldSession::HandleLfgJoin does once the packet is parsed, so the bot
// takes the same path as a real client without going through serialisation.
bool PlayerBotSession::ProcessInLFGQueue(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->IsInWorld())
		return false;
	if (player->isUsingLfg())
		return true;
	if (player->InBattleground() || player->InArena() || player->InBattlegroundQueue())
	{
		ClearAllSchedule();
		return false;
	}
	if (!sLFGMgr->isOptionEnabled(lfg::LFG_OPTION_ENABLE_DUNGEON_FINDER))
	{
		ClearAllSchedule();
		return false;
	}
	if (schedule.parameter2 > 3)
		return true;

	uint8 roles = schedule.parameter1 ? uint8(schedule.parameter1)
	                                  : uint8(player->GetSpecializationRoleMaskForGroup());
	if (!(roles & (lfg::PLAYER_ROLE_TANK | lfg::PLAYER_ROLE_HEALER | lfg::PLAYER_ROLE_DAMAGE)))
		roles |= lfg::PLAYER_ROLE_DAMAGE;

	lfg::LfgDungeonSet dungeons;
	if (schedule.parameter2 == 0)
	{
		dungeons = sLFGMgr->GetRewardableDungeons(player->getLevel(), player->GetSession()->Expansion());
		if (dungeons.empty())
		{
			ClearAllSchedule();
			return false;
		}
	}
	else
	{
		if (schedule.parameter2 >= 1 && schedule.parameter3)
			dungeons.insert(schedule.parameter3);
		if (schedule.parameter2 >= 2 && schedule.parameter4)
			dungeons.insert(schedule.parameter4);
		if (schedule.parameter2 >= 3 && schedule.parameter5)
			dungeons.insert(schedule.parameter5);
		if (dungeons.empty())
			return true;
	}

	sLFGMgr->JoinLfg(player, roles, dungeons);
	return false;
}

bool PlayerBotSession::ProcessOutLFGQueue(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->IsInWorld())
		return false;
	if (!player->isUsingLfg())
		return true;

	// queue id 0 leaves every queue the bot sits in
	sLFGMgr->LeaveLfg(player->GetGUID(), schedule.parameter1);
	return true;
}

// parameter1  proposal id
// parameter2  non-zero accepts, zero declines
bool PlayerBotSession::ProcessAcceptLFGProposal(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	if (schedule.parameter1 == 0)
		return true;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}
	if (!player->isUsingLfg())
		return true;

	WorldPackets::LFG::ProposalResponse response;
	response.ProposalID = schedule.parameter1;
	response.InstanceID = 0;
	response.Accepted = schedule.parameter2 != 0;
	response.Ticket.RequesterGuid = player->GetGUID();
	response.Ticket.Id = schedule.parameter1;
	response.Ticket.Type = WorldPackets::LFG::RideType::Lfg;
	response.Ticket.Time = int32(time(nullptr));

	sLFGMgr->UpdateProposal(response, player->GetGUID());
	return true;
}

bool PlayerBotSession::ProcessOfferPetitionSign(BotGlobleSchedule& schedule)
{
	if (PlayerLoading())
		return false;
	if (schedule.parameter1 == 0 && schedule.parameter2 == 0)
		return true;
	Player* player = GetPlayer();
	if (!player)
	{
		ClearAllSchedule();
		return false;
	}

	uint64 signGUID = uint64(schedule.parameter1);
	uint64 highID = uint64(schedule.parameter2) << 32;
	signGUID |= highID;
	//WorldPacket cmd(1);
	//cmd << ObjectGuid(signGUID);
	//cmd << uint8(1);
	//HandlePetitionSignOpcode(cmd);
	return true;
}
