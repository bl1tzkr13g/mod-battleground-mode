#include "BattlegroundModePolicy.h"

#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <mutex>
#include <optional>
#include <unordered_map>

namespace BattlegroundMode
{
constexpr uint32 MenuId = 600019;
constexpr uint32 Sender = 600019;
constexpr uint32 ChooseNormal = 1;
constexpr uint32 ChooseBracket = 2;
constexpr uint32 Back = 3;
constexpr uint32 LevelAction = 100;
static_assert(MaxBracketChoices + 1 <= GOSSIP_MAX_MENU_ITEMS);

struct Destination
{
    uint32 map;
    float x, y, z, orientation;
};

struct Settings
{
    bool enabled = true;
    bool prompt = true;
    bool allowNormal = true;
    bool restore = true;
    bool teleport = true;
    bool blockTrade = true;
    bool blockSendMail = true;
    bool blockReceiveMail = true;
    bool blockAuction = true;
    bool blockGuildBank = true;
    std::vector<uint8> levels;
    Destination alliance{0, -8833.38f, 628.62f, 94.0f, 0.0f};
    Destination horde{1, 1629.36f, -4373.39f, 31.26f, 0.0f};
};

struct CharacterState
{
    Mode mode = Mode::Normal;
    uint8 level = 0;
    bool persisted = false;
    bool applyingLevel = false;
    uint32 checkTimer = 1000;
    bool prompt = false;
};

// Login/logout run on the world thread; player updates can run on map threads.
// Never retain references to the cache or hold its lock across a core callback.
class Manager
{
public:
    static Manager& Instance()
    {
        static Manager instance;
        return instance;
    }

    Settings Config()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _settings;
    }

    void LoadConfig()
    {
        Settings next;
        next.enabled = sConfigMgr->GetOption<bool>("BattlegroundMode.Enable", true);
        next.prompt = sConfigMgr->GetOption<bool>("BattlegroundMode.PromptOnLogin", true);
        next.allowNormal = sConfigMgr->GetOption<bool>("BattlegroundMode.AllowNormal", true);
        next.restore = sConfigMgr->GetOption<bool>("BattlegroundMode.RestoreHealthMana", true);
        next.teleport = sConfigMgr->GetOption<bool>("BattlegroundMode.TeleportOnSelection", true);
        next.blockTrade = sConfigMgr->GetOption<bool>("BattlegroundMode.BlockTrading", true);
        next.blockSendMail = sConfigMgr->GetOption<bool>("BattlegroundMode.BlockSendingMail", true);
        next.blockReceiveMail = sConfigMgr->GetOption<bool>("BattlegroundMode.BlockReceivingMail", true);
        next.blockAuction = sConfigMgr->GetOption<bool>("BattlegroundMode.BlockAuctionHouse", true);
        next.blockGuildBank = sConfigMgr->GetOption<bool>("BattlegroundMode.BlockGuildBank", true);
        next.levels = ParseLevels(sConfigMgr->GetOption<std::string>("BattlegroundMode.AllowedLevels", "19,29,39,49,59,69,79"),
            static_cast<uint8>(std::min<uint32>(80, sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))));
        if (next.levels.empty())
            LOG_ERROR("module", "Battleground Mode: invalid/empty AllowedLevels; bracket selection is unavailable.");

