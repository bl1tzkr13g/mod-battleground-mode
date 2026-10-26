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

    Check(GearArmorAllowed(2, 19, 4, true), "heirloom plate uses the core's low-level mail adaptation");
    Check(GearArmorAllowed(3, 19, 3, true), "heirloom mail adapts for a low-level Hunter");
    Check(!GearArmorAllowed(8, 79, 4, true), "heirloom quality does not give a Mage plate proficiency");
    Check(!GearArmorAllowed(11, 19, 3, true), "heirloom mail does not bypass Druid armor restrictions");
    Check(StarterEquipmentSlots(12) == std::vector<std::uint8_t>({12,13}), "two trinkets receive distinct candidate slots");
    Check(StarterEquipmentSlots(13) == std::vector<std::uint8_t>({15,16}), "one-handed weapons can use separate hands");
    Check(StarterEquipmentSlots(17) == std::vector<std::uint8_t>({255}), "two-handed weapons use core slot selection");
    Check(StarterWeaponProficiency(1, 1) == 197, "Warrior can train the starter axe proficiency");
    Check(StarterWeaponProficiency(2, 8) == 202, "Paladin can train the starter sword proficiency");
    Check(StarterWeaponProficiency(3, 2) == 264, "Hunter bow proficiency is independent of race");
    Check(StarterWeaponProficiency(11, 5) == 199, "Druid can train the starter two-handed mace");
    Check(!StarterWeaponProficiency(8, 1), "Mage cannot gain axe proficiency from configuration");
    Check(!StarterWeaponProficiency(2, 15), "Paladin cannot gain dagger proficiency from configuration");

    // Regression: a fresh backpack with three starting items has 13 free slots,
    // insufficient to bag a 16-piece kit, but enough for four displaced items
    // and one staging slot. Food/hearthstone need not be deleted to auto-equip.
    int newItems[16] = {}, oldItems[16] = {};
    std::vector<int*> incoming, replaced;
    for (int& item : newItems)
        incoming.push_back(&item);
    for (unsigned i = 0; i < 4; ++i)
        replaced.push_back(&oldItems[i]);
    auto reservation = StarterStorageReservation(incoming, replaced, true);
    Check(reservation.size() == 5 && reservation.back() == incoming.front(), "auto-equip reserves displaced gear and one staging item");
    Check(StarterStorageReservation(incoming, replaced, false) == incoming, "bag-only delivery still reserves the entire kit");
    Check(reservation.size() <= 13 && incoming.size() > 13, "full caster kit fits a fresh backpack by equipping incrementally");
    bool safe = true;
    // Exercise every occupied/empty target combination for a 16-piece kit.
    // At each step a new piece temporarily consumes a bag slot; an empty
    // equipment target releases it, while a replacement retains the old item.
    for (unsigned mask = 0; mask < (1u << 16); ++mask)
    {
        replaced.clear();
        for (unsigned i = 0; i < 16; ++i)
            if (mask & (1u << i))
                replaced.push_back(&oldItems[i]);
        auto slots = StarterStorageReservation(incoming, replaced, true);
        int free = static_cast<int>(slots.size());
        unsigned preserved = 0;
        for (unsigned i = 0; i < 16; ++i)
        {
            if (free <= 0)
                safe = false;
            --free; // New item staged in a bag.
            if (mask & (1u << i))
                ++preserved; // Swap leaves old equipment in the vacated bag slot.
            else
                ++free; // New item moves to an empty equipment target.
        }
        if (preserved != replaced.size() || free < 1)
            safe = false;
    }
    Check(safe, "reservation retains all replaced gear and staging capacity for every slot ordering");

    Check(ValidStarterBag(1,0,18,20,0), "Frostweave Bag is a valid generic starter bag");
    Check(!ValidStarterBag(4,1,1,20,0), "ordinary armor cannot be configured as a starter bag");
    Check(!ValidStarterBag(1,2,18,20,32), "specialized bags cannot stand in for general starter storage");
    Check(!ValidStarterBag(1,0,18,0,0) && !ValidStarterBag(1,0,18,37,0), "invalid container sizes are rejected");
    Check(StarterStorageReservation(incoming, {}, false).size() <= 20, "new 20-slot bag can hold the complete 16-piece kit even in bag-only mode");

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
    auto contains = [](GearCatalog::Items const& items, std::uint32_t entry)
    {
        return std::any_of(items.begin(), items.end(), [entry](GearItem const& item) { return item.entry == entry; });
    };
    struct HeirloomLoadout { std::uint8_t playerClass; std::uint32_t shoulders, chest, weapon, trinket; };
    for (auto level : {19,29,39,49,59,69,79})
        for (auto kit : {HeirloomLoadout{1,42949,48685,42943,42991}, {2,42949,48685,44092,42991},
                {3,42950,48677,42944,42991}, {4,42952,48689,42944,42991}, {5,42985,48691,42947,42992},
                {7,42950,48677,48716,42991}, {8,42985,48691,42947,42992}, {9,42985,48691,42947,42992},
                {11,42952,48689,48718,42991}})
        {
            auto const& items = shipped.Find(level, kit.playerClass, 1);
            Check(contains(items, kit.shoulders) && contains(items, kit.chest), "every bracket includes class heirloom armor");
            Check(contains(items, kit.weapon) && contains(items, kit.trinket), "every bracket includes class heirloom weapon and trinket");
            Check(contains(items, 50255) && contains(items, 44098) && contains(items, 44097), "all brackets define ring and both faction insignia options");
            Check(!contains(items, 44090) && !contains(items, 38691), "test and non-heirloom starting weapons are excluded");
        }
    Check(contains(shipped.Find(19, 4, 1), 44091), "Rogue also gets the offhand dagger");
    Check(contains(shipped.Find(79, 3, 1), 42946), "Hunter also gets the ranged heirloom");
    for (auto level : {19,29,39,49,59,69,79})
        for (auto playerClass : {5,8,9})
        {
            auto const& items = shipped.Find(level, playerClass, 1);
            Check(items.size() == 17, "caster config has 16 equipped pieces plus one alternate faction insignia");
            for (std::uint32_t entry = 900019; entry <= 900028; ++entry)
                Check(contains(items, entry), "every caster bracket includes all ten custom completion pieces");
            Check(contains(items, 42985) && contains(items, 48691) && contains(items, 42947), "existing heirloom shoulders, robe and staff remain");
        }
    for (auto level : {19,29,39,49,59,69,79})
        for (auto playerClass : {3,4,7,11})
        {
            auto const& items = shipped.Find(level, playerClass, 1);
            bool leather = playerClass == 4 || playerClass == 11 || level < 40;
            for (std::uint32_t entry = 900029; entry <= 900037; ++entry)
                Check(contains(items, entry) == leather, "leather completion follows class and bracket armor needs");
            if (leather)
            {
                auto expected = (playerClass == 3 || playerClass == 4) ? 17u : 16u;
                Check(items.size() == expected, "leather kit has the expected piece count plus alternate insignia");
                auto shoulders = (playerClass == 4 || playerClass == 11) ? 42952u : 42950u;
                auto chest = (playerClass == 4 || playerClass == 11) ? 48689u : 48677u;
                Check(contains(items, shoulders) && contains(items, chest), "appropriate existing heirloom chest and shoulders remain");
            }
        }
    std::cout << checks << " gear checks passed\n";
}
