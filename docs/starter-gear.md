# Starter armor and configurable honor catalogs

The config is the editable source for gear and prices. No item lists or honor
prices are stored in the module's character database.

## Editing a character's gear list

Edit the live `mod_battleground_mode.conf` in your server's installed module
configuration directory. The repository distributes the defaults in
[`conf/mod_battleground_mode.conf.dist`](../conf/mod_battleground_mode.conf.dist).
Preserve your existing settings when copying the new gear sections into it.

```ini
# LEVEL 19 - TIER 1 STARTER GEAR
# Paladin: free starter armor
BattlegroundMode.Gear.Level19.Tier1.Paladin = "6573:0,6574:0,6576:0,6577:0,6578:0,6579:0,6580:0"

# LEVEL 19 - TIER 2 GEAR
# Paladin: boots cost 75 honor; each belt/bracer costs 50 honor
BattlegroundMode.Gear.Level19.Tier2.Paladin = "12982:75,5425:50,14147:50"

# LEVEL 19 - TIER 6 GEAR (example of adding a tier)
BattlegroundMode.Gear.Level19.Tier6.Paladin = "12994:1200"
```

Each pair is **item ID:honor price**, separated by commas. Prices apply to one
item, not an entire set. Item names are comments above each default list. Tier 1
must be free. Every item in tiers 2 and above must have an explicit positive
integer price within the server's configured honor cap. Empty quotes mean no
items, with no fallback to another class or bracket.

The defaults cover levels 19, 29, 39, 49, 59, 69, and 79, with a separate entry for
every WotLK class in each of five tiers. Class names are case-sensitive:
`Paladin`, `Warrior`, `Hunter`, `Rogue`, `Priest`, `Shaman`, `Mage`, `Warlock`,
`Druid`, `DeathKnight`. Death Knight lists are intentionally empty because its
starting level makes it ineligible for the module's level-1 enrollment.

Five tiers are laid out: Starter, Soldier, Veteran, Champion, Elite. Tier 1 has
starter armor, tiers 2 and 3 have upgrade candidates, and tier 4 has additional
expansion/PvP candidates at levels 59, 69, and 79. Other tier-4 lists and all
tier-5 lists are empty placeholders. They are not completed five-tier sets.
Stock gear at low brackets does not provide five distinct full sets with
consistent stat improvements. Those tiers need further item curation or custom
items before they can become complete progression steps.

Add a new tier by adding keys with `Tier6`, `Tier7`, etc. No tier-count setting or
code change is required; supported tier numbers are 1 through 65535. Levels 1
through 80 are supported in keys, separately from which levels you allow players
to select. Add each desired class explicitly. Use `.reload config` to refresh the
catalog after edits; existing characters' bracket selections are unaffected.

## Current runtime behavior

The C++ loader reads these lists and retains each item together with its honor
price. Once item templates are loaded, it rejects a whole list if it contains an
unknown item, an item above the bracket, incompatible armor/class restrictions,
or a price above the honor cap. Malformed keys, duplicate IDs within a list,
missing prices, negative prices, and incorrect zero prices are rejected with a
server log message. Weapon skill, race, profession, and reputation checks must
also be enforced by the future purchase/equip flow if you add restricted items.

A Battleground Mode character can use **`.bggear 2`** (or another tier number) to
preview only the exact saved bracket/class list and its prices. Other brackets,
other classes, and Normal Mode characters do not receive that list. This command
is a preview: it does not grant, sell, equip, or deduct honor for anything.

Automatic one-time starter grants and the NPC purchase flow are the next
implementation step. The future vendor must use tiers 2+ and recheck the same
bracket/class, live price, honor balance, and inventory capacity at purchase time.
The data structures preserve prices for that flow, but purchasing is not yet
implemented. No SQL changes are needed for this catalog update.

## Starter recommendations

These are ordinary leveling items, generally green, with white shoulders at
level 19. They leave room for stronger affixes, dungeon rewards, crafted gear,
PvP gear, and enchants. Level 19 kits have seven core armor slots; higher brackets
have eight, including a head item. Weapons, shields, cloaks, jewelry, and trinkets
remain to be curated separately.