        auto destination = [](std::string const& faction, Destination defaults)
        {
            std::string prefix = "BattlegroundMode." + faction + ".";
            return Destination{sConfigMgr->GetOption<uint32>(prefix + "Map", defaults.map),
                sConfigMgr->GetOption<float>(prefix + "X", defaults.x),
                sConfigMgr->GetOption<float>(prefix + "Y", defaults.y),
                sConfigMgr->GetOption<float>(prefix + "Z", defaults.z),
                sConfigMgr->GetOption<float>(prefix + "O", defaults.orientation)};
        };
        next.alliance = destination("Alliance", next.alliance);
        next.horde = destination("Horde", next.horde);
        std::lock_guard<std::mutex> lock(_mutex);
        _settings = std::move(next);
    }

    std::optional<CharacterState> State(Player const* player)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto found = _characters.find(player->GetGUID().GetCounter());
        if (found == _characters.end())
            return std::nullopt;
        return found->second;
    }

    bool Restricted(Player const* player)
    {
        auto state = State(player);
        return !state || state->mode != Mode::Normal;
    }

    void Store(Player const* player, CharacterState const& state)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _characters[player->GetGUID().GetCounter()] = state;
    }

    void Forget(ObjectGuid guid)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _characters.erase(guid.GetCounter());
    }

    void DatabaseError(Player* player)
    {
        LOG_ERROR("module", "Battleground Mode: cannot load/verify character {}. Check the character database migration.",
            player->GetGUID().GetCounter());
        player->GetSession()->KickPlayer("Battleground Mode character state unavailable");
    }

    std::optional<CharacterState> Read(Player* player)
    {
        // LEFT JOIN guarantees a row even when this existing character has no
        // module record. An empty result therefore means an actual DB failure.
        QueryResult result = CharacterDatabase.Query(
            "SELECT b.mode, b.locked_level "
            "FROM characters c LEFT JOIN mod_battleground_mode b ON b.guid = c.guid WHERE c.guid = {}",
            player->GetGUID().GetCounter());
        if (!result)
            return std::nullopt;
        if ((*result)[0].IsNull())
            return CharacterState{};
        uint8 mode = (*result)[0].Get<uint8>();
        uint8 level = (*result)[1].Get<uint8>();
        if (!ValidRecord(static_cast<Mode>(mode), level))
            return std::nullopt;
        CharacterState state;
        state.mode = static_cast<Mode>(mode);
        state.level = level;
        state.persisted = true;
        return state;
    }

    std::optional<CharacterState> EligibleState(Player* player)
    {
        auto state = State(player);
        if (!state || state->mode != Mode::Pending || player->GetLevel() == 1)
            return state;

        // Retire old pending choices, including characters that leveled while
        // the module was disabled. Never rewrite an established bracket lock.
        CharacterDatabase.DirectExecute(
            "UPDATE mod_battleground_mode SET mode = 1, locked_level = 0 WHERE guid = {} AND mode = 0",
            player->GetGUID().GetCounter());
        auto saved = Read(player);
        if (!saved || !saved->persisted || saved->mode != Mode::Normal)
        {
            DatabaseError(player);
            return std::nullopt;
        }
        Store(player, *saved);
        return saved;
    }

    void Login(Player* player)
    {
        auto config = Config();
        if (!config.enabled)
            return;
        if (player->HasAtLoginFlag(AT_LOGIN_FIRST))
            CharacterDatabase.DirectExecute(
                "INSERT IGNORE INTO mod_battleground_mode (guid, mode, locked_level) VALUES ({}, {}, 0)",
                player->GetGUID().GetCounter(), player->GetLevel() == 1 ? 0 : 1);
        auto state = Read(player);
        if (!state || (player->HasAtLoginFlag(AT_LOGIN_FIRST) && !state->persisted))
        {
            DatabaseError(player);
            return;
        }
        Store(player, *state);
        state = EligibleState(player);
        if (!state)
            return;
        state->prompt = config.prompt && state->mode == Mode::Pending;
        Store(player, *state);
        if (state->mode == Mode::Battleground)
            Enforce(player, *state);
        if (state->mode == Mode::Pending)
            Tell(player, "Choose Normal or Battleground Mode. Type .bgmode if you close the selection menu. XP is paused until you choose.");
    }

    void Enforce(Player* player, CharacterState state)
    {
        if (state.mode != Mode::Battleground)
            return;
        player->SetPlayerFlag(PLAYER_FLAGS_NO_XP_GAIN);
        player->SetUInt32Value(PLAYER_XP, 0);
        if (player->GetLevel() != state.level)
        {
            state.applyingLevel = true;
            Store(player, state);
            // Use the normal level-change flow so the talent module sees it.
            player->GiveLevel(state.level);
            state.applyingLevel = false;
            Store(player, state);
            if (player->GetLevel() != state.level)
            {
                LOG_ERROR("module", "Battleground Mode: another hook prevented level {} for character {}.",
                    state.level, player->GetGUID().GetCounter());
                player->GetSession()->KickPlayer("Battleground Mode level could not be restored");
            }
        }
    }

    static void Tell(Player* player, char const* text)
    {
        ChatHandler(player->GetSession()).SendSysMessage(text);
    }

    static bool SafeToSelect(Player* player)
    {
        return player->IsAlive() && !player->IsInCombat() && !player->IsInFlight() &&
            !player->IsBeingTeleported() && !player->InBattleground() && !player->InBattlegroundQueue();
    }

    void ShowMenu(Player* player, bool brackets = false)
    {
        auto config = Config();
        auto state = config.enabled ? EligibleState(player) : State(player);
        if (!config.enabled)
        {
            Tell(player, "Battleground Mode is disabled.");
            return;
        }
        if (!state)
        {
            Tell(player, "Your character mode is unavailable. Please relog.");
            return;
        }
        if (state->mode != Mode::Pending)
        {
            if (state->mode == Mode::Normal)
                Tell(player, "This character uses Normal Mode. Its selection is permanent.");
            else
                ChatHandler(player->GetSession()).PSendSysMessage("Battleground Mode: permanently locked at level {}.", state->level);
            return;
        }
        if (!SafeToSelect(player))
        {
            Tell(player, "Choose your mode while alive, out of combat, and outside battlegrounds, queues, and travel.");
            return;
        }

        ClearGossipMenuFor(player);
        player->PlayerTalkClass->GetGossipMenu().SetMenuId(MenuId);
        if (!brackets)
        {
            if (config.allowNormal)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Normal Mode - level and play normally", Sender, ChooseNormal,
                    "Choose Normal Mode permanently for this character?", 0, false);
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Battleground Mode - choose a permanent level", Sender, ChooseBracket);
        }
        else
        {
            for (uint8 level : config.levels)
                if (CanChoose(state->mode, player->GetLevel(), level, config.levels))
                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "Level " + std::to_string(level), Sender, LevelAction + level,
                        "Permanently lock this character at level " + std::to_string(level) +
                        "? XP is disabled. Trading, mail, auctions, and guild banking may be restricted. This choice cannot be undone.", 0, false);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", Sender, Back);
        }
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, player->GetGUID());
    }

    void Select(Player* player, uint32 action)
    {
        auto config = Config();
        auto state = config.enabled ? EligibleState(player) : State(player);
        CloseGossipMenuFor(player);
        if (!config.enabled || !state || state->mode != Mode::Pending || !SafeToSelect(player))
            return;
        if (action == ChooseBracket || action == Back)
        {
            ShowMenu(player, action == ChooseBracket);
            return;
        }

        Mode chosen;
        uint8 level = 0;
        if (action == ChooseNormal && config.allowNormal)
            chosen = Mode::Normal;
        else if (action > LevelAction && action <= LevelAction + 80 &&
            CanChoose(state->mode, player->GetLevel(), static_cast<uint8>(action - LevelAction), config.levels))
        {
            chosen = Mode::Battleground;
            level = static_cast<uint8>(action - LevelAction);
        }
        else
            return;

        // Persist and verify BEFORE changing the character. Never overwrite a
        // completed decision; duplicate clicks and old menus cannot re-enroll.
        CharacterDatabase.DirectExecute(
            "UPDATE mod_battleground_mode SET mode = {}, locked_level = {} WHERE guid = {} AND mode = 0",
            static_cast<uint8>(chosen), level, player->GetGUID().GetCounter());
        auto persisted = Read(player);
        if (!persisted || !persisted->persisted || persisted->mode != chosen || persisted->level != level)
        {
            DatabaseError(player);
            return;
        }
        Store(player, *persisted);
        if (chosen == Mode::Normal)
        {
            Tell(player, "Normal Mode selected. You can now gain experience normally.");
            return;
        }

        Enforce(player, *persisted);
        if (player->GetLevel() != level)
            return;
        if (config.restore)
        {
            player->SetHealth(player->GetMaxHealth());
            player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));
        }
        if (config.teleport)
        {
            auto const& dest = player->GetTeamId(true) == TEAM_ALLIANCE ? config.alliance : config.horde;
            if (!player->TeleportTo(dest.map, dest.x, dest.y, dest.z, dest.orientation))
            {
                Tell(player, "Your level is locked, but the capital teleport failed. Contact a game master.");
                LOG_ERROR("module", "Battleground Mode: capital teleport failed for character {}.", player->GetGUID().GetCounter());
            }
        }
        player->SaveToDB(false, false);
        Tell(player, "Battleground Mode selected. Your level is permanently locked. Earn equipment upgrades within your bracket.");
    }

    void Update(Player* player, uint32 elapsed)
    {
        if (!Config().enabled)
            return;
        auto state = EligibleState(player);
        if (!state || state->mode == Mode::Normal)
            return;
        if (elapsed < state->checkTimer)
        {
            state->checkTimer -= elapsed;
            Store(player, *state);
            return;
        }
        bool prompt = state->prompt;
        state->prompt = false;
        state->checkTimer = 5000;
        Store(player, *state);
        Enforce(player, *state);
        if (prompt)
            ShowMenu(player);
    }

