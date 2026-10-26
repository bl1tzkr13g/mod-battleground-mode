#ifndef BATTLEGROUND_MODE_GEAR_H
#define BATTLEGROUND_MODE_GEAR_H

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <map>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace BattlegroundMode
{
enum class StarterState : std::uint8_t { Pending = 0, Claimed = 1, Delivered = 2 };

inline bool CanDeliverStarter(bool enabled, bool battlegroundMode, StarterState state)
{
    return enabled && battlegroundMode && state == StarterState::Pending;
}

inline bool ValidStarterBag(std::uint32_t itemClass, std::uint32_t subclass,
    std::uint32_t inventoryType, std::uint32_t slots, std::uint32_t family)
{
    // WotLK's client/core container limit is 36. Only generic bags can hold
    // every piece of a mixed armor/weapon/jewelry starter kit.
    return itemClass == 1 && subclass == 0 && inventoryType == 18 && slots > 0 && slots <= 36 && family == 0;
}

// Auto-equip stages one piece at a time. Reserve space for all displaced gear
// plus alternatives and one working item; bag-only delivery holds the whole grant.
template <typename Item>
std::vector<Item*> StarterStorageReservation(std::vector<Item*> const& incoming,
    std::vector<Item*> const& replaced, bool autoEquip, std::vector<Item*> const& bagOnly = {})
{
    if (!autoEquip)
        return incoming;
    auto storage = replaced;
    storage.insert(storage.end(), bagOnly.begin(), bagOnly.end());
    if (incoming.size() > bagOnly.size())
        storage.push_back(incoming.front());
    return storage;
}

// IDs are persisted: never renumber or reuse an existing role.
enum class StarterRole : std::uint8_t
{
    None = 0, Damage = 1, Holy = 2, Retribution = 3, Feral = 4,
    Balance = 5, Restoration = 6, Enhancement = 7, Elemental = 8, Protection = 9
};

inline StarterRole DefaultStarterRole(std::uint8_t playerClass)
{
    switch (playerClass)
    {
        case 2: return StarterRole::Retribution;
        case 7: return StarterRole::Enhancement;
        case 11: return StarterRole::Feral;
        default: return StarterRole::Damage;
    }
}

inline std::string_view StarterRoleName(StarterRole role)
{
    switch (role)
    {
        case StarterRole::Damage: return "Damage";
        case StarterRole::Holy: return "Holy";
        case StarterRole::Retribution: return "Retribution";
        case StarterRole::Feral: return "Feral";
        case StarterRole::Balance: return "Balance";
        case StarterRole::Restoration: return "Restoration";
        case StarterRole::Enhancement: return "Enhancement";
        case StarterRole::Elemental: return "Elemental";
        case StarterRole::Protection: return "Protection";
        default: return "Unselected";
    }
}

inline bool StarterRoleAllowed(std::uint8_t playerClass, StarterRole role)
{
    if (role == DefaultStarterRole(playerClass))
        return playerClass >= 1 && playerClass <= 11 && playerClass != 10;
    return ((playerClass == 1 || playerClass == 2) && role == StarterRole::Protection) ||
        (playerClass == 2 && role == StarterRole::Holy) ||
        (playerClass == 7 && (role == StarterRole::Elemental || role == StarterRole::Restoration)) ||
        (playerClass == 11 && (role == StarterRole::Balance || role == StarterRole::Restoration));
}

struct GearKey
{
    std::uint8_t level;
    std::uint8_t playerClass;
    std::uint16_t tier;
    StarterRole role = StarterRole::None;
    bool bag = false;

    bool operator<(GearKey const& other) const
    {
        return std::tie(level, playerClass, tier, role, bag) <
            std::tie(other.level, other.playerClass, other.tier, other.role, other.bag);
    }
};

struct GearItem
{
    std::uint32_t entry;
    std::uint32_t honor;
};

inline std::optional<std::uint32_t> GearNumber(std::string_view value)
{
    if (value.empty())
        return std::nullopt;
    std::uint32_t number = 0;
    auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), number);
    if (error != std::errc{} || end != value.data() + value.size() || number == 0)
        return std::nullopt;
    return number;
}

