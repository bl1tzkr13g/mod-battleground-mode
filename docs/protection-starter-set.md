# Protection Warrior and Paladin starters

Both classes can choose **Protection** at 19/29/39/49/59/69/79/80 in the existing
bracket → build → confirmation flow. The main module config contains all 16
Protection entries. Other starter builds remain available with the same gear.

The kit keeps the existing strength/stamina Valor armor, cloak and jewelry:
custom items 900047–900055, Polished Spaulders of Valor (42949), Polished
Breastplate of Valor (48685), Dread Pirate Ring (50255), Swift Hand of Justice
(42991), and the player's faction insignia (44098 Alliance / 44097 Horde).

The main hand is **Venerable Dal'Rend's Sacred Charge (42945)**, an existing
scaling one-handed sword legal for both classes and all their applicable races.
It replaces the damage build's two-handed weapon in the configured kit. It is
listed before the shield so the existing starter planner can validate a shield
while replacing a currently equipped two-handed weapon. Native swaps equip the
sword first, then the shield, and preserve displaced equipment in bags.

| Bracket | Shield | ID | Required level | Base armor | Base block |
|---|---|---:|---:|---:|---:|
| 19 | Bear Buckler | 4821 | 18 | 461 | 8 |
| 29 | Nefarious Buckler | 4477 | 29 | 645 | 13 |
| 39 | Symbolic Crest | 14825 | 38 | 1257 | 19 |
| 49 | Valorous Shield | 8282 | 46 | 1578 | 25 |
| 59 | Imbued Shield | 15943 | 56 | 1861 | 35 |
| 69 | Shield of the Wayward Footman | 31200 | 65 | 3329 | 71 |
| 79 | Blade-Binding Bulwark | 37810 | 76 | 6011 | 162 |
| 80 | Blade-Binding Bulwark | 37810 | 76 | 6011 | 162 |

These are existing stock shields with fixed physical/defensive stats, no random
affixes, no profession/reputation prerequisite, and no faction restriction. Base
block above excludes strength, talents and rating effects. The lower brackets
use green shields; the last two use modest blue shields with defensive ratings.
These are starter baselines with room for upgrades from normal PvP vendors.
The shared Valor armor remains a strength/stamina baseline; it is not a new
specialized defense set or a promise of a raid-tanking cap.

The armor, jewelry and sword retain their existing heirloom behavior. Shields
use native stock armor/block and **do not scale**: WotLK's existing heirloom value
masks have no dedicated shield armor/block curve. Each fixed BG bracket gets
its appropriate shield instead. Stock shield sell/disenchant properties remain;
starter binding follows the existing `BindToCharacter` setting. No stock item
is modified and no client patch is needed for these sword/shield IDs.

Each kit configures 17 entries, delivering **16 equipped items** after faction
filtering, plus the existing configurable starter bag first. Warrior ranged and
Paladin relic slots remain untouched. There is no talent assignment, respec,
defense-skill boost, Dual Wield grant or shield-ability bypass. Normal class
training and legal sword proficiency training use the existing settings. Native
shield proficiency and spell/talent prerequisites still apply. Below 40, Valor
armor retains the native heirloom proficiency adaptation already used by damage
builds. Completed starter claims never regrant equipment when config changes.

## Install and verify

Pull and add the 16 `.Warrior.Protection` / `.Paladin.Protection` entries from
`conf/mod_battleground_mode.conf.dist` to the active module `.conf`, or replace
its full gear section with the new version. Apply `.reload config` or restart.
If the role-selection update (`b52f383`) is already built and installed, this
is a config/data update and needs no additional rebuild or SQL import. Otherwise
rebuild/install the latest worldserver and restart to enable saved role selection.
The previous startup updater still handles all custom Valor items and role schema.

Use fresh eligible characters and choose Protection after the bracket. Test
both classes/factions across brackets, especially 19/39 for low-level armor and
69/79 for the defensive shields. Check the bag first, sword and shield both
equipped, replaced two-handed weapon in bags, paired rings/trinkets, correct
insignia, armor/block changes, available shield abilities and save/relog. Retry
an unclaimed full-bag failure with `.bgstarter`; repeat requests after success
must not grant another bag or kit. In-game operation and balance still need testing.

Stock metadata was checked against the pinned AzerothCore item snapshot used
for the existing starter catalog. The exported [gear catalog](gear-catalog.csv)
includes the completed Protection entries.
