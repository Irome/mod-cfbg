/*
 * Copyright (С) since 2019 Andrei Guluaev (Winfidonarleyan/Kargatum) https://github.com/Winfidonarleyan 
 * Licence MIT https://opensource.org/MIT
 */

#include "CFBG.h"
#include "Config.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "GroupMgr.h"
#include "BattlegroundMgr.h"
#include "Opcodes.h"
#include "Chat.h"

CFBG* CFBG::instance()
{
    static CFBG instance;
    return &instance;
}

void CFBG::LoadConfig()
{
    _IsEnableSystem = sConfigMgr->GetBoolDefault("CFBG.Enable", false);
    _IsEnableAvgIlvl = sConfigMgr->GetBoolDefault("CFBG.Include.Avg.Ilvl.Enable", false);
    _MaxPlayersCountInGroup = sConfigMgr->GetIntDefault("CFBG.Players.Count.In.Group", 3);
}

bool CFBG::IsEnableSystem()
{
    return _IsEnableSystem;
}

bool CFBG::IsEnableAvgIlvl()
{
    return _IsEnableAvgIlvl;
}

uint32 CFBG::GetMaxPlayersCountInGroup()
{
    return _MaxPlayersCountInGroup;
}

uint32 CFBG::GetBGTeamAverageItemLevel(Battleground* bg, TeamId team)
{
    if (!bg)
        return 0;

    uint32 PlayersCount = bg->GetPlayersCountByTeam(team);
    if (!PlayersCount)
        return 0;

    uint32 Sum = 0;
    uint32 Count = 0;

    for (auto itr : bg->GetPlayers())
    {
        Player* player = itr.second;
        if (!player)
            continue;

        if (player->GetTeamId(true) != team)
            continue;

        Sum += player->GetAverageItemLevel();
        Count++;
    }

    if (!Count || !Sum)
        return 0;

    return Sum / Count;
}

TeamId CFBG::GetLowerTeamIdInBG(Battleground* bg)
{
    int32 PlCountA = bg->GetPlayersCountByTeam(TEAM_ALLIANCE);
    int32 PlCountH = bg->GetPlayersCountByTeam(TEAM_HORDE);
    uint32 Diff = abs(PlCountA - PlCountH);

    if (Diff)
        return PlCountA < PlCountH ? TEAM_ALLIANCE : TEAM_HORDE;

    if (IsEnableAvgIlvl() && !IsAvgIlvlTeamsInBgEqual(bg))
        return GetLowerAvgIlvlTeamInBg(bg);

    uint8 rnd = urand(0, 1);

    if (rnd)
        return TEAM_ALLIANCE;

    return TEAM_HORDE;
}

TeamId CFBG::GetLowerAvgIlvlTeamInBg(Battleground* bg)
{
    uint32 AvgAlliance = GetBGTeamAverageItemLevel(bg, TeamId::TEAM_ALLIANCE);
    uint32 AvgHorde = GetBGTeamAverageItemLevel(bg, TeamId::TEAM_HORDE);

    return (AvgAlliance < AvgHorde) ? TEAM_ALLIANCE : TEAM_HORDE;
}

bool CFBG::IsAvgIlvlTeamsInBgEqual(Battleground* bg)
{
    uint32 AvgAlliance = GetBGTeamAverageItemLevel(bg, TeamId::TEAM_ALLIANCE);
    uint32 AvgHorde = GetBGTeamAverageItemLevel(bg, TeamId::TEAM_HORDE);

    return AvgAlliance == AvgHorde;
}

void CFBG::ValidatePlayerForBG(Battleground* bg, Player* player, TeamId teamId)
{
    BGData bgdata = player->GetBGData();
    bgdata.bgTeamId = teamId;
    player->SetBGData(bgdata);

    SetFakeRaceAndMorph(player);

    float x, y, z, o;
    bg->GetTeamStartLoc(teamId, x, y, z, o);
    player->TeleportTo(bg->GetMapId(), x, y, z, o);
}

uint32 CFBG::GetAllPlayersCountInBG(Battleground* bg)
{
    return bg->GetPlayersSize();
}

