# Holy Paladin Lightforge starter set

The alternate Holy loadout covers **19, 29, 39, 49, 59, 69 and 79** with 12 custom
caster items, Devout Aurastone Hammer, Dread Pirate Ring, Discerning Eye of the
Beast and the matching faction insignia. It delivers **16 equipped pieces**,
plus the existing configurable starter bag. Retribution and Holy are separate player choices.

| Slot | Item | ID | Stock appearance |
|---|---|---:|---|
| Head | Blessed Lightforge Helm | 900074 | Lightforge Helm (16727) |
| Back | Blessed Lightforge Cloak | 900075 | Spritecaster Cape (11623) |
| Waist | Blessed Lightforge Belt | 900076 | Lightforge Belt (16723) |
| Wrists | Blessed Lightforge Bracers | 900077 | Lightforge Bracers (16722) |
| Hands | Blessed Lightforge Gauntlets | 900078 | Lightforge Gauntlets (16724) |
| Legs | Blessed Lightforge Legplates | 900079 | Lightforge Legplates (16728) |
| Feet | Blessed Lightforge Boots | 900080 | Lightforge Boots (16725) |
| Ring 2 | Blessed Lightforge Signet | 900081 | Maiden's Circle (13001) |
| Neck | Blessed Lightforge Pendant | 900082 | Star of Mystaria (12103) |
| Shoulders | Blessed Lightforge Spaulders | 900083 | Lightforge Spaulders (16729) |
| Chest | Blessed Lightforge Breastplate | 900084 | Lightforge Breastplate (16726) |
| Offhand | Blessed Lightforge Tome | 900085 | Tome of Divine Right (22319) |
| Main hand | Devout Aurastone Hammer | 42948 | Existing stock caster heirloom |

The shoulders and chest are custom caster items too. Their stats come from the
existing caster distribution used by Mystical Vest of Elements (stock **332**,
donor **48683**), with plate armor scaling masks. Source Lightforge pieces supply
appearances only. No physical-stat shoulder or Valor chest is reused as Holy gear.
Class mask **2** restricts the new items to Paladin of any applicable race.

The mace is a legal Paladin one-handed weapon. The tome is a **held offhand**,
not a shield: it has no armor or block value. Paladin relic slot stays empty.
No talent selection, respec, Dual Wield or early plate-proficiency training is
added. Below 40, caster plate uses the same native heirloom proficiency exception
as the stock Valor family. Low-level armor values still need in-game verification.

## Selectable Holy build

Holy and Retribution live together in the main config at all seven brackets.
Choose Holy after the bracket to receive this caster kit. The choice persists
without respeccing the player. See [configuration and migration](starter-gear.md).
Existing completed grants stay completed. Protection is not offered until built.

**Keep the one-handed mace before the tome in the item list.** The updated
preflight can validate a shield/held offhand after an already validated planned
one-handed weapon, even if the current weapon is two-handed. Only the core's
specific current-two-hand conflict can be deferred. Native class/race, level,
proficiency, ownership, unique-item, combat and script checks still run. Staves,
polearms, fishing poles and ordinary dual-wield offhand weapons are excluded
from this special planning path. During delivery, normal native swapping checks
the real inventory again after the new main hand has been equipped.

## Initial scaling budgets

| Pieces | Mask | Initial approximation |
|---|---:|---|
| Helm, legplates, chest | `0x800008` | Primary caster stats plus plate chest armor |
| Belt, gloves, boots, shoulders | `0x101` | Shoulder stats plus plate shoulder armor |
| Bracers | `0x40100` | Tertiary caster stats plus plate shoulder armor |
| Cloak | `0xC0000` | Tertiary caster stats plus cloak armor |
| Signet, pendant, tome | `0x40000` | Tertiary caster stats without armor |

These share an existing caster stat profile with initial approximate slot
budgets. They need actual Holy gameplay/balance checks. New items have no XP
spells, set bonuses, sockets, resale/disenchant value or repair costs. No custom
DBC records or new artwork are added; occupied custom IDs and stock items remain
untouched. Client patch work for custom-ID icons/right-click equip remains deferred.

## Install and test

1. Pull, **rebuild/install worldserver**, and stop the server to deploy the new
   executable. This update includes the planned main/offhand preflight fix.
2. Update the active config to the unified build lists, restart, then choose Holy.
   The core updater automatically applies
   `mod_battleground_mode_heirloom_holy_plate_set.sql`; expect 12 Holy templates
   and no missing/conflicting-ID rows. No manual SQL import is needed.
3. Test fresh Paladins at 19/39 and 49/79, both factions and applicable races.
   Specifically test starting with a two-handed weapon: the kit must equip the
   mace first and the tome second, preserving the original weapon in bags.
4. Verify starter bag first, all 16 pieces, paired rings/trinkets, correct
   insignia, armor/proficiency behavior below 40, actual stat changes, appearance,
   and save/relog persistence. Completed starter claims are not repeated.

The new item metadata and planning policy are tested and module compilation
against the supported core is checked; Holy set behavior and balance have not
been verified in-game. Existing testers can inspect individual IDs using GM
`.additem`, equipping the caster mace before the held tome.