| Bracket | Cloth | Leather | Mail | Plate |
| --- | --- | --- | --- | --- |
| 19 | Seer's | Bandit | Defender | Not available at this level |
| 29 | Bright + Azure Silk Hood | Superior + Emblazoned Hat | Fortified + Glimmering Mail Coif | Not available at this level |
| 39 | Conjurer's | Infiltrator | Phalanx | Not available at this level |
| 49 | Sorcerer | Tracker's | Brigade | Field Plate |
| 59 | Councillor's | Wanderer's | Merciless | Templar |
| 69 | Feralfen | Ranger | Marshcreeper | Talonguard |
| 79 | Frostwoven | Arctic | Frostscale | Cobalt |

The research CSV also includes **Iceborne** (caster leather) and **Nerubian**
(caster mail) at level 79. The default class lists favor physical roles for
Paladins, Druids, and Shamans. They do not automatically infer a specialization:
healing/caster variants need administrator edits or a later role-selection flow.
Cobalt is a defensive physical baseline, not a tailored Retribution set.

Hunter/Shaman defaults use leather below 40 and mail afterward. Warrior/Paladin
defaults use mail below 40 and plate afterward. A future auto-equip implementation
must teach the character's valid armor proficiency where necessary. The existing
talent module remains responsible for talents.

The level-69 starter families are item level 99, leaving space for stronger
Outland equipment and Northrend greens requiring levels 67-69. Level-79 starter
families are roughly item level 134-150; the tier-4 crafted PvP families are item
level 187 and require level 78. Level-59 Outland greens are reserved for tier 4,
because they are a large jump over the Classic starter items.

These are candidate progression lists, not proven upgrades for every spec and
every random stat roll. Honor prices are initial editable suggestions. Verify
the item tooltips and test time-to-upgrade against your server's honor rewards
before opening a vendor to players.

## Affixes and the restricted economy

Many green items use random-property or random-suffix pools. An item ID alone
does not specify its final stats. Before automatic grants are implemented,
choose controlled, legal affixes for the intended role; do not award unrelated
random stats or expect an unrolled template to contain its full stat budget.
The starter research CSV records the relevant pool IDs and fixed stats.

Stock templates have ordinary vendor resale and disenchant values. Merely listing
them here does not make them zero-value or prevent disenchanting. If those actions
must be prohibited, the grant/purchase implementation needs appropriate item
restrictions or separate custom templates. Do not modify the stock templates
globally and affect Normal Mode characters. Starter grants must be one-time and
persisted safely before any free equipment is distributed.

## Research and verification

Source: AzerothCore's
[item_template.sql](https://github.com/azerothcore/azerothcore-wotlk/blob/a64665927de9d23cf81b0b623ee7117131f0919a/data/sql/base/db_world/item_template.sql),
revision `a64665927de9d23cf81b0b623ee7117131f0919a`.
The downloaded SQL SHA-256 is
`bd4859be01946ca1521f7896f558f665e1e1d98890757a3df30e717b62768cc6`.
Item facts originate from AzerothCore; catalog choices and honor prices are this
module's proposed configuration.

- [Starter research CSV](starter-gear-candidates.csv): 213 pieces across 27
  armor profiles, including the two optional caster profiles. Includes fixed
  stats, item levels, required levels, affix pools, resale values, and disenchant
  IDs. Of these, 146 pieces require a controlled affix.
- [Priced catalog snapshot](gear-catalog.csv): a readable export of the shipped
  defaults, including each class and honor price. It is a research snapshot;
  runtime settings come exclusively from the `.conf`, not this CSV.

All 552 distinct items in the default class/tier lists were checked against that
snapshot for existence, level cap, armor type, class/race/faction eligibility,
and absence of profession, spell, reputation, honor-rank, map, area, and holiday
requirements. This checks eligibility, not PvP balance or live-client behavior.
The runtime validator also checks against your server's actual item templates.
