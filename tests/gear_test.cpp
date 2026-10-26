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
    Check(!catalog.Add("BattlegroundMode.Gear.Level19.Tier2.Paladin", "12982:75"), "upgrade tiers are retired");
    Check(!catalog.Add("BattlegroundMode.Gear.Level19.Tier6.Paladin", "12982:75"), "additional upgrade tiers cannot return implicitly");
    Check(catalog.Add("BattlegroundMode.Gear.Level19.Tier1.Paladin.Holy", "42948:0,900085:0"), "Holy build parses independently");
    Check(catalog.Find(19,2,1,StarterRole::Holy).front().entry == 42948, "chosen build selects its own kit");
    Check(catalog.Find(19,2,1).front().entry == 6573, "legacy class key maps only to the default build");
    Check(catalog.Find(29,2,1,StarterRole::Holy).empty(), "build cannot leak across brackets");
    Check(catalog.Find(19,1,1,StarterRole::Holy).empty(), "build cannot leak across classes");
    Check(!catalog.Add("BattlegroundMode.Gear.Level19.Tier1.Warrior.Holy", "42948:0"), "class-inappropriate build rejected");
    Check(catalog.Add("BattlegroundMode.Gear.Level19.Tier1.Paladin.Protection", ""), "empty Protection keys disable that build without offering a placeholder");
    Check(catalog.Roles(19,2).size() == 2, "only configured nonempty builds are offered");
    Check(catalog.Add("BattlegroundMode.Gear.Level29.Tier1.Paladin.Holy", ""), "empty build intentionally disabled");
    Check(catalog.Roles(29,2).empty(), "empty builds do not appear in selection");
    Check(AvailableStarterRoles(catalog,29,2,true).empty(), "enabled delivery never invents an empty fallback");
    Check(AvailableStarterRoles(catalog,29,2,false) == std::vector<StarterRole>{StarterRole::Retribution},
        "disabled grants permit default enrollment when no gear is configured");
    Check(catalog.Add("BattlegroundMode.Gear.Level39.Tier1.Paladin.Holy", "42948:0"), "single-role catalog can omit the default");
    Check(AvailableStarterRoles(catalog,39,2,false) == std::vector<StarterRole>{StarterRole::Holy},
        "disabled grants cannot accept an unoffered default when other builds are present");
    Check(AvailableStarterRoles(catalog,19,10,false).empty(), "invalid class cannot obtain disabled-delivery fallback");
    GearCatalog mixed;
    Check(mixed.Add("BattlegroundMode.Gear.Level19.Tier1.Mage.Damage", "42947:0"), "staff equipped list parses");
    Check(mixed.Add("BattlegroundMode.Gear.Bag.Level19.Tier1.Mage.Damage", "900086:0,900087:0"), "bag-only alternatives parse separately");
    Check(mixed.Find(19,8,1).size() == 1 && mixed.Find(19,8,1,StarterRole::Damage,true).size() == 2,
        "bag weapons do not enter the equipped list");
    Check(DistinctStarterItems(mixed.Find(19,8,1),mixed.Find(19,8,1,StarterRole::Damage,true)), "staff and alternate weapons can share a grant");
    Check(!DistinctStarterItems({{42947,0}},{{42947,0}}), "duplicate items across equipped/bag lists block delivery");
    Check(mixed.Add("BattlegroundMode.Gear.Bag.Level29.Tier1.Paladin.Holy", "6383:0"), "a standalone bag list can be parsed for diagnostics");
    Check(mixed.Roles(29,2).empty(), "bag-only catalog cannot expose a build with no primary kit");
    Check(mixed.Find(29,8,1,StarterRole::Damage,true).empty(), "bag choices do not leak across brackets");
    Check(mixed.Find(19,2,1,StarterRole::Holy,true).empty(), "bag choices do not leak across classes or roles");
    auto rejected = *ParseGearKey("BattlegroundMode.Gear.Bag.Level19.Tier1.Mage.Damage");
    mixed.RemoveBuild(rejected);
    Check(mixed.Roles(19,8).empty() && mixed.Find(19,8,1,StarterRole::Damage,true).empty(),
        "invalid alternative removes both lists of only the affected build");
    Check(mixed.Find(29,2,1,StarterRole::Holy,true).size() == 1, "invalid build removal preserves unrelated catalog entries");
    Check(!mixed.Add("BattlegroundMode.Gear.Bag.Level19.Tier1.Mage.Damage", "900086:10"), "bag alternatives must be free");
    for (unsigned cls = 1; cls <= 11; ++cls)
        for (unsigned id = 0; id <= 255; ++id)
        {
            auto role = static_cast<StarterRole>(id);
            bool expected = cls != 10 && (role == DefaultStarterRole(cls) ||
                (cls == 2 && id == 2) || ((cls == 1 || cls == 2) && id == 9) || ((cls == 7 || cls == 11) && id == 6) ||
                (cls == 7 && id == 8) || (cls == 11 && id == 5));
            Check(StarterRoleAllowed(cls,role) == expected, "role validation rejects wrong-class and forged IDs");
        }
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

    int alternateItems[2] = {};
    std::vector<int*> extras{&alternateItems[0], &alternateItems[1]};
    auto complete = incoming;
    complete.insert(complete.end(),extras.begin(),extras.end());
    for (unsigned mask = 0; mask < (1u << 16); ++mask)
    {
        replaced.clear();
        for (unsigned i = 0; i < 16; ++i)
            if (mask & (1u << i))
                replaced.push_back(&oldItems[i]);
        auto slots = StarterStorageReservation(complete,replaced,true,extras);
        int free = static_cast<int>(slots.size());
        for (unsigned i = 0; i < incoming.size(); ++i)
        {
            if (--free < 0)
                safe = false;
            if (!(mask & (1u << i)))
                ++free;
        }
        free -= static_cast<int>(extras.size()); // Alternatives are stored last and never equipped.
        if (free < 1 || slots.size() != replaced.size() + extras.size() + 1)
            safe = false;
    }
    Check(safe, "mixed reservation preserves displaced gear and both bag alternatives for all target occupancies");
    Check(StarterStorageReservation(complete,replaced,false,extras) == complete,
        "bag-only mode reserves all items once without double-counting alternatives");

    Check(ValidStarterBag(1,0,18,20,0), "Frostweave Bag is a valid generic starter bag");
    Check(!ValidStarterBag(4,1,1,20,0), "ordinary armor cannot be configured as a starter bag");
    Check(!ValidStarterBag(1,2,18,20,32), "specialized bags cannot stand in for general starter storage");
    Check(!ValidStarterBag(1,0,18,0,0) && !ValidStarterBag(1,0,18,37,0), "invalid container sizes are rejected");
    Check(StarterStorageReservation(incoming, {}, false).size() <= 20, "new 20-slot bag can hold the complete 16-piece kit even in bag-only mode");

    Check(CanPlanHeldOffhand(23,21,4), "Holy mace can precede a held tome while replacing an old 2H weapon");
    Check(CanPlanHeldOffhand(14,13,7), "a validated one-handed sword can precede a shield");
    Check(!CanPlanHeldOffhand(23,17,5), "two-handed mace cannot be planned with a held offhand");
    Check(!CanPlanHeldOffhand(13,21,4), "ordinary offhand weapons do not bypass dual-wield checks");
    Check(!CanPlanHeldOffhand(23,13,6) && !CanPlanHeldOffhand(23,13,10) && !CanPlanHeldOffhand(23,13,20),
        "polearm, staff and fishing pole restrictions remain intact");

    Check(argc == 2, "configuration path provided");
    std::ifstream config(argv[1]);
    Check(config.good(), "distributed config is readable");
    GearCatalog shipped;
    unsigned lists = 0, bagLists = 0;
    for (std::string line; std::getline(config, line);)
    {
        if (line.rfind("BattlegroundMode.Gear.Level", 0) != 0 && line.rfind("BattlegroundMode.Gear.Bag.Level", 0) != 0)
            continue;
        auto equals = line.find('=');
        auto key = line.substr(0, line.find_last_not_of(" \t", equals - 1) + 1);
        auto firstQuote = line.find('"', equals);
        auto lastQuote = line.rfind('"');
        Check(firstQuote != std::string::npos && lastQuote > firstQuote, "config list is quoted");
        Check(shipped.Add(key, line.substr(firstQuote + 1, lastQuote - firstQuote - 1)), "shipped catalog syntax and prices are valid");
        if (line.rfind("BattlegroundMode.Gear.Bag.Level", 0) == 0)
            ++bagLists;
        else
            ++lists;
    }
    Check(bagLists == 56, "eight caster class/build alternatives are configured across seven brackets");
    Check(lists == 112, "seven brackets include sixteen completed class/build choices each");
    Check(!shipped.Find(19, 2, 1).empty(), "level 19 Paladin starter list is populated");
    Check(!shipped.Find(79, 8, 1).empty(), "level 79 Mage starter list is populated");
    Check(shipped.Find(79, 1, 4).empty(), "upgrade catalog is removed");
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
    for (auto level : {19,29,39,49,59,69,79})
        for (auto playerClass : {3,7})
        {
            auto const& items = shipped.Find(level, playerClass, 1);
            bool mail = level >= 40;
            for (std::uint32_t entry = 900038; entry <= 900046; ++entry)
                Check(contains(items, entry) == mail, "agility mail completion is granted only to mail-level Hunter/Shaman");
            if (mail)
            {
                Check(items.size() == (playerClass == 3 ? 17u : 16u), "mail kit preserves expected piece count and faction option");
                Check(contains(items, 42950) && contains(items, 48677), "mail kit retains existing heirloom chest and shoulders");
            }
        }
    for (auto level : {19,29,39,49,59,69,79})
        for (auto playerClass : {1,2})
        {
            auto const& items = shipped.Find(level, playerClass, 1);
            for (std::uint32_t entry = 900047; entry <= 900055; ++entry)
                Check(contains(items, entry), "Warrior/Paladin get strength plate completion at every bracket");
            Check(items.size() == 16, "strength kit has 15 equipped pieces plus one alternate insignia");
            Check(contains(items, 42949) && contains(items, 48685), "strength kit retains existing Valor shoulders and chest");
        }
    for (auto [level, shield] : {std::pair<unsigned,unsigned>{19,4821}, {29,4477}, {39,14825},
            {49,8282}, {59,15943}, {69,31200}, {79,37810}})
        for (auto playerClass : {1,2})
        {
            auto const& items = shipped.Find(level, playerClass, 1, StarterRole::Protection);
            auto roles = AvailableStarterRoles(shipped, level, playerClass, true);
            Check(std::find(roles.begin(), roles.end(), StarterRole::Protection) != roles.end(),
                "Protection is selectable for both shield classes at every bracket");
            Check(items.size() == 17, "Protection has sixteen equipped items plus the alternate faction insignia");
            for (std::uint32_t entry = 900047; entry <= 900055; ++entry)
                Check(contains(items,entry), "Protection retains all strength/stamina Valor completion pieces");
            Check(contains(items,42949) && contains(items,48685), "Protection retains scaling Valor shoulders/chest");
            Check(contains(items,42991) && contains(items,50255) && contains(items,44098) && contains(items,44097),
                "Protection retains physical trinket, ring and faction-filtered insignia");
            auto sword = std::find_if(items.begin(),items.end(),[](GearItem const& x) { return x.entry == 42945; });
            auto offhand = std::find_if(items.begin(),items.end(),[shield](GearItem const& x) { return x.entry == shield; });
            Check(sword != items.end() && offhand != items.end() && sword < offhand,
                "Protection plans its legal 1H sword before the bracket shield over an old 2H weapon");
            Check(!contains(items,42943) && !contains(items,44092) && !contains(items,42947) && !contains(items,900085),
                "Protection excludes damage 2H weapons, caster staves and the Holy tome");
        }
    for (auto [level, shield] : {std::pair<unsigned,unsigned>{19,6383}, {29,14608}, {39,14777},
            {49,17718}, {59,18485}, {69,31287}, {79,35636}})
    {
        for (auto [playerClass, role] : {std::pair<unsigned,StarterRole>{5,StarterRole::Damage},
                {8,StarterRole::Damage}, {9,StarterRole::Damage}, {11,StarterRole::Balance},
                {11,StarterRole::Restoration}, {7,StarterRole::Elemental}, {7,StarterRole::Restoration}, {2,StarterRole::Holy}})
        {
            auto const& primary = shipped.Find(level,playerClass,1,role);
            auto const& extras = shipped.Find(level,playerClass,1,role,true);
            Check(DistinctStarterItems(primary,extras), "no extra duplicates a primary item in any caster loadout");
            if (playerClass == 2)
                Check(extras.size() == 1 && contains(extras,shield) && contains(primary,42948) && contains(primary,900085),
                    "Holy keeps its mace/tome equipped and receives only a shield alternative");
            else if (playerClass == 7)
                Check(extras.size() == 2 && contains(extras,42948) && contains(extras,shield) && contains(primary,42947),
                    "caster Shaman keeps its staff equipped and gets a caster mace/shield in bags");
            else
                Check(extras.size() == 2 && contains(extras,900086) && contains(extras,900087) && contains(primary,42947),
                    "cloth and Druid casters keep staff equipped and get legal dagger/tome alternatives");
            Check(primary.size() + extras.size() - 1 <= 20, "faction-filtered complete bag-only caster grant fits one empty starter bag");
        }
        Check(shipped.Find(level,1,1,StarterRole::Protection,true).empty() &&
            shipped.Find(level,2,1,StarterRole::Retribution,true).empty() &&
            shipped.Find(level,11,1,StarterRole::Feral,true).empty() &&
            shipped.Find(level,7,1,StarterRole::Enhancement,true).empty(), "caster bag alternatives do not leak into physical builds");
    }
    struct AlternateLoadout { std::uint8_t playerClass; StarterRole role; };
    for (auto example : {AlternateLoadout{11,StarterRole::Balance}, {11,StarterRole::Restoration},
            {7,StarterRole::Elemental}, {7,StarterRole::Restoration}, {2,StarterRole::Holy}})
    {
        bool druid = example.playerClass == 11;
        bool holy = example.playerClass == 2;

        for (auto level : {19,29,39,49,59,69,79})
        {
            auto const& items = shipped.Find(level, example.playerClass, 1, example.role);
            auto choices = shipped.Roles(level, example.playerClass);
            Check(std::find(choices.begin(),choices.end(),example.role) != choices.end(), "all completed alternate builds are selectable at every bracket");
            Check(items.size() == (holy ? 17u : 16u), "caster alternate has the correct piece count plus alternate insignia");
            std::uint32_t firstEntry = holy ? 900074 : !druid && level >= 40 ? 900065 : 900056;
            for (std::uint32_t entry = firstEntry; entry < firstEntry + (holy ? 12u : 9u); ++entry)
                Check(contains(items, entry), "caster alternate selects the correct armor family at each bracket");
            Check(contains(items, holy ? 42948 : 42947) && contains(items, 42992) && !contains(items, 42991), "alternate uses a class-legal caster weapon and stat trinket");
            Check(contains(items, holy ? 900083 : druid ? 42984 : 42951) && contains(items, holy ? 900084 : druid ? 48687 : 48683), "alternate uses class-appropriate caster chest/shoulders");
            if (holy)
            {
                auto mace = std::find_if(items.begin(), items.end(), [](GearItem const& x) { return x.entry == 42948; });
                auto tome = std::find_if(items.begin(), items.end(), [](GearItem const& x) { return x.entry == 900085; });
                Check(mace != items.end() && tome != items.end() && mace < tome, "Holy mace is planned and equipped before its tome");
                Check(!contains(items, 48685) && !contains(items, 44100), "Holy alternate does not substitute physical-stat stock armor");
            }
        }
    }
    std::cout << checks << " gear checks passed\n";
}
