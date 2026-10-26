# Strength Valor starter completion set

Warrior and Retribution Paladin receive nine custom scaling completion pieces at
**19, 29, 39, 49, 59, 69 and 79**. All their old ordinary filler armor is removed
from those 14 lists. This is a strength/two-handed damage baseline, not a separate
Protection or Holy loadout. A [Holy caster plate alternate](holy-paladin-starter-set.md)
is now available with a separate example config. Other classes and every Tier 2+ list are unchanged.

| Slot | Item | ID | Stock appearance |
|---|---|---:|---|
| Head | Polished Helm of Valor | 900047 | Helm of Valor (16731) |
| Back | Polished Cloak of Valor | 900048 | Cape of the Black Baron (13340) |
| Waist | Polished Belt of Valor | 900049 | Belt of Valor (16736) |
| Wrists | Polished Bracers of Valor | 900050 | Bracers of Valor (16735) |
| Hands | Polished Gauntlets of Valor | 900051 | Gauntlets of Valor (16737) |
| Legs | Polished Legplates of Valor | 900052 | Legplates of Valor (16732) |
| Feet | Polished Boots of Valor | 900053 | Boots of Valor (16734) |
| Ring 2 | Polished Signet of Valor | 900054 | Painweaver Band (13098) |
| Neck | Polished Pendant of Valor | 900055 | Mark of Fordring (15411) |

Both classes keep Polished Spaulders of Valor (42949), Polished Breastplate of
Valor (48685), Dread Pirate Ring, Swift Hand of Justice and the faction-correct
insignia. Warrior retains Bloodied Arcanite Reaper; Paladin retains Reforged
Truesilver Champion. Each gets **15 equipped pieces**, plus the configurable
starter bag. The two-handed weapon leaves offhand empty; Warrior ranged and
Paladin relic slots remain empty. No shield or tank/healer role selection is
introduced. Death Knight remains outside the level-1 enrollment starter lists.

## Below level 40

The new armor uses heirloom quality 7, the same existing Valor distribution and
armor scaling masks as the stock family, with level-1 item requirements. The
module permits heirloom plate for Warrior/Paladin below 40, while the core's
`Player::CanUseItem` explicitly permits their heirloom plate without plate skill.
This is the same proficiency exception already used by their stock heirloom
chest/shoulders; normal non-heirloom plate stays restricted.

The module does **not** teach plate proficiency below 40. At 40+, its existing
`TrainArmorProficiency` setting can teach it normally. No new DBC records or core
rules are introduced by this gear-data update. Actual low-level armor values and
client presentation still need testing at 19/39 as well as at 49/79; the
proficiency exception alone is not proof of slot-balanced armor values.

## Scaling and economy

The set reuses Polished Breastplate of Valor's stock strength distribution
(**333**, copied from donor 48685). Custom class mask **3** allows Warrior and
Paladin of any applicable race. Cloaks are cloth, jewelry uses subclass 0, and
body armor is plate. No existing stock templates or occupied custom IDs are
changed.

| Pieces | Mask | Initial approximation |
|---|---:|---|
| Head, legplates | `0x800008` | Primary stats plus plate chest armor |
| Belt, gauntlets, boots | `0x101` | Shoulder stats plus plate shoulder armor |
| Bracers | `0x40100` | Tertiary stats plus plate shoulder armor |
| Cloak | `0xC0000` | Tertiary stats plus cloak armor |
| Pendant, signet | `0x40000` | Tertiary stats without armor |

These are initial approximations with existing client-supported budgets, not a
claim of final balance or exact normal slot armor weights. No XP spells,
item-set bonuses, sockets, resale/disenchant value or repair cost are added.
Stock heirloom effects remain unchanged. Client patch work for custom item IDs'
icons and manual right-click equip remains deferred. Automatic starter equip
continues to use the server's template slot information.

## Install and test

Pull the update, replace Warrior/Paladin Tier 1 entries for all seven brackets in
the active `.conf` with the updated `.conf.dist` entries, and restart worldserver.
Its normal updater applies `mod_battleground_mode_heirloom_plate_set.sql`
automatically; expect nine plate-set items and no missing/conflicting-ID rows.
No manual SQL import or additional C++ rebuild is needed with the current
starter-bag/heirloom module installed. The starter bag still equips first.

Test fresh Warrior and Paladin characters at 19, 39, 49 and 79, both factions and
applicable races. Check equip eligibility/proficiency, armor values, all models,
separate rings/trinkets, correct insignia, actual stat changes on equip/unequip,
and save/relog. The pieces have not yet been verified in-game. Existing completed
starter deliveries are not repeated; GM `.additem` can inspect individual IDs.