void CFBG::SetFakeRaceAndMorph(Player* player)
{
    if (!player->InBattleground())
        return;

    if (player->GetTeamId(true) == player->GetBgTeamId())
        return;

    if (IsPlayerFake(player))
        return;

    uint8 FakeRace;
    uint32 FakeMorph;

    if (player->getClass() == CLASS_DRUID)
    {
        if (player->GetTeamId(true) == TEAM_ALLIANCE)
        {
            FakeMorph = player->getGender() == GENDER_MALE ? FAKE_M_TAUREN : FAKE_F_TAUREN;
            FakeRace = RACE_TAUREN;
        }
        else if (player->getGender() == GENDER_MALE) // HORDE PLAYER, ONLY HAVE MALE Night Elf ID
        {
            FakeMorph = FAKE_M_NELF;
            FakeRace = RACE_NIGHTELF;
        }
        else
            FakeRace = player->GetTeamId(true) == TEAM_ALLIANCE ? RACE_BLOODELF : RACE_HUMAN;

        if (player->GetTeamId(true) == TEAM_HORDE)
        {
            if (player->getGender() == GENDER_MALE)
                FakeMorph = 19723;
            else
                FakeMorph = 19724;
        }
        else
        {
            if (player->getGender() == GENDER_MALE)
                FakeMorph = 20578;
            else
                FakeMorph = 20579;
        }
    }
    else if (player->getClass() == CLASS_SHAMAN && player->GetTeamId(true) == TEAM_HORDE && player->getGender() == GENDER_FEMALE)
    {
        FakeMorph = FAKE_F_DRAENEI; // Female Draenei
        FakeRace = RACE_DRAENEI;
    }
    else
    {
        FakeRace = player->GetTeamId(true) == TEAM_ALLIANCE ? RACE_BLOODELF : RACE_HUMAN;

        if (player->GetTeamId(true) == TEAM_HORDE)
        {
            if (player->getGender() == GENDER_MALE)
                FakeMorph = 19723;
            else
                FakeMorph = 19724;
        }
        else
        {
            if (player->getGender() == GENDER_MALE)
                FakeMorph = 20578;
            else
                FakeMorph = 20579;
        }
    }

    TeamId FakeTeamID = player->TeamIdForRace(FakeRace);

    FakePlayer fakePlayer;
    fakePlayer.FakeMorph    = FakeMorph;
    fakePlayer.FakeRace     = FakeRace;
    fakePlayer.FakeTeamID   = FakeTeamID;
    fakePlayer.RealMorph    = player->GetDisplayId();
    fakePlayer.RealRace     = player->getRace(true);
    fakePlayer.RealTeamID   = player->GetTeamId(true);

    _fakePlayerStore[player] = fakePlayer;

    player->setRace(FakeRace);
    SetFactionForRace(player, FakeRace);
    player->SetDisplayId(FakeMorph);
    player->SetNativeDisplayId(FakeMorph);
}

void CFBG::SetFactionForRace(Player* player, uint8 Race)
{
    player->setTeamId(player->TeamIdForRace(Race));

    ChrRacesEntry const* DBCRace = sChrRacesStore.LookupEntry(Race);
    player->setFaction(DBCRace ? DBCRace->FactionID : 0);
}

void CFBG::ClearFakePlayer(Player* player)
{
    if (!IsPlayerFake(player))
        return;

    player->setRace(_fakePlayerStore[player].RealRace);
    player->SetDisplayId(_fakePlayerStore[player].RealMorph);
    player->SetNativeDisplayId(_fakePlayerStore[player].RealMorph);
    SetFactionForRace(player, _fakePlayerStore[player].RealRace);

    _fakePlayerStore.erase(player);
}

bool CFBG::IsPlayerFake(Player* player)
{
    FakePlayersContainer::const_iterator itr = _fakePlayerStore.find(player);
    if (itr != _fakePlayerStore.end())
        return true;

    return false;
}

void CFBG::DoForgetPlayersInList(Player* player)
{
    // m_FakePlayers is filled from a vector within the battleground
    // they were in previously so all players that have been in that BG will be invalidated.
    for (auto itr : _fakeNamePlayersStore)
    {
        WorldPacket data(SMSG_INVALIDATE_PLAYER, 8);
        data << itr.second;
        player->GetSession()->SendPacket(&data);

        if (Player* _player = ObjectAccessor::FindPlayer(itr.second))
            player->GetSession()->SendNameQueryOpcode(_player->GetGUID());
    }
     
    _fakeNamePlayersStore.erase(player);
}

