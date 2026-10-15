#ifndef BATTLEGROUND_MODE_GEAR_H
#define BATTLEGROUND_MODE_GEAR_H

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace BattlegroundMode
{
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

inline bool GearArmorAllowed(std::uint8_t playerClass, std::uint8_t level, std::uint32_t armor)
{
    if (armor == 0 || armor == 1)
        return true;
    if (armor == 2)
        return playerClass == 1 || playerClass == 2 || playerClass == 3 || playerClass == 4 ||
            playerClass == 6 || playerClass == 7 || playerClass == 11;
    if (armor == 3)
        return playerClass == 1 || playerClass == 2 || playerClass == 6 ||
            ((playerClass == 3 || playerClass == 7) && level >= 40);
    if (armor == 4)
        return (playerClass == 1 || playerClass == 2) ? level >= 40 : playerClass == 6;
    if (armor == 6)
        return playerClass == 1 || playerClass == 2 || playerClass == 7;
    return true; // Relics/offhands receive their normal class and skill checks.
}
}
#endif