inline std::optional<GearKey> ParseGearKey(std::string_view key)
{
    constexpr std::string_view bagPrefix = "BattlegroundMode.Gear.Bag.";
    bool bag = key.substr(0, bagPrefix.size()) == bagPrefix;
    std::string_view prefix = bag ? "BattlegroundMode.Gear.Bag.Level" : "BattlegroundMode.Gear.Level";
    if (key.substr(0, prefix.size()) != prefix)
        return std::nullopt;
    key.remove_prefix(prefix.size());
    auto tierPosition = key.find(".Tier");
    if (tierPosition == std::string_view::npos)
        return std::nullopt;
    auto level = GearNumber(key.substr(0, tierPosition));
    key.remove_prefix(tierPosition + 5);
    auto classPosition = key.find('.');
    if (classPosition == std::string_view::npos)
        return std::nullopt;
    auto tier = GearNumber(key.substr(0, classPosition));
    key.remove_prefix(classPosition + 1);
    constexpr std::string_view classes[] = {"", "Warrior", "Paladin", "Hunter", "Rogue", "Priest",
        "DeathKnight", "Shaman", "Mage", "Warlock", "", "Druid"};
    if (!level || *level > 80 || !tier || *tier > 65535)
        return std::nullopt;
    if (*tier != 1)
        return std::nullopt; // Progression gear belongs to stock PvP vendors.
    auto rolePosition = key.find('.');
    auto className = key.substr(0, rolePosition);
    auto roleName = rolePosition == std::string_view::npos ? std::string_view{} : key.substr(rolePosition + 1);
    for (std::uint8_t playerClass = 1; playerClass < 12; ++playerClass)
        if (!classes[playerClass].empty() && className == classes[playerClass])
        {
            if (rolePosition == std::string_view::npos)
                return GearKey{static_cast<std::uint8_t>(*level), playerClass, 1, DefaultStarterRole(playerClass), bag};
            for (unsigned id = 1; id <= 9; ++id)
            {
                auto role = static_cast<StarterRole>(id);
                if (roleName == StarterRoleName(role) && StarterRoleAllowed(playerClass, role))
                    return GearKey{static_cast<std::uint8_t>(*level), playerClass, 1, role, bag};
            }
        }
    return std::nullopt;
}

// Empty is an intentional empty catalog. A malformed list is distinct from it.
inline std::optional<std::vector<GearItem>> ParseGearItems(std::string_view text)
{
    std::vector<GearItem> items;
    if (text.find_first_not_of(" \t") == std::string_view::npos)
        return items;
    while (!text.empty())
    {
        auto comma = text.find(',');
        auto token = text.substr(0, comma);
        auto start = token.find_first_not_of(" \t");
        if (start == std::string_view::npos)
            return std::nullopt;
        token = token.substr(start, token.find_last_not_of(" \t") - start + 1);
        auto colon = token.find(':');
        if (colon == std::string_view::npos)
            return std::nullopt;
        auto item = GearNumber(token.substr(0, colon));
        auto priceText = token.substr(colon + 1);
        // Zero is explicit and only appropriate for starter items.
        auto price = priceText == "0" ? std::optional<std::uint32_t>(0) : GearNumber(priceText);
        if (!item || !price || std::any_of(items.begin(), items.end(),
                [item](GearItem const& existing) { return existing.entry == *item; }))
            return std::nullopt;
        items.push_back({*item, *price});
        if (comma == std::string_view::npos)
            break;
        text.remove_prefix(comma + 1);
        if (text.empty())
            return std::nullopt;
    }
    return items;
}

class GearCatalog
{
public:
    using Items = std::vector<GearItem>;
    using Lists = std::map<GearKey, Items>;

    GearCatalog() = default;
    explicit GearCatalog(Lists lists) : _lists(std::move(lists)) { }

    bool Add(std::string_view key, std::string_view text)
    {
        auto parsedKey = ParseGearKey(key);
        auto items = ParseGearItems(text);
        if (!parsedKey || !items)
            return false;
        for (auto const& item : *items)
            if (item.honor != 0)
                return false;
        _lists[*parsedKey] = std::move(*items);
        return true;
    }

    Items const& Find(std::uint8_t level, std::uint8_t playerClass, std::uint16_t tier,
        StarterRole role = StarterRole::None, bool bag = false) const
    {
        static Items const empty;
        if (role == StarterRole::None)
            role = DefaultStarterRole(playerClass);
        auto found = _lists.find({level, playerClass, tier, role, bag});
        return found == _lists.end() ? empty : found->second;
    }

    std::vector<StarterRole> Roles(std::uint8_t level, std::uint8_t playerClass) const
    {
        std::vector<StarterRole> roles;
        for (auto const& [key, items] : _lists)
            if (!key.bag && key.level == level && key.playerClass == playerClass && !items.empty())
                roles.push_back(key.role);
        return roles;
    }

    void RemoveBuild(GearKey const& rejected)
    {
        for (auto it = _lists.begin(); it != _lists.end();)
        {
            auto const& key = it->first;
            if (key.level == rejected.level && key.playerClass == rejected.playerClass &&
                key.tier == rejected.tier && key.role == rejected.role)
                it = _lists.erase(it);
            else
                ++it;
        }
    }

    Lists const& All() const { return _lists; }

private:
    Lists _lists;
};