private:
    std::mutex _mutex;
    Settings _settings;
    std::unordered_map<ObjectGuid::LowType, CharacterState> _characters;
};

class ModeWorldScript : public WorldScript
{
public:
    ModeWorldScript() : WorldScript("battleground_mode_world") { }
    void OnAfterConfigLoad(bool /*reload*/) override { Manager::Instance().LoadConfig(); }
};

class ModePlayerScript : public PlayerScript
{
public:
    ModePlayerScript() : PlayerScript("battleground_mode_player") { }
    void OnPlayerLogin(Player* player) override { Manager::Instance().Login(player); }
    void OnPlayerLogout(Player* player) override { Manager::Instance().Forget(player->GetGUID()); }
    void OnPlayerUpdate(Player* player, uint32 elapsed) override { Manager::Instance().Update(player, elapsed); }
    void OnPlayerDeleteFromDB(CharacterDatabaseTransaction trans, uint32 guid) override
    {
        trans->Append("DELETE FROM mod_battleground_mode WHERE guid = {}", guid);
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*source*/) override
    {
        auto& manager = Manager::Instance();
        if (manager.Config().enabled && manager.Restricted(player))
            amount = 0;
    }

    bool OnPlayerCanGiveLevel(Player* player, uint8 newLevel) override
    {
        auto& manager = Manager::Instance();
        if (!manager.Config().enabled)
            return true;
        auto state = manager.State(player);
        // Player creation/loading precedes OnPlayerLogin and must retain its
        // native initialization (including the Death Knight starting level).
        if (!state)
            return player->isBeingLoaded();
        return state->mode == Mode::Normal ||
            (state->mode == Mode::Battleground && state->applyingLevel && newLevel == state->level);
    }

