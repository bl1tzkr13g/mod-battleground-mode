# Starter gear and configurable honor catalogs

The config is the editable source for gear and prices. No item lists or honor
prices are stored in the module's character database.

## Editing a character's gear list

Edit the live `mod_battleground_mode.conf` in your server's installed module
configuration directory. The repository distributes the defaults in
[`conf/mod_battleground_mode.conf.dist`](../conf/mod_battleground_mode.conf.dist).
Preserve your existing settings when copying the new gear sections into it.

```ini
# LEVEL 19 - TIER 1 STARTER GEAR
# Paladin: ordinary filler armor plus heirloom armor, sword, ring, and trinkets
BattlegroundMode.Gear.Level19.Tier1.Paladin = "6573:0,6574:0,6576:0,6577:0,6578:0,42949:0,48685:0,44092:0,42991:0,50255:0,44098:0,44097:0"

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
heirlooms plus ordinary filler armor, tiers 2 and 3 have upgrade candidates, and tier 4 has additional
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
also be enforced by the future purchase flow if you add restricted items. Starter
auto-equip uses the core's equipment checks, including item-use restrictions.

A Battleground Mode character can use **`.bggear 2`** (or another tier number) to
preview only the exact saved bracket/class list and its prices. Other brackets,
other classes, and Normal Mode characters do not receive that list. This command
is a preview: it does not grant, sell, equip, or deduct honor for anything.

The future vendor must use tiers 2+ and recheck the same
bracket/class, live price, honor balance, and inventory capacity at purchase time.
The data structures preserve prices for that flow, but purchasing is not yet
implemented.

## One-time starter delivery and auto-equip

```ini
BattlegroundMode.StarterGear.Enable = 1
BattlegroundMode.StarterGear.AutoEquip = 1
BattlegroundMode.StarterGear.TrainArmorProficiency = 1
BattlegroundMode.StarterGear.BindToCharacter = 1
```

On confirming Battleground Mode, the character receives its configured Tier 1
list once. With auto-equip enabled, every piece is equipped using the native
inventory swap flow. Existing gear in the same slots moves into bags; slots
absent from the configured kit remain unchanged. Old items are not deleted.
With `AutoEquip = 0`, the entire set goes into bags without changing equipment.
Changing the setting afterward does not reissue or auto-equip an already
delivered set. Characters can equip their delivered bag items manually.

Existing Battleground Mode characters receive an unclaimed starter set on their
next safe login, including replacement of their currently equipped matching
slots. Grants wait until the player is loaded, alive, out of combat, and outside
travel, trades, battlegrounds, and queues. Normal characters never get this grant.
Disabling starter delivery leaves the entitlement unclaimed for later use.

The entire set must fit in inventory first, even when it will then be equipped.
Auto-equip preflights every item and rejects kits with conflicting destination
slots. Auto-equip reserves bag room for displaced gear and one reusable working
slot, checking incoming item restrictions before claiming. Each item is stored
and equipped immediately so empty equipment slots release staging capacity.
Starting food/hearthstone do not need to be removed just to hold a complete kit.
Bag-only delivery still reserves room for the entire kit. If preflight checks
fail, nothing is delivered and the claim stays available. Use **`.bgstarter`**
after resolving the restriction. Success never grants a second set or overwrites
later upgrades. An unexpected native swap failure after claiming stops delivery
and requires GM reconciliation; the claim prevents automatic duplicate grants.

### Starter bag

```ini
BattlegroundMode.StarterGear.Bag.Enable = 1
BattlegroundMode.StarterGear.Bag.Item = 41599
```

A Frostweave Bag (20 slots) is included by default and equipped **before** the
gear, in the first empty equipped bag slot. Existing bags are never replaced.
If all four slots are occupied, no bag is added and the kit uses existing space.
Only ordinary generic bags with 1..36 slots are accepted; specialized bags are
rejected. The bag is bound when `BindToCharacter` is enabled. It equips even with
`AutoEquip=0`, since that setting controls gear delivery rather than the bag.

The bag is prepared and checked before the existing durable starter claim. Only
after that claim is verified does it enter inventory. Gear storage preflight can
account for the planned generic bag, allowing delivery from a full backpack when
the new bag alone holds the required reservation. Ownership and unique-item
restrictions remain checked. Bag and all kit items are saved in the same final
inventory transaction with the completion marker. No extra state column or
entitlement is introduced. Losing/selling the bag after a completed grant does
not allow another. Unexpected post-claim failures still require GM review.

Pull and rebuild/install the updated module, then restart. The default bag works
without adding config keys; copy them into the active config to customize or
disable it. No manual SQL import or new schema migration is needed. Already
completed starter grants stay completed; use a fresh character for bag testing.
Verify first-empty-slot behavior, existing bag preservation, a full backpack
with an empty bag slot, both gear delivery modes, repeat requests/relog, and
survival of bag contents after save/relog. In-game bag delivery remains to test.

The optional training teaches only legal plate proficiency for level-40+
Warriors/Paladins and mail for level-40+ Hunters/Shamans. It does not grant weapon
skill ranks or talents. Configurable starter weapon proficiency training now
teaches class-legal skills needed to equip the supplied weapons; class-trainer
combat spells are managed separately by AutoLearnClassTrainerSpells. New starter pieces are soulbound when
`BindToCharacter` is enabled. Honor is not charged for Tier 1.

The core startup database updater automatically applies
[`mod_battleground_mode.sql`](../data/sql/db-characters/base/mod_battleground_mode.sql)
to the character database, creating fresh schemas and upgrading earlier ones
without resetting choices or completed deliveries. No manual import is required;
see [automatic database setup](automatic-database-setup.md).

The single `starter_state` column stores pending (0), claimed (1), or delivered
(2). A durable claim precedes inventory changes, then item instances, positions,
and completion are queued together on the core's asynchronous database worker
before the city teleport. Delivery is confirmed after that transaction succeeds.
Player-save statements require this worker connection; a synchronous direct
commit causes a missing prepared-statement assertion. A server crash
or exceptional failure after claiming requires a GM to reconcile inventory;
state 1 is deliberately not retried automatically. Do not blindly reset that
state, since some items may already have been received. This prevents repeated
free grants when a delivery's outcome is uncertain.

## Optional clean test reset

To clear test setup, stop the server and back up the character database if you
want to retain its current state. Manually import
[`reset_battleground_mode.sql`](../data/sql/manual/reset_battleground_mode.sql)
into the character database, then import the original
[`mod_battleground_mode.sql`](../data/sql/db-characters/base/mod_battleground_mode.sql)
before restarting with the module enabled.

The reset clears the core XP-disabled flag only for recorded Battleground Mode
characters, preserving their other flags. It drops this module's own table,
including saved choices and starter claims. It does not delete characters or
alter unrelated characters' XP settings.

Already-applied levels, equipment, armor proficiency, and positions remain; the
module has no record of their prior values. Fully reversing those changes needs
a suitable backup or deliberate manual character edits. Existing characters
without module records become Normal Mode; create a new level-1 character to
retest first-login selection. The reset is optional and destructive, never a
normal install/build/startup step.

## Starter recommendations

Tier 1 now uses the same class-appropriate heirloom shoulders, chest, weapon(s),
ring, and trinkets at every bracket. The old shoulders and chest are removed;
ordinary pieces for other slots remain for Warrior/Paladin. Hunter/Shaman
use the [agility mail completion set](mail-starter-set.md) from level 40 onward. Rogue/Feral Druid use the [leather completion set](leather-starter-set.md)
at every bracket; Hunter/Enhancement Shaman get it below 40. Priest, Mage and
Warlock instead use the [full custom caster completion set](caster-starter-set.md).
See [the heirloom loadouts](tier1-heirlooms.md).
The following families describe those retained filler pieces and the original
research, rather than complete current sets. Casters have scaling cloaks and
wands; shields and ammunition remain separate. Warrior/Paladin level-19 kits have no head item.

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

Retained Hunter/Shaman filler armor uses leather below 40 and mail afterward.
Warrior/Paladin filler uses mail below 40 and plate afterward. Heirloom armor
uses the core's built-in low-level adaptation. Starter auto-equip can teach the
character's valid mail/plate armor proficiency where necessary. The existing
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
does not specify its final stats. Starter delivery uses the core's normal random
property/suffix generation and refuses to deliver a random-stat item if its
affix could not be generated. These are legal ordinary rolls, not controlled
role-specific stats; deterministic role-specific affixes remain future work.
The starter research CSV records the relevant pool IDs and fixed stats.

Stock templates have ordinary vendor resale and disenchant values. Merely listing
them here does not make them zero-value or prevent disenchanting. If those actions
must be prohibited, the grant/purchase implementation needs appropriate item
restrictions or separate custom templates. Do not modify the stock templates
globally and affect Normal Mode characters. Starter delivery is tracked once per
character using the persistent delivery marker described above.

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

## Item tooltips

Starter delivery sends the core's localized item-template response before the
new-item notification and city teleport. BG login also refreshes information
for equipped items. If an existing character shows “Retrieving item information,”
log in again after updating, or use `.bgstarter` to refresh equipped-item
information. A completed delivery remains completed; this does not grant gear.