void CFBG::FitPlayerInTeam(Player* player, bool action, Battleground* bg)
{
    if (!bg)
        bg = player->GetBattleground();

    if ((!bg || bg->isArena()) && action)
        return;

    if (action)
        SetForgetBGPlayers(player, true);
    else
        SetForgetInListPlayers(player, true);
}

void CFBG::SetForgetBGPlayers(Player* player, bool value)
{
    _forgetBGPlayersStore[player] = value;
}

bool CFBG::ShouldForgetBGPlayers(Player* player)
{
    return _forgetBGPlayersStore[player];
}

void CFBG::SetForgetInListPlayers(Player* player, bool value)
{
    _forgetInListPlayersStore[player] = value;
}

bool CFBG::ShouldForgetInListPlayers(Player* player)
{
    return _forgetInListPlayersStore[player];
}

void CFBG::DoForgetPlayersInBG(Player* player, Battleground* bg)
{
    for (auto itr : bg->GetPlayers())
    {
        // Here we invalidate players in the bg to the added player
        WorldPacket data1(SMSG_INVALIDATE_PLAYER, 8);
        data1 << itr.first;
        player->GetSession()->SendPacket(&data1);

        if (Player* pPlayer = ObjectAccessor::FindPlayer(itr.first))
        {
            player->GetSession()->SendNameQueryOpcode(pPlayer->GetGUID()); // Send namequery answer instantly if player is available

            // Here we invalidate the player added to players in the bg
            WorldPacket data2(SMSG_INVALIDATE_PLAYER, 8);
            data2 << player->GetGUID();
            pPlayer->GetSession()->SendPacket(&data2);
            pPlayer->GetSession()->SendNameQueryOpcode(player->GetGUID());
        }
    }
}

bool CFBG::SendRealNameQuery(Player* player)
{
    if (IsPlayingNative(player))
        return false;

    WorldPacket data(SMSG_NAME_QUERY_RESPONSE, (8 + 1 + 1 + 1 + 1 + 1 + 10));
    data.appendPackGUID(player->GetGUID());                     // player guid
    data << uint8(0);                                           // added in 3.1; if > 1, then end of packet
    data << player->GetName();                                  // played name
    data << uint8(0);                                           // realm name for cross realm BG usage
    data << uint8(player->getRace(true));
    data << uint8(player->getGender());
    data << uint8(player->getClass());
    data << uint8(0);                                           // is not declined
    player->GetSession()->SendPacket(&data);

    return true;
}

bool CFBG::IsPlayingNative(Player* player)
{
    return player->GetTeamId(true) == player->GetBGData().bgTeamId;
}