    void OnPlayerGossipSelect(Player* player, uint32 menu, uint32 sender, uint32 action) override
    {
        if (menu == MenuId && sender == Sender)
            Manager::Instance().Select(player, action);
    }

    bool OnPlayerCanInitTrade(Player* player, Player* target) override
    {
        auto& manager = Manager::Instance();
        auto config = manager.Config();
        if (config.enabled && config.blockTrade && (manager.Restricted(player) || manager.Restricted(target)))
        {
            Manager::Tell(player, "Trading is restricted for Battleground Mode characters.");
            return false;
        }
        return true;
    }

    bool OnPlayerCanSendMail(Player* player, ObjectGuid receiver, ObjectGuid /*mailbox*/, std::string& /*subject*/,
        std::string& /*body*/, uint32 /*money*/, uint32 /*cod*/, Item* /*item*/) override
    {
        auto& manager = Manager::Instance();
        auto config = manager.Config();
        if (!config.enabled)
            return true;
        if (config.blockSendMail && manager.Restricted(player))
            return false;
        if (!config.blockReceiveMail)
            return true;
        // The receiver can be offline. Do not rely on the online cache.
        QueryResult result = CharacterDatabase.Query(
            "SELECT b.mode FROM characters c LEFT JOIN mod_battleground_mode b ON b.guid = c.guid WHERE c.guid = {}",
            receiver.GetCounter());
        if (!result || (!(*result)[0].IsNull() && (*result)[0].Get<uint8>() != static_cast<uint8>(Mode::Normal)))
        {
            Manager::Tell(player, "Mail to this character is restricted or its mode could not be verified.");
            return false;
        }
        return true;
    }
};