inline bool DistinctStarterItems(GearCatalog::Items const& equipped, GearCatalog::Items const& bag)
{
    for (auto const& item : bag)
        if (std::any_of(equipped.begin(), equipped.end(),
                [&item](GearItem const& existing) { return existing.entry == item.entry; }))
            return false;
    return true;
}

// Share the same choice list between menu rendering and confirmation. Disabled
// starter delivery may still enroll a class with no configured gear at all.
inline std::vector<StarterRole> AvailableStarterRoles(GearCatalog const& catalog,
    std::uint8_t level, std::uint8_t playerClass, bool starterEnabled)
{
    auto roles = catalog.Roles(level, playerClass);
    if (!starterEnabled && roles.empty() && StarterRoleAllowed(playerClass, DefaultStarterRole(playerClass)))
        roles.push_back(DefaultStarterRole(playerClass));
    return roles;
}

// A held offhand may be preflighted after an ALREADY validated planned 1H
// main hand, even while the current weapon is 2H. Ordinary dual-wield weapons
// are not covered by this exception, nor are polearms, staves or fishing poles.
inline bool CanPlanHeldOffhand(std::uint32_t offhandType, std::uint32_t mainType, std::uint32_t mainSubclass)
{
    return (offhandType == 14 || offhandType == 23) && (mainType == 13 || mainType == 21) &&
        mainSubclass != 6 && mainSubclass != 10 && mainSubclass != 20;
}

// Explicit paired slots prevent preflighting two items into the same empty
// slot. 255 means let the core choose the normal slot for all other items.
inline std::vector<std::uint8_t> StarterEquipmentSlots(std::uint32_t inventoryType)
{
    switch (inventoryType)
    {
        case 11: return {10, 11}; // Rings.
        case 12: return {12, 13}; // Trinkets.
        case 13: return {15, 16}; // One-handed weapons (offhand still needs Dual Wield).
        default: return {255};
    }
}

// Proficiency spells only, restricted to the class's legal weapon families.
// No talents, Dual Wield, or skill-rank boosts are granted here.
inline std::uint32_t StarterWeaponProficiency(std::uint8_t playerClass, std::uint32_t subclass)
{
    auto allowed = [playerClass](std::initializer_list<std::uint8_t> classes)
    {
        return std::find(classes.begin(), classes.end(), playerClass) != classes.end();
    };
    switch (subclass)
    {
        case 0: return allowed({1,2,3,4,6,7}) ? 196 : 0; // One-handed axes.
        case 1: return allowed({1,2,3,6,7}) ? 197 : 0; // Two-handed axes.
        case 2: return allowed({1,3,4}) ? 264 : 0; // Bows.
        case 3: return allowed({1,3,4}) ? 266 : 0; // Guns.
        case 4: return allowed({1,2,4,5,6,7,11}) ? 198 : 0; // Maces.
        case 5: return allowed({1,2,6,7,11}) ? 199 : 0; // Two-handed maces.
        case 6: return allowed({1,2,3,6,11}) ? 200 : 0; // Polearms.
        case 7: return allowed({1,2,3,4,6,8,9}) ? 201 : 0; // Swords.
        case 8: return allowed({1,2,3,6}) ? 202 : 0; // Two-handed swords.
        case 10: return allowed({1,3,5,7,8,9,11}) ? 227 : 0; // Staves.
        case 13: return allowed({1,3,4,7,11}) ? 15590 : 0; // Fist weapons.
        case 15: return allowed({1,3,4,5,7,8,9,11}) ? 1180 : 0; // Daggers.
        case 16: return allowed({1,3,4}) ? 2567 : 0; // Thrown.
        case 18: return allowed({1,3,4}) ? 5011 : 0; // Crossbows.
        case 19: return allowed({5,8,9}) ? 5009 : 0; // Wands.
        default: return 0;
    }
}

inline bool GearArmorAllowed(std::uint8_t playerClass, std::uint8_t level, std::uint32_t armor, bool heirloom = false)
{
    if (armor == 0 || armor == 1)
        return true;
    if (armor == 2)
        return playerClass == 1 || playerClass == 2 || playerClass == 3 || playerClass == 4 ||
            playerClass == 6 || playerClass == 7 || playerClass == 11;
    if (armor == 3)
        return playerClass == 1 || playerClass == 2 || playerClass == 6 ||
            ((playerClass == 3 || playerClass == 7) && (level >= 40 || heirloom));
    if (armor == 4)
        return (playerClass == 1 || playerClass == 2) ? (level >= 40 || heirloom) : playerClass == 6;
    if (armor == 6)
        return playerClass == 1 || playerClass == 2 || playerClass == 7;
    return true; // Relics/offhands receive their normal class and skill checks.
}
}
#endif