bool CFBG::FillPlayersToCFBGWithSpecific(BattlegroundQueue* bgqueue, Battleground* bg, const int32 aliFree, const int32 hordeFree, BattlegroundBracketId thisBracketId, BattlegroundQueue* specificQueue, BattlegroundBracketId specificBracketId)
{
    if (!IsEnableSystem() || bg->isArena() || bg->isRated())
        return false;

    // clear selection pools
    bgqueue->m_SelectionPools[TEAM_ALLIANCE].Init();
    bgqueue->m_SelectionPools[TEAM_HORDE].Init();

    // quick check if nothing we can do:
    if (!sBattlegroundMgr->isTesting() && bgqueue->m_QueuedGroups[thisBracketId][BG_QUEUE_CFBG].empty() && specificQueue->m_QueuedGroups[specificBracketId][BG_QUEUE_CFBG].empty())
        return false;

    // copy groups from both queues to new joined container
    BattlegroundQueue::GroupsQueueType m_QueuedBoth[BG_TEAMS_COUNT];
    m_QueuedBoth[TEAM_ALLIANCE].insert(m_QueuedBoth[TEAM_ALLIANCE].end(), specificQueue->m_QueuedGroups[specificBracketId][BG_QUEUE_CFBG].begin(), specificQueue->m_QueuedGroups[specificBracketId][BG_QUEUE_CFBG].end());
    m_QueuedBoth[TEAM_ALLIANCE].insert(m_QueuedBoth[TEAM_ALLIANCE].end(), bgqueue->m_QueuedGroups[thisBracketId][BG_QUEUE_CFBG].begin(), bgqueue->m_QueuedGroups[thisBracketId][BG_QUEUE_CFBG].end());
    m_QueuedBoth[TEAM_HORDE].insert(m_QueuedBoth[TEAM_HORDE].end(), specificQueue->m_QueuedGroups[specificBracketId][BG_QUEUE_CFBG].begin(), specificQueue->m_QueuedGroups[specificBracketId][BG_QUEUE_CFBG].end());
    m_QueuedBoth[TEAM_HORDE].insert(m_QueuedBoth[TEAM_HORDE].end(), bgqueue->m_QueuedGroups[thisBracketId][BG_QUEUE_CFBG].begin(), bgqueue->m_QueuedGroups[thisBracketId][BG_QUEUE_CFBG].end());

    // ally: at first fill as much as possible
    BattlegroundQueue::GroupsQueueType::const_iterator Ali_itr = m_QueuedBoth[TEAM_ALLIANCE].begin();
    for (; Ali_itr != m_QueuedBoth[TEAM_ALLIANCE].end() && bgqueue->m_SelectionPools[TEAM_ALLIANCE].AddGroup((*Ali_itr), aliFree); ++Ali_itr);

    // horde: at first fill as much as possible
    BattlegroundQueue::GroupsQueueType::const_iterator Horde_itr = m_QueuedBoth[TEAM_HORDE].begin();
    for (; Horde_itr != m_QueuedBoth[TEAM_HORDE].end() && bgqueue->m_SelectionPools[TEAM_HORDE].AddGroup((*Horde_itr), hordeFree); ++Horde_itr);

    // calculate free space after adding
    int32 aliDiff = aliFree - int32(bgqueue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount());
    int32 hordeDiff = hordeFree - int32(bgqueue->m_SelectionPools[TEAM_HORDE].GetPlayerCount());

    // if free space differs too much, ballance
    while (abs(aliDiff - hordeDiff) > 1 && (bgqueue->m_SelectionPools[TEAM_HORDE].GetPlayerCount() > 0 || bgqueue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount() > 0))
    {
        // if results in more alliance players than horde:
        if (aliDiff < hordeDiff)
        {
            // no more alliance in pool, invite whatever we can from horde
            if (!bgqueue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount())
                break;

            // kick alliance, returns true if kicked more than needed, so then try to fill up
            if (bgqueue->m_SelectionPools[TEAM_ALLIANCE].KickGroup(hordeDiff - aliDiff))
                for (; Ali_itr != m_QueuedBoth[TEAM_ALLIANCE].end() && bgqueue->m_SelectionPools[TEAM_ALLIANCE].AddGroup((*Ali_itr), aliFree >= hordeDiff ? aliFree - hordeDiff : 0); ++Ali_itr);
        }
        // if results in more horde players than alliance:
        else
        {
            // no more horde in pool, invite whatever we can from alliance
            if (!bgqueue->m_SelectionPools[TEAM_HORDE].GetPlayerCount())
                break;

            // kick horde, returns true if kicked more than needed, so then try to fill up
            if (bgqueue->m_SelectionPools[TEAM_HORDE].KickGroup(aliDiff - hordeDiff))
                for (; Horde_itr != m_QueuedBoth[TEAM_HORDE].end() && bgqueue->m_SelectionPools[TEAM_HORDE].AddGroup((*Horde_itr), hordeFree >= aliDiff ? hordeFree - aliDiff : 0); ++Horde_itr);
        }

        // recalculate free space after adding
        aliDiff = aliFree - int32(bgqueue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount());
        hordeDiff = hordeFree - int32(bgqueue->m_SelectionPools[TEAM_HORDE].GetPlayerCount());
    }

    return true;
}

