#include "BattlegroundModePolicy.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace BattlegroundMode;

namespace
{
unsigned checks = 0;
void Check(bool condition, std::string const& description)
{
    ++checks;
    if (!condition)
    {
        std::cerr << "FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    auto levels = ParseLevels("19,29,39,49,59,69,79", 80);
    Check(levels == std::vector<std::uint8_t>({19,29,39,49,59,69,79}), "all default brackets are available");
    Check(ParseLevels(" 39 ,19,39, 29\t", 80) == std::vector<std::uint8_t>({19,29,39}), "whitespace and duplicate levels normalize");
    for (auto invalid : {"", "0", "81", "256", "-1", "19junk", "19,,29", ",19", "19,", "19, ", "19;29", "99999999999999999999999"})
        Check(ParseLevels(invalid, 80).empty(), std::string("reject malformed config: ") + invalid);
    Check(ParseLevels("19,79", 70).empty(), "reject a list exceeding the server cap");
    Check(ParseLevels("80", 80) == std::vector<std::uint8_t>({80}), "level 80 is explicit opt-in");
    std::string manyLevels;
    for (unsigned level = 1; level <= MaxBracketChoices; ++level)
        manyLevels += (level == 1 ? "" : ",") + std::to_string(level);
    Check(ParseLevels(manyLevels, 80).size() == MaxBracketChoices, "maximum menu choices leave room for Back");
    Check(ParseLevels(manyLevels + ",32", 80).empty(), "oversized configuration cannot overflow the gossip menu");
    Check(CanChoose(Mode::Pending, 1, 19, levels), "a new character can choose a bracket");
    Check(CanChoose(Mode::Pending, 55, 59, levels), "Death Knight can choose a higher bracket");
    Check(!CanChoose(Mode::Pending, 55, 19, levels), "Death Knight cannot retain high-level abilities in level 19 bracket");
    Check(!CanChoose(Mode::Normal, 1, 19, levels), "normal decision is permanent");
    Check(!CanChoose(Mode::Battleground, 19, 29, levels), "a second confirmation cannot change bracket");
    Check(!CanChoose(Mode::Pending, 1, 80, levels), "a forged unconfigured bracket is rejected");
    Check(!CanChoose(Mode::Pending, 1, 19, {}), "invalid configuration cannot enroll a character");
    Check(ValidRecord(Mode::Pending, 0), "pending record is valid");
    Check(ValidRecord(Mode::Normal, 0), "normal record is valid");
    Check(ValidRecord(Mode::Battleground, 19), "saved bracket remains valid independently of current allowed list");
    Check(!ValidRecord(Mode::Battleground, 0), "zero-level lock is corrupt");
    Check(!ValidRecord(Mode::Battleground, 81), "unsupported saved level is corrupt");
    Check(!ValidRecord(Mode::Normal, 19), "normal record must not carry a lock");
    Check(!ValidRecord(static_cast<Mode>(3), 19), "unknown mode is corrupt");
    Check(!ValidRecord(static_cast<Mode>(255), 0), "invalid mode cannot masquerade as an existing normal character");
    std::cout << checks << " policy checks passed\n";
}
