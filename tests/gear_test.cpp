#include "BattlegroundModeGear.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

using namespace BattlegroundMode;

namespace
{
unsigned checks = 0;
void Check(bool value, char const* description)
{
    ++checks;
    if (!value)
    {
        std::cerr << "FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main(int argc, char** argv)
{
    Check(CanDeliverStarter(true, true, StarterState::Pending), "unclaimed BG starter delivery is permitted");
    Check(!CanDeliverStarter(true, true, StarterState::Claimed), "interrupted claims cannot duplicate a starter kit");
    Check(!CanDeliverStarter(true, true, StarterState::Delivered), "relog and repeated requests cannot overwrite upgrades");
    Check(!CanDeliverStarter(false, true, StarterState::Pending), "starter delivery respects its enable setting");
    Check(!CanDeliverStarter(true, false, StarterState::Pending), "normal characters never receive free BG kits");
    GearCatalog catalog;
    Check(catalog.Add("BattlegroundMode.Gear.Level19.Tier1.Paladin", "6573:0, 6574:0"), "free starter list parses");
    Check(catalog.Add("BattlegroundMode.Gear.Level19.Tier2.Paladin", "12982:75,6460:100"), "per-piece honor prices parse");
    auto const& gear = catalog.Find(19, 2, 2);
    Check(gear.size() == 2 && gear[0].entry == 12982 && gear[0].honor == 75 && gear[1].honor == 100,
        "item and honor prices remain paired");
    Check(catalog.Find(29, 2, 2).empty(), "vendor cannot leak gear into another bracket");
    Check(catalog.Find(19, 1, 2).empty(), "vendor cannot leak gear into another class");
    Check(catalog.Find(19, 2, 3).empty(), "missing tiers have no implicit fallback");
    Check(catalog.Add("BattlegroundMode.Gear.Level19.Tier6.Paladin", "12994:700"), "new tiers need no code changes");
    Check(catalog.Find(19, 2, 6).front().honor == 700, "new tiers preserve their configured price");
    Check(catalog.Add("BattlegroundMode.Gear.Level19.Tier5.Paladin", ""), "empty placeholder tiers are valid");
    for (auto key : {"BattlegroundMode.Gear.Level0.Tier1.Paladin", "BattlegroundMode.Gear.Level81.Tier1.Paladin",
        "BattlegroundMode.Gear.Level19.Tier0.Paladin", "BattlegroundMode.Gear.Level19.Tier65536.Paladin",
        "BattlegroundMode.Gear.Level19.Tier1.Unknown", "BattlegroundMode.Gear.Level19.Tier1.Paladin.Extra"})
        Check(!ParseGearKey(key), "bad catalog keys are rejected");
    for (auto text : {"6573", "6573:-1", "6573:", "6573:abc", "6573:4294967296", "0:75",
        "6573:75,", "6573:75,,6574:75", "6573:75,6573:100", "6573:75:100"})
        Check(!ParseGearItems(text), "bad or duplicate item/price pairs are rejected");
    Check(!catalog.Add("BattlegroundMode.Gear.Level19.Tier1.Warrior", "6573:75"), "starter gear cannot cost honor");
    Check(!catalog.Add("BattlegroundMode.Gear.Level19.Tier2.Warrior", "6573:0"), "vendor upgrades cannot accidentally be free");
    Check(GearArmorAllowed(2, 19, 3), "level 19 Paladin can use mail");
    Check(!GearArmorAllowed(2, 19, 4), "level 19 Paladin cannot use plate");
    Check(!GearArmorAllowed(3, 39, 3), "Hunter cannot use mail before level 40");
    Check(GearArmorAllowed(3, 49, 3), "level 49 Hunter can use mail");
    Check(!GearArmorAllowed(8, 79, 2), "Mage cannot use leather");
    Check(GearArmorAllowed(11, 79, 2), "Druid can use leather");

    Check(argc == 2, "configuration path provided");
    std::ifstream config(argv[1]);
    Check(config.good(), "distributed config is readable");
    GearCatalog shipped;
    unsigned lists = 0;
    for (std::string line; std::getline(config, line);)
    {
        if (line.rfind("BattlegroundMode.Gear.Level", 0) != 0)
            continue;
        auto equals = line.find('=');
        auto key = line.substr(0, line.find_last_not_of(" \t", equals - 1) + 1);
        auto firstQuote = line.find('"', equals);
        auto lastQuote = line.rfind('"');
        Check(firstQuote != std::string::npos && lastQuote > firstQuote, "config list is quoted");
        Check(shipped.Add(key, line.substr(firstQuote + 1, lastQuote - firstQuote - 1)), "shipped catalog syntax and prices are valid");
        ++lists;
    }
    Check(lists >= 350, "all seven brackets, five tiers, and ten classes are represented");
    Check(!shipped.Find(19, 2, 1).empty(), "level 19 Paladin starter list is populated");
    Check(!shipped.Find(79, 8, 1).empty(), "level 79 Mage starter list is populated");
    Check(!shipped.Find(79, 1, 4).empty(), "level 79 Warrior has a priced PvP upgrade catalog");
    Check(shipped.Find(19, 6, 1).empty(), "Death Knight has no starter kit while enrollment is ineligible");
    std::cout << checks << " gear checks passed\n";
}
