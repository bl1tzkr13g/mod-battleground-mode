#include "BattlegroundModePolicy.h"
#include "BattlegroundModeGear.h"

#include "Chat.h"
#include "CinematicMgr.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Trainer.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <algorithm>
#include <mutex>
#include <memory>
#include <optional>
#include <set>
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
    bool trainClassSpells = true;
    bool blockTrade = true;
    bool blockSendMail = true;
    bool blockReceiveMail = true;
    bool blockAuction = true;
    bool blockGuildBank = true;
    bool starterEnabled = true;
    bool starterAutoEquip = true;
    bool starterTrainArmor = true;
    bool starterBind = true;
    std::shared_ptr<GearCatalog const> gear = std::make_shared<GearCatalog>();
    std::vector<uint8> levels;
    uint32 dalaranMinLevel = 79;
    Destination shattrath{530, -1838.16f, 5301.79f, -12.428f, 5.0f};
    Destination dalaran{571, 5807.75f, 588.347f, 660.939f, 1.64f};
};

struct CharacterState
{
    Mode mode = Mode::Normal;
    uint8 level = 0;
    bool persisted = false;
    bool applyingLevel = false;
    StarterState starter = StarterState::Pending;
    bool starterAttempt = false;
    bool trainSpellsAttempt = false;
    uint32 checkTimer = 1000;
    LoginPrompt prompt;
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

    void LoadConfig(bool reload)
    {
        Settings next;
        next.enabled = sConfigMgr->GetOption<bool>("BattlegroundMode.Enable", true);
        next.prompt = sConfigMgr->GetOption<bool>("BattlegroundMode.PromptOnLogin", true);
        next.allowNormal = sConfigMgr->GetOption<bool>("BattlegroundMode.AllowNormal", true);
        next.restore = sConfigMgr->GetOption<bool>("BattlegroundMode.RestoreHealthMana", true);
        next.teleport = sConfigMgr->GetOption<bool>("BattlegroundMode.TeleportOnSelection", true);
        next.trainClassSpells = sConfigMgr->GetOption<bool>("BattlegroundMode.AutoLearnClassTrainerSpells", true);
        next.blockTrade = sConfigMgr->GetOption<bool>("BattlegroundMode.BlockTrading", true);
        next.blockSendMail = sConfigMgr->GetOption<bool>("BattlegroundMode.BlockSendingMail", true);
        next.blockReceiveMail = sConfigMgr->GetOption<bool>("BattlegroundMode.BlockReceivingMail", true);
        next.blockAuction = sConfigMgr->GetOption<bool>("BattlegroundMode.BlockAuctionHouse", true);
        next.blockGuildBank = sConfigMgr->GetOption<bool>("BattlegroundMode.BlockGuildBank", true);
        next.starterEnabled = sConfigMgr->GetOption<bool>("BattlegroundMode.StarterGear.Enable", true);
        next.starterAutoEquip = sConfigMgr->GetOption<bool>("BattlegroundMode.StarterGear.AutoEquip", true);
        next.starterTrainArmor = sConfigMgr->GetOption<bool>("BattlegroundMode.StarterGear.TrainArmorProficiency", true);
        next.starterBind = sConfigMgr->GetOption<bool>("BattlegroundMode.StarterGear.BindToCharacter", true);
        next.levels = ParseLevels(sConfigMgr->GetOption<std::string>("BattlegroundMode.AllowedLevels", "19,29,39,49,59,69,79"),
            static_cast<uint8>(std::min<uint32>(80, sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))));
        if (next.levels.empty())
            LOG_ERROR("module", "Battleground Mode: invalid/empty AllowedLevels; bracket selection is unavailable.");