class ModeServerScript : public ServerScript
{
public:
    ModeServerScript() : ServerScript("battleground_mode_transfers") { }

    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
        Player* player = session ? session->GetPlayer() : nullptr;
        if (!player)
            return true;
        auto& manager = Manager::Instance();
        auto config = manager.Config();
        if (!config.enabled || !manager.Restricted(player))
            return true;

        bool blocked = false;
        switch (packet.GetOpcode())
        {
            case CMSG_INITIATE_TRADE:
            case CMSG_BEGIN_TRADE:
            case CMSG_ACCEPT_TRADE:
            case CMSG_SET_TRADE_ITEM:
            case CMSG_SET_TRADE_GOLD:
                blocked = config.blockTrade;
                break;
            case CMSG_SEND_MAIL:
                blocked = config.blockSendMail;
                break;
            case CMSG_MAIL_TAKE_MONEY:
            case CMSG_MAIL_TAKE_ITEM:
            case CMSG_MAIL_RETURN_TO_SENDER:
                blocked = config.blockReceiveMail;
                break;
            case CMSG_AUCTION_SELL_ITEM:
            case CMSG_AUCTION_REMOVE_ITEM:
            case CMSG_AUCTION_PLACE_BID:
                blocked = config.blockAuction;
                break;
            case CMSG_GUILD_BANK_SWAP_ITEMS:
            case CMSG_GUILD_BANK_BUY_TAB:
            case CMSG_GUILD_BANK_DEPOSIT_MONEY:
            case CMSG_GUILD_BANK_WITHDRAW_MONEY:
                blocked = config.blockGuildBank;
                break;
            case CMSG_REPAIR_ITEM:
                // Guild-funded repairs withdraw bank gold through the NPC
                // handler, rather than CMSG_GUILD_BANK_WITHDRAW_MONEY.
                if (config.blockGuildBank && packet.size() >= 17)
                {
                    WorldPacket copy(packet);
                    ObjectGuid npc, item;
                    uint8 guildBank;
                    copy >> npc >> item >> guildBank;
                    blocked = guildBank != 0;
                }
                break;
            default:
                break;
        }
        if (blocked)
            Manager::Tell(player, "That transaction is restricted for Battleground Mode characters.");
        return !blocked;
    }
};

class ModeCommandScript : public CommandScript
{
public:
    ModeCommandScript() : CommandScript("battleground_mode_commands") { }
    Acore::ChatCommands::ChatCommandTable GetCommands() const override
    {
        using namespace Acore::ChatCommands;
        static ChatCommandTable commands = {{"bgmode", HandleMode, SEC_PLAYER, Console::No}};
        return commands;
    }
    static bool HandleMode(ChatHandler* handler)
    {
        if (Player* player = handler->GetPlayer())
            Manager::Instance().ShowMenu(player);
        return true;
    }
};
}

void AddBattlegroundModeScripts()
{
    new BattlegroundMode::ModeWorldScript();
    new BattlegroundMode::ModePlayerScript();
    new BattlegroundMode::ModeServerScript();
    new BattlegroundMode::ModeCommandScript();
}
