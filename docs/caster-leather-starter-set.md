# Caster/healer leather completion set

Nine scaling pieces provide an alternate caster/healer kit for **Balance or
Restoration Druid** at every bracket, plus **Elemental/Restoration Shaman** below
40. These roles are selectable alongside physical builds in the main config.

| Slot | Item | ID | Stock appearance |
|---|---|---:|---|
| Head | Preened Ironfeather Cowl | 900056 | Wildheart Cowl (16720) |
| Back | Preened Ironfeather Cloak | 900057 | Spritecaster Cape (11623) |
| Waist | Preened Ironfeather Belt | 900058 | Wildheart Belt (16716) |
| Wrists | Preened Ironfeather Bracers | 900059 | Wildheart Bracers (16714) |
| Hands | Preened Ironfeather Gloves | 900060 | Wildheart Gloves (16717) |
| Legs | Preened Ironfeather Kilt | 900061 | Wildheart Kilt (16719) |
| Feet | Preened Ironfeather Boots | 900062 | Wildheart Boots (16715) |
| Ring 2 | Preened Ironfeather Signet | 900063 | Maiden's Circle (13001) |
| Neck | Preened Ironfeather Pendant | 900064 | Star of Mystaria (12103) |

The set uses **Preened Ironfeather Breastplate (48687)** as the scaling donor:
stock caster distribution **334**. Class mask **1088** permits Druid/Shaman of any
applicable race, while excluding physical-only Rogue/Hunter loadouts. Cloaks use
cloth subclass 1, jewelry subclass 0, and body armor leather subclass 2. All new
pieces require level 1 and use existing heirloom scaling through the normal
client/core tables. No custom DBC records or artwork are added.

## Selectable builds

The main module config includes Balance/Restoration Druid at every bracket and
Elemental/Restoration Shaman at 19/29/39. Druid keeps caster shoulders/chest
42984/48687; Shaman uses 42951/48683 with native low-level mail adaptation.

These builds use Dignified Headmaster's Charge (42947), Discerning Eye of the
Beast (42992), Dread Pirate Ring (50255) and the correct faction insignia. The
staff uses both hands in the equipped kit. [Bag alternatives](caster-weapon-alternatives.md)
provide a dagger/tome for caster Druids and a mace/shield for caster Shamans.
Relic slots remain empty. Each delivers **15 equipped pieces**, plus the existing
configurable starter bag. Physical weapons and Swift Hand of Justice are removed
from the alternate entries. No talents, forms or extra weapon skill ranks are
granted by this item-data update.

Choose the role after choosing a bracket. All builds are in the single active
module config; see [migration and selection](starter-gear.md). No separate
example files or default-class replacement is needed.

## Budgets and testing

| Pieces | Mask | Initial approximation |
|---|---:|---|
| Cowl, kilt | `0x200008` | Primary caster stats plus leather chest armor |
| Belt, gloves, boots | `0x41` | Shoulder stats plus leather shoulder armor |
| Cloak, bracers | `0xC0000` | Tertiary stats plus cloak armor |
| Signet, pendant | `0x40000` | Tertiary caster stats without armor |

These are initial shared caster/healer budgets, including a cloak-armor
approximation for bracers, rather than final role-specific balance. New pieces
have no XP spells, item-set bonuses, sockets, resale/disenchant value or repair
cost. Stock heirlooms keep their normal effects. Existing source templates and
occupied custom IDs are never overwritten.

Pull and restart with the core updater enabled to install
`mod_battleground_mode_heirloom_caster_leather_set.sql` automatically. Expect nine
caster-leather templates and no missing/conflicting-ID rows. No manual SQL import
is needed. The role-selection update requires a rebuilt/installed worldserver.

Test fresh Druid casters at 19/79 and Shaman casters at 19/39 after choosing the
matching build in the new menu. Check both factions/races, model
visibility, actual stat changes on equip/unequip, staff proficiency, separate
rings/trinkets, starter bag first, and save/relog. Existing testers can inspect
individual pieces using GM `.additem` with the IDs above. Client patch work for
custom-ID icons/right-click equip remains deferred. In-game scaling and balance
of this new set remain unverified.