        auto destination = [](std::string const& city, Destination defaults)
        {
            std::string prefix = "BattlegroundMode.Teleport." + city + ".";
            return Destination{sConfigMgr->GetOption<uint32>(prefix + "Map", defaults.map),
                sConfigMgr->GetOption<float>(prefix + "X", defaults.x),
                sConfigMgr->GetOption<float>(prefix + "Y", defaults.y),
                sConfigMgr->GetOption<float>(prefix + "Z", defaults.z),
                sConfigMgr->GetOption<float>(prefix + "O", defaults.orientation)};
        };
        next.dalaranMinLevel = std::clamp<uint32>(
            sConfigMgr->GetOption<uint32>("BattlegroundMode.Teleport.Dalaran.MinLevel", 79), 1, 80);
        next.shattrath = destination("Shattrath", next.shattrath);
        next.dalaran = destination("Dalaran", next.dalaran);
        auto gear = std::make_shared<GearCatalog>();
        for (auto const& key : sConfigMgr->GetKeysByString("BattlegroundMode.Gear.Level"))
            if (!gear->Add(key, sConfigMgr->GetOption<std::string>(key, "", false)))
                LOG_ERROR("module", "Battleground Mode: invalid gear list {}. Expected itemId:honor pairs; "
                    "tier 1 must be free and later tiers must have positive prices.", key);
        next.gear = std::move(gear);
        // Item templates are loaded later during initial server startup. On
        // reload, validate before publishing the replacement immutable catalog.
        if (reload)
            next.gear = ValidateGearLists(next.gear);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _settings = std::move(next);
        }
    }

    std::shared_ptr<GearCatalog const> ValidateGearLists(std::shared_ptr<GearCatalog const> const& snapshot)
    {
        GearCatalog::Lists valid;
        for (auto const& [key, items] : snapshot->All())
        {
            bool usable = true;
            for (auto const& item : items)
            {
                ItemTemplate const* prototype = sObjectMgr->GetItemTemplate(item.entry);
                if (!prototype || prototype->RequiredLevel > key.level ||
                    !(prototype->AllowableClass & (1u << (key.playerClass - 1))) ||
                    (prototype->Class == ITEM_CLASS_ARMOR &&
                        !GearArmorAllowed(key.playerClass, key.level, prototype->SubClass)) ||
                    item.honor > sWorld->getIntConfig(CONFIG_MAX_HONOR_POINTS))
                {
                    LOG_ERROR("module", "Battleground Mode: rejecting level {} class {} tier {}: "
                        "invalid/unusable item {} or honor price {} exceeds the honor cap.",
                        key.level, key.playerClass, key.tier, item.entry, item.honor);
                    usable = false;
                }
            }
            if (usable)
                valid.emplace(key, items);
        }
        return std::make_shared<GearCatalog>(std::move(valid));
    }

    void ValidateGear()
    {
        auto validated = ValidateGearLists(Config().gear);
        std::lock_guard<std::mutex> lock(_mutex);
        _settings.gear = std::move(validated);
    }

    void StarterDeliveryError(Player* player)
    {
        LOG_ERROR("module", "Battleground Mode: interrupted starter delivery for character {}. "
            "Reconcile inventory before resetting starter_state; do not blindly repeat the grant.",
            player->GetGUID().GetCounter());
        Tell(player, "Starter delivery was interrupted. Contact a game master; it will not be repeated automatically.");
    }

    void TrainClassSpells(Player* player)
    {
        auto state = State(player);
        if (!Config().enabled || !Config().trainClassSpells || !state || state->mode != Mode::Battleground ||
            player->GetLevel() != state->level)
            return;

        // Use the same class-trainer eligibility and learning paths as the
        // core's .learn all my trainer command, without charging training fees.
        // Repeat for prerequisite chains, irrespective of trainer row order.
        // Attempt each spell only once so a script refusing a learn cannot loop.
        auto const& trainers = sObjectMgr->GetClassTrainers(player->getClass());
        std::set<uint32> attempted;
        bool progressed;
        do
        {
            progressed = false;
            for (Trainer::Trainer const* trainer : trainers)
            {
                if (!trainer->IsTrainerValidForPlayer(player))
                    continue;
                for (Trainer::Spell const& spell : trainer->GetSpells())
                {
                    if (spell.ReqLevel > state->level || attempted.count(spell.SpellId) ||
                        !trainer->CanTeachSpell(player, &spell))
                        continue;
                    attempted.insert(spell.SpellId);
                    if (spell.IsCastable())
                        player->CastSpell(player, spell.SpellId, true);
                    else
                        player->learnSpell(spell.SpellId, false);
                    progressed = true;
                }
            }
        } while (progressed);
    }

    // Use the core's localized item-query response rather than duplicating
    // its packet format. Prime the client before item notifications/teleport.
    static void SendItemInformation(Player* player, uint32 entry)
    {
        WorldPacket query(CMSG_ITEM_QUERY_SINGLE, sizeof(uint32));
        query << entry;
        player->GetSession()->HandleItemQuerySingleOpcode(query);
    }

    static void RefreshEquippedItemInformation(Player* player)
    {
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                SendItemInformation(player, item->GetEntry());
    }

    void GiveStarterGear(Player* player)
    {
        auto config = Config();
        auto state = State(player);
        if (!config.enabled || !state || state->mode != Mode::Battleground)
        {
            Tell(player, "Starter gear is available to Battleground Mode characters only.");
            return;
        }
        if (!CanDeliverStarter(config.starterEnabled, true, state->starter))
        {
            RefreshEquippedItemInformation(player);
            Tell(player, state->starter == StarterState::Claimed ?
                "An earlier starter delivery needs game master review." :
                "Starter gear is disabled or has already been delivered to this character.");
            return;
        }
        if (!player->IsInWorld() || player->isBeingLoaded() || !SafeToSelect(player) || player->GetTradeData() ||
            player->GetLevel() != state->level)
        {
            Tell(player, "Use .bgstarter when alive, out of combat, and outside travel, trades, and battlegrounds.");
            return;
        }
        auto const& kit = config.gear->Find(state->level, player->getClass(), 1);
        if (kit.empty())
        {
            Tell(player, "No valid Tier 1 gear is configured for your bracket and class. Contact a game master.");
            return;
        }

        if (config.starterAutoEquip && config.starterTrainArmor && player->GetLevel() >= 40)
        {
            if (player->getClass() == CLASS_WARRIOR || player->getClass() == CLASS_PALADIN)
                player->learnSpell(750); // Class-appropriate plate proficiency.
            else if (player->getClass() == CLASS_HUNTER || player->getClass() == CLASS_SHAMAN)
                player->learnSpell(8737); // Class-appropriate mail proficiency.
        }

        // Preallocate and preflight the WHOLE kit before consuming its claim.
        // Creating an affixed item queues it; discarded probes must be removed.
        auto discard = [player](Item* item)
        {
            if (item)
            {
                item->RemoveFromUpdateQueueOf(player);
                delete item;
            }
        };
        std::vector<std::unique_ptr<Item, decltype(discard)>> prepared;
        std::vector<Item*> incoming;
        std::vector<uint16> equipment;
        std::set<uint16> usedSlots;
        for (auto const& entry : kit)
        {
            ItemTemplate const* prototype = sObjectMgr->GetItemTemplate(entry.entry);
            if (!prototype)
            {
                Tell(player, "A configured starter item is unavailable. Contact a game master.");
                return;
            }
            prepared.emplace_back(Item::CreateItem(entry.entry, 1, player), discard);
            Item* item = prepared.back().get();
            if (!item)
            {
                Tell(player, "Starter items could not be prepared. Try .bgstarter again later.");
                return;
            }
            if ((prototype->RandomProperty || prototype->RandomSuffix) && item->GetItemRandomPropertyId() == 0)
            {
                Tell(player, "A starter item's random stats could not be generated. Nothing was delivered; contact a game master.");
                return;
            }
            if (config.starterBind)
                item->SetBinding(true);
            incoming.push_back(item);
            if (config.starterAutoEquip)
            {
                uint16 destination = 0;
                InventoryResult result = player->CanEquipItem(NULL_SLOT, destination, item, true);
                if (result != EQUIP_ERR_OK || player->CanUnequipItem(destination, true) != EQUIP_ERR_OK)
                {
                    if (result != EQUIP_ERR_OK)
                        player->SendEquipError(result, item);
                    Tell(player, "The whole starter set must be equippable. Nothing was delivered; fix the restriction and use .bgstarter.");
                    return;
                }
                if (!usedSlots.insert(destination).second)
                {
                    Tell(player, "Starter gear contains conflicting equipment slots. Nothing was delivered; contact a game master.");
                    return;
                }
                equipment.push_back(destination);
            }
        }
        uint32 limitedBy = 0;
        InventoryResult capacity = player->CanStoreItems(incoming.data(), static_cast<int>(incoming.size()), &limitedBy);
        if (capacity != EQUIP_ERR_OK)
        {
            player->SendEquipError(capacity, nullptr);
            Tell(player, "Make bag room for the entire starter set, then use .bgstarter. Existing equipment will be kept in your bags.");
            return;
        }

        // A durable claim precedes item creation in inventory. A crash/error
        // after this point requires review instead of duplicating free items.
        state->starter = StarterState::Claimed;
        state->starterAttempt = false;
        Store(player, *state);
        CharacterDatabase.DirectExecute(
            "UPDATE mod_battleground_mode SET starter_state = 1 WHERE guid = {} AND mode = 2 AND starter_state = 0",
            player->GetGUID().GetCounter());
        auto claimed = Read(player);
        if (!claimed || claimed->mode != Mode::Battleground || claimed->level != state->level ||
            claimed->starter != StarterState::Claimed)
        {
            DatabaseError(player);
            return;
        }

        std::vector<Item*> delivered;
        for (auto& item : prepared)
        {
            ItemPosCountVec positions;
            if (player->CanStoreItem(NULL_BAG, NULL_SLOT, positions, item.get()) != EQUIP_ERR_OK)
            {
                StarterDeliveryError(player);
                return;
            }
            uint32 entry = item->GetEntry();
            Item* stored = player->StoreItem(positions, item.release(), true);
            if (!stored)
            {
                StarterDeliveryError(player);
                return;
            }
            player->ItemAddedQuestCheck(entry, 1);
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM, entry, 1);
            sScriptMgr->OnPlayerStoreNewItem(player, stored, 1);
            SendItemInformation(player, entry);
            player->SendNewItem(stored, 1, true, false);
            delivered.push_back(stored);
        }

        bool fullyEquipped = true;
        if (config.starterAutoEquip)
            for (std::size_t i = 0; i < delivered.size(); ++i)
            {
                // Native swapping updates stats/appearance and puts the old
                // equipped item into the new item's vacated bag position.
                player->SwapItem(delivered[i]->GetPos(), equipment[i]);
                fullyEquipped = fullyEquipped && player->GetItemByPos(equipment[i]) == delivered[i];
            }

        // Queue item instances, inventory positions, and completion together.
        // Build the save BEFORE any city teleport (which delays saves).
        if (!player->IsInWorld() || player->IsBeingTeleportedFar())
        {
            StarterDeliveryError(player);
            return;
        }
        CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
        player->SaveToDB(transaction, false, false);
        transaction->Append(
            "UPDATE mod_battleground_mode SET starter_state = 2 WHERE guid = {} AND mode = 2 AND starter_state = 1",
            player->GetGUID().GetCounter());
        // Player::SaveToDB appends statements prepared only on async workers.
        // A direct commit uses a synchronous connection and asserts in the core.
        // Resolve the player through the session when the worker finishes; the
        // original Player pointer may no longer be valid after logout.
        WorldSession* session = player->GetSession();
        ObjectGuid guid = player->GetGUID();
        session->AddTransactionCallback(CharacterDatabase.AsyncCommitTransaction(transaction)).AfterComplete(
            [this, session, guid, restore = config.restore, autoEquip = config.starterAutoEquip, fullyEquipped](bool success)
        {
            Player* current = session->GetPlayer();
            if (!current || current->GetGUID() != guid)
                return;
            auto saved = success ? Read(current) : std::nullopt;
            if (!saved || saved->starter != StarterState::Delivered)
            {
                StarterDeliveryError(current);
                return;
            }
            Store(current, *saved);
            if (restore)
            {
                current->SetHealth(current->GetMaxHealth());
                current->SetPower(POWER_MANA, current->GetMaxPower(POWER_MANA));
            }
            Tell(current, !autoEquip ? "Your one-time Tier 1 starter set has been placed in your bags." :
                fullyEquipped ? "Your Tier 1 starter set is equipped. Replaced equipment is in your bags." :
                "Your starter set was delivered, but some pieces could not be equipped and remain in your bags.");
        });
    }

    void ShowGear(Player* player, uint32 tier)
    {
        auto config = Config();
        auto state = State(player);
        if (!config.enabled || !state || state->mode != Mode::Battleground)
        {
            Tell(player, "Gear lists are available to Battleground Mode characters only.");
            return;
        }
        if (tier == 0 || tier > 65535)
        {
            Tell(player, "Tier must be between 1 and 65535.");
            return;
        }
        auto const& items = config.gear->Find(state->level, player->getClass(), static_cast<uint16>(tier));
        if (items.empty())
        {
            Tell(player, "No gear is configured for your bracket, class, and requested tier.");
            return;
        }
        ChatHandler handler(player->GetSession());
        handler.PSendSysMessage("Level {} / tier {} gear catalog (preview only):", state->level, tier);
        for (auto const& item : items)
            handler.PSendSysMessage("Item {}: {} honor", item.entry, item.honor);
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
            "SELECT b.mode, b.locked_level, b.starter_state "
            "FROM characters c LEFT JOIN mod_battleground_mode b ON b.guid = c.guid WHERE c.guid = {}",
            player->GetGUID().GetCounter());
        if (!result)
            return std::nullopt;
        if ((*result)[0].IsNull())
            return CharacterState{};
        uint8 mode = (*result)[0].Get<uint8>();
        uint8 level = (*result)[1].Get<uint8>();
        uint8 starter = (*result)[2].Get<uint8>();
        if (!ValidRecord(static_cast<Mode>(mode), level) || starter > static_cast<uint8>(StarterState::Delivered))
            return std::nullopt;
        CharacterState state;
        state.mode = static_cast<Mode>(mode);
        state.level = level;
        state.starter = static_cast<StarterState>(starter);
        state.persisted = true;
        return state;
    }

    std::optional<CharacterState> EligibleState(Player* player)
    {
        auto state = State(player);
        if (!state || !ShouldBecomeNormal(state->mode, player->GetLevel()))
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
        CloseGossipMenuFor(player);
        Tell(player, "You have progressed beyond level 1. This character now permanently uses Normal Mode.");
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
        state->prompt.pending = config.prompt && state->mode == Mode::Pending;
        state->starterAttempt = CanDeliverStarter(config.starterEnabled, state->mode == Mode::Battleground, state->starter);
        state->trainSpellsAttempt = config.trainClassSpells && state->mode == Mode::Battleground;
        Store(player, *state);
        if (state->mode == Mode::Battleground)
        {
            Enforce(player, *state);
            RefreshEquippedItemInformation(player);
        }
        if (state->mode == Mode::Pending)
            Tell(player, "Choose Normal or Battleground Mode. Type .bgmode to reopen the menu at level 1. XP works normally; reaching level 2 permanently selects Normal Mode.");
        if (state->mode == Mode::Battleground && state->starter == StarterState::Claimed)
            Tell(player, "An earlier starter delivery was interrupted. Contact a game master; it will not be repeated automatically.");
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
            !player->IsBeingTeleported() && !player->InBattleground() && !player->InBattlegroundQueue() &&
            player->GetCinematicMgr().GetActiveCinematicCamera() == 0;
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
        TrainClassSpells(player);
        GiveStarterGear(player);
        if (config.restore)
        {
            player->SetHealth(player->GetMaxHealth());
            player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));
        }
        if (config.teleport)
        {
            auto const& dest = level >= config.dalaranMinLevel ? config.dalaran : config.shattrath;
            if (!player->TeleportTo(dest.map, dest.x, dest.y, dest.z, dest.orientation))
            {
                Tell(player, "Your level is locked, but the city teleport failed. Contact a game master.");
                LOG_ERROR("module", "Battleground Mode: city teleport failed for character {}.", player->GetGUID().GetCounter());
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
        bool prompt = state->prompt.Advance(SafeToSelect(player), elapsed);
        bool ready = player->IsInWorld() && !player->isBeingLoaded() &&
            !player->GetTradeData() && SafeToSelect(player);
        bool starter = state->starterAttempt && ready;
        bool train = state->trainSpellsAttempt && ready;
        if (train)
            state->trainSpellsAttempt = false;
        if (starter)
            state->starterAttempt = false;
        bool checkLevel = elapsed >= state->checkTimer;
        if (!checkLevel)
            state->checkTimer -= elapsed;
        else
            state->checkTimer = 5000;
        Store(player, *state);
        if (checkLevel)
            Enforce(player, *state);
        if (train)
            TrainClassSpells(player);
        if (starter)
            GiveStarterGear(player);
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
    void OnAfterConfigLoad(bool reload) override { Manager::Instance().LoadConfig(reload); }
    void OnStartup() override { Manager::Instance().ValidateGear(); }
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
        auto state = manager.State(player);
        if (manager.Config().enabled && (!state || !CanGainExperience(state->mode)))
            amount = 0;
    }

    void OnPlayerLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        auto& manager = Manager::Instance();
        if (manager.Config().enabled)
            manager.EligibleState(player);
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
        return CanChangeLevel(state->mode, newLevel, state->level, state->applyingLevel);
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
        static ChatCommandTable commands =
        {
            {"bgmode", HandleMode, SEC_PLAYER, Console::No},
            {"bggear", HandleGear, SEC_PLAYER, Console::No},
            {"bgstarter", HandleStarter, SEC_PLAYER, Console::No}
        };
        return commands;
    }
    static bool HandleMode(ChatHandler* handler)
    {
        if (Player* player = handler->GetPlayer())
            Manager::Instance().ShowMenu(player);
        return true;
    }
    static bool HandleGear(ChatHandler* handler, uint32 tier)
    {
        if (Player* player = handler->GetPlayer())
            Manager::Instance().ShowGear(player, tier);
        return true;
    }
    static bool HandleStarter(ChatHandler* handler)
    {
        if (Player* player = handler->GetPlayer())
            Manager::Instance().GiveStarterGear(player);
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