bool CFBG::FillPlayersToCFBG(BattlegroundQueue* bgqueue, Battleground* bg, const int32 aliFree, const int32 hordeFree, BattlegroundBracketId bracket_id)
{
    if (!IsEnableSystem() || bg->isArena() || bg->isRated())
        return false;

    // clear selection pools
    bgqueue->m_SelectionPools[TEAM_ALLIANCE].Init();
    bgqueue->m_SelectionPools[TEAM_HORDE].Init();

    // quick check if nothing we can do:
    if (!sBattlegroundMgr->isTesting())
        if ((aliFree > hordeFree && bgqueue->m_QueuedGroups[bracket_id][BG_QUEUE_CFBG].empty()))
            return false;

    // ally: at first fill as much as possible
    BattlegroundQueue::GroupsQueueType::const_iterator Ali_itr = bgqueue->m_QueuedGroups[bracket_id][BG_QUEUE_CFBG].begin();
    for (; Ali_itr != bgqueue->m_QueuedGroups[bracket_id][BG_QUEUE_CFBG].end() && bgqueue->m_SelectionPools[TEAM_ALLIANCE].AddGroup((*Ali_itr), aliFree); ++Ali_itr);

    // horde: at first fill as much as possible
    BattlegroundQueue::GroupsQueueType::const_iterator Horde_itr = bgqueue->m_QueuedGroups[bracket_id][BG_QUEUE_CFBG].begin();
    for (; Horde_itr != bgqueue->m_QueuedGroups[bracket_id][BG_QUEUE_CFBG].end() && bgqueue->m_SelectionPools[TEAM_HORDE].AddGroup((*Horde_itr), hordeFree); ++Horde_itr);

    // calculate free space after adding
    int32 aliDiff = aliFree - int32(bgqueue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount());
    int32 hordeDiff = hordeFree - int32(bgqueue->m_SelectionPools[TEAM_HORDE].GetPlayerCount());

    // if free space differs too much, ballance
    while (abs(aliDiff - hordeDiff) > 1 && (bgqueue->m_SelectionPools[TEAM_HORDE].GetPlayerCount() > 0 || bgqueue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount() > 0))
    {
        // if results in more alliance players than horde:
        if (aliDiff < hordeDiff)
        {
            // no more alliance in pool, invite whatever we can from horde
            if (!bgqueue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount())
                break;

            // kick alliance, returns true if kicked more than needed, so then try to fill up
            if (bgqueue->m_SelectionPools[TEAM_ALLIANCE].KickGroup(hordeDiff - aliDiff))
                for (; Ali_itr != bgqueue->m_QueuedGroups[bracket_id][BG_QUEUE_CFBG].end() && bgqueue->m_SelectionPools[TEAM_ALLIANCE].AddGroup((*Ali_itr), aliFree >= hordeDiff ? aliFree - hordeDiff : 0); ++Ali_itr);
        }        
        else // if results in more horde players than alliance:
        {
            // no more horde in pool, invite whatever we can from alliance
            if (!bgqueue->m_SelectionPools[TEAM_HORDE].GetPlayerCount())
                break;

            // kick horde, returns true if kicked more than needed, so then try to fill up
            if (bgqueue->m_SelectionPools[TEAM_HORDE].KickGroup(aliDiff - hordeDiff))
                for (; Horde_itr != bgqueue->m_QueuedGroups[bracket_id][BG_QUEUE_CFBG].end() && bgqueue->m_SelectionPools[TEAM_HORDE].AddGroup((*Horde_itr), hordeFree >= aliDiff ? hordeFree - aliDiff : 0); ++Horde_itr);
        }

        // recalculate free space after adding
        aliDiff = aliFree - int32(bgqueue->m_SelectionPools[TEAM_ALLIANCE].GetPlayerCount());
        hordeDiff = hordeFree - int32(bgqueue->m_SelectionPools[TEAM_HORDE].GetPlayerCount());
    }

    return true;
}

void CFBG::UpdateForget(Player* player)
{
    Battleground* bg = player->GetBattleground();
    if (bg)
    {
        if (ShouldForgetBGPlayers(player) && bg)
        {
            DoForgetPlayersInBG(player, bg);
            SetForgetBGPlayers(player, false);
        }
    }
    else if (ShouldForgetInListPlayers(player))
    {
        DoForgetPlayersInList(player);
        SetForgetInListPlayers(player, false);
    }
}

