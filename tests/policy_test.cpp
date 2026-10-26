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
    LoginPrompt prompt;
    prompt.pending = true;
    Check(!prompt.Advance(false, 60000), "long intro does not consume the login prompt");
    Check(prompt.pending, "prompt stays pending during the cinematic");
    Check(!prompt.Advance(true, 500), "skip-cinematic Escape gets a settling delay");
    Check(prompt.Advance(true, 500), "prompt appears after cinematic completion without relogging");
    Check(!prompt.Advance(true, 10000), "automatic prompt is sent only once");
    LoginPrompt noIntro;
    noIntro.pending = true;
    Check(noIntro.Advance(true, 1000), "login without a cinematic still prompts");
    LoginPrompt interrupted;
    interrupted.pending = true;
    Check(!interrupted.Advance(true, 500), "ready interval begins counting down");
    Check(!interrupted.Advance(false, 1000), "unsafe state defers the prompt");
    Check(!interrupted.Advance(true, 500), "readiness must remain stable for the whole delay");
    Check(interrupted.Advance(true, 500), "deferred prompt appears once safe again");
    LoginPrompt disabled;
    Check(!disabled.Advance(true, 10000), "disabled or already chosen prompt never opens");
    Check(CanGainExperience(Mode::Pending), "closing selection does not pause experience");
    Check(CanGainExperience(Mode::Normal), "normal experience remains available");
    Check(!CanGainExperience(Mode::Battleground), "bracket characters still cannot gain experience");
    Check(CanChangeLevel(Mode::Pending, 2, 0, false), "undecided characters can reach level 2 naturally");
    Check(CanChangeLevel(Mode::Normal, 3, 0, false), "auto-normal characters can continue leveling");
    Check(!CanChangeLevel(Mode::Battleground, 20, 19, false), "bracket level changes remain blocked");
    Check(CanChangeLevel(Mode::Battleground, 19, 19, true), "initial bracket setup can still set its locked level");
    Check(!CanChangeLevel(Mode::Battleground, 29, 19, true), "internal level permission cannot change the selected bracket");
    Check(!ShouldBecomeNormal(Mode::Pending, 1), "closing at level 1 preserves eligibility");
    Check(ShouldBecomeNormal(Mode::Pending, 2), "reaching level 2 automatically finalizes normal mode");
    Check(ShouldBecomeNormal(Mode::Pending, 5), "a multilevel jump also finalizes normal mode");
    Check(!ShouldBecomeNormal(Mode::Battleground, 19), "selected bracket characters never auto-convert");
    Check(!ShouldBecomeNormal(Mode::Normal, 2), "normal choices do not trigger repeated conversion");
    auto levels = ParseLevels("19,29,39,49,59,69,79,80", 80);
    Check(levels == std::vector<std::uint8_t>({19,29,39,49,59,69,79,80}), "all default brackets are available");
    Check(ParseLevels(" 39 ,19,39, 29\t", 80) == std::vector<std::uint8_t>({19,29,39}), "whitespace and duplicate levels normalize");
    for (auto invalid : {"", "0", "81", "256", "-1", "19junk", "19,,29", ",19", "19,", "19, ", "19;29", "99999999999999999999999"})
        Check(ParseLevels(invalid, 80).empty(), std::string("reject malformed config: ") + invalid);
    Check(ParseLevels("19,79", 70).empty(), "reject a list exceeding the server cap");
    Check(ParseLevels("80", 80) == std::vector<std::uint8_t>({80}), "level 80 is within the supported cap");
    std::string manyLevels;
    for (unsigned level = 1; level <= MaxBracketChoices; ++level)
        manyLevels += (level == 1 ? "" : ",") + std::to_string(level);
    Check(ParseLevels(manyLevels, 80).size() == MaxBracketChoices, "maximum menu choices leave room for Back");
    Check(ParseLevels(manyLevels + ",32", 80).empty(), "oversized configuration cannot overflow the gossip menu");
    Check(CanChoose(Mode::Pending, 1, 19, levels), "a new character can choose a bracket");
    Check(CanChoose(Mode::Pending, 1, 80, levels), "new characters can choose the default level 80 bracket");
    Check(ParseLevels("19,80", 79).empty(), "level 80 still respects the server level cap");
    Check(ValidRecord(Mode::Battleground, 80), "level 80 lock survives persistence validation");
    Check(!CanChoose(Mode::Pending, 2, 19, levels), "a character above level 1 cannot enroll");
    Check(!CanChoose(Mode::Pending, 55, 59, levels), "Death Knight starting above level 1 cannot enroll");
    Check(!CanChoose(Mode::Pending, 79, 79, levels), "matching a bracket does not bypass level 1 eligibility");
    Check(!CanChoose(Mode::Pending, 55, 19, levels), "Death Knight cannot retain high-level abilities in level 19 bracket");
    Check(!CanChoose(Mode::Normal, 1, 19, levels), "normal decision is permanent");
    Check(!CanChoose(Mode::Battleground, 19, 29, levels), "a second confirmation cannot change bracket");
    Check(!CanChoose(Mode::Pending, 1, 78, levels), "a forged unconfigured bracket is rejected");
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
