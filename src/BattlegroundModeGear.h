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
// and one reusable working item; bag-only delivery must hold the entire kit.
template <typename Item>
std::vector<Item*> StarterStorageReservation(std::vector<Item*> const& incoming,
    std::vector<Item*> const& replaced, bool autoEquip)
{
    if (!autoEquip)
        return incoming;
    auto storage = replaced;
    if (!incoming.empty())
        storage.push_back(incoming.front());
    return storage;
}

struct GearKey
{
    std::uint8_t level;
    std::uint8_t playerClass;
    std::uint16_t tier;

    bool operator<(GearKey const& other) const
    {
        return std::tie(level, playerClass, tier) < std::tie(other.level, other.playerClass, other.tier);
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
    constexpr std::string_view prefix = "BattlegroundMode.Gear.Level";
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
    for (std::uint8_t playerClass = 1; playerClass < 12; ++playerClass)
        if (!classes[playerClass].empty() && key == classes[playerClass])
            return GearKey{static_cast<std::uint8_t>(*level), playerClass, static_cast<std::uint16_t>(*tier)};
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
            if ((parsedKey->tier == 1 && item.honor != 0) || (parsedKey->tier > 1 && item.honor == 0))
                return false;
        _lists[*parsedKey] = std::move(*items);
        return true;
    }

    Items const& Find(std::uint8_t level, std::uint8_t playerClass, std::uint16_t tier) const
    {
        static Items const empty;
        auto found = _lists.find({level, playerClass, tier});
        return found == _lists.end() ? empty : found->second;
    }

    Lists const& All() const { return _lists; }

private:
    Lists _lists;
};

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