bool CFBG::SendMessageQueue(BattlegroundQueue* bgqueue, Battleground* bg, PvPDifficultyEntry const* bracketEntry, Player* leader)
{
    if (!IsEnableSystem())
        return false;

    BattlegroundBracketId bracketId = bracketEntry->GetBracketId();

    char const* bgName = bg->GetName();
    uint32 q_min_level = std::min(bracketEntry->minLevel, (uint32)80);
    uint32 q_max_level = std::min(bracketEntry->maxLevel, (uint32)80);
    uint32 MinPlayers = bg->GetMinPlayersPerTeam() * 2;
    uint32 qPlayers = bgqueue->GetPlayersCountInGroupsQueue(bracketId, BG_QUEUE_CFBG);

    ChatHandler(leader->GetSession()).PSendSysMessage("CFBG %s (Levels: %u - %u). Registered: %u/%u", bgName, q_min_level, q_max_level, qPlayers, MinPlayers);

    return true;
}

bool CFBG::IsAllCheckPassed(Player* player, bool JoinAsGroup, Battleground* /*bg*/)
{
    if (!IsEnableSystem())
        return true;

    if (JoinAsGroup)
    {
        Group* group = player->GetGroup();
        if (!group)
            return true;

        if (group->isRaidGroup() || group->GetMembersCount() > GetMaxPlayersCountInGroup())
            OutErrorAtJoin(player);
        
        return false;
    }
        
    return true;
}

void CFBG::OutErrorAtJoin(Player* player)
{
    WorldPacket data;
    sBattlegroundMgr->BuildGroupJoinedBattlegroundPacket(&data, ERR_BATTLEGROUND_JOIN_FAILED);
    player->GetSession()->SendPacket(&data);
}

// CFBG custom script
class CFBG_BG : public BGScript
{
public:
    CFBG_BG() : BGScript("CFBG_BG") {}

    void OnBattlegroundBeforeAddPlayer(Battleground* bg, Player* player) override
    {
        if (!bg || bg->isArena() || !player)
            return;

        if (!sCFBG->IsEnableSystem())
            return;

        TeamId teamid;
        Group* group = player->GetOriginalGroup();
        uint32 PlayerCountInBG = sCFBG->GetAllPlayersCountInBG(bg);

        PlayerCountInBG ? (teamid = sCFBG->GetLowerTeamIdInBG(bg)) : (teamid = player->GetTeamId(true));

        if (!group)
            sCFBG->ValidatePlayerForBG(bg, player, teamid);
        else
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member)
                    continue;

                if (bg->IsPlayerInBattleground(member->GetGUID()))
                    continue;

                sCFBG->ValidatePlayerForBG(bg, member, teamid);
            }
        }
    }

    void OnBattlegroundAddPlayer(Battleground* bg, Player* player) override
    {
        if (!sCFBG->IsEnableSystem())
            return;

        sCFBG->FitPlayerInTeam(player, true, bg);
    }

    void OnBattlegroundEndReward(Battleground* bg, Player* player, TeamId /*winnerTeamId*/) override
    {
        if (!bg || !player || bg->isArena())
            return;

        if (!sCFBG->IsEnableSystem())
            return;

        if (sCFBG->IsPlayerFake(player))
            sCFBG->ClearFakePlayer(player);
    }

    void OnBattlegroundRemovePlayerAtLeave(Battleground* bg, Player* player) override
    {
        if (!sCFBG->IsEnableSystem())
            return;

        sCFBG->FitPlayerInTeam(player, false, bg);

        if (sCFBG->IsPlayerFake(player))
            sCFBG->ClearFakePlayer(player);
    }
};

class CFBG_Player : public PlayerScript
{
public:
    CFBG_Player() : PlayerScript("CFBG_Player") { }
    
    void OnLogin(Player* player) override
    {
        if (!sCFBG->IsEnableSystem())
            return;

        if (player->GetTeamId(true) != player->GetBgTeamId())
            sCFBG->FitPlayerInTeam(player, player->GetBattleground() && !player->GetBattleground()->isArena() ? true : false, player->GetBattleground());
    }
};

class CFBG_World : public WorldScript
{
public:
    CFBG_World() : WorldScript("CFBG_World") { }

    void OnAfterConfigLoad(bool /*Reload*/) override
    {
        sCFBG->LoadConfig();
    }
};

void AddSC_CFBG()
{
    new CFBG_BG();
    new CFBG_Player();
    new CFBG_World();
}
