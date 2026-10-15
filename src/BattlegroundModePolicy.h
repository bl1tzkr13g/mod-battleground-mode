#ifndef BATTLEGROUND_MODE_POLICY_H
#define BATTLEGROUND_MODE_POLICY_H

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace BattlegroundMode
{
// One gossip slot is reserved for Back. Keep within the client's/core's limit.
constexpr std::size_t MaxBracketChoices = 31;
enum class Mode : std::uint8_t { Pending = 0, Normal = 1, Battleground = 2 };

struct LoginPrompt
{
    static constexpr std::uint32_t Delay = 1000;
    bool pending = false;
    std::uint32_t remaining = Delay;

    bool Advance(bool ready, std::uint32_t elapsed)
    {
        if (!pending)
            return false;
        if (!ready)
        {
            // Include a fresh delay after the cinematic ends, so Escape used
            // to skip the intro cannot immediately dismiss the selection.
            remaining = Delay;
            return false;
        }
        if (elapsed < remaining)
        {
            remaining -= elapsed;
            return false;
        }
        pending = false;
        return true;
    }
};

// Reject the entire setting on malformed input. Never silently enable a level.
inline std::vector<std::uint8_t> ParseLevels(std::string_view text, std::uint8_t serverMax)
{
    std::vector<std::uint8_t> levels;
    while (!text.empty())
    {
        auto comma = text.find(',');
        auto token = text.substr(0, comma);
        auto first = token.find_first_not_of(" \t");
        if (first == std::string_view::npos)
            return {};
        token = token.substr(first, token.find_last_not_of(" \t") - first + 1);
        unsigned value = 0;
        auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
        if (error != std::errc{} || end != token.data() + token.size() || value < 1 || value > serverMax || value > 80)
            return {};
        levels.push_back(static_cast<std::uint8_t>(value));
        if (comma == std::string_view::npos)
            break;
        text.remove_prefix(comma + 1);
        if (text.empty())
            return {};
    }
    std::sort(levels.begin(), levels.end());
    levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
    if (levels.size() > MaxBracketChoices)
        return {};
    return levels;
}

inline bool CanChoose(Mode mode, std::uint8_t current, std::uint8_t target,
    std::vector<std::uint8_t> const& allowed)
{
    // Enrollment is only for level 1 characters, regardless of starting class.
    return mode == Mode::Pending && current == 1 && target >= current &&
        std::find(allowed.begin(), allowed.end(), target) != allowed.end();
}

inline bool ValidRecord(Mode mode, std::uint8_t level)
{
    return ((mode == Mode::Pending || mode == Mode::Normal) && level == 0) ||
        (mode == Mode::Battleground && level >= 1 && level <= 80);
}
}
#endif
