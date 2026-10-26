# Battleground Mode

An AzerothCore 3.3.5a C++ module for permanent battleground bracket characters.
Provides character selection, restrictions, and one-time starter gear delivery.
Gear catalogs and per-item honor prices are configurable; vendors and progression
rewards are the next milestone.

## Implemented

- New level 1 characters choose **Normal Mode** or **Battleground Mode** through a
  player-owned gossip menu, with confirmation before either permanent decision.
- Default bracket levels: **19, 29, 39, 49, 59, 69, 79**, configurable up to 80
  with at most 31 distinct choices to stay within the gossip menu limit.
- Battleground characters cannot gain XP or change level through `GiveLevel`,
  including GM level commands. The lock is restored on login and checked every
  five seconds. Changing allowed levels never rewrites existing choices.
- Selection uses the core's normal level-change flow so other modules receive
  `OnPlayerLevelChanged`. This module does not grant talent points or select talents.
- Automatically teaches available normal class-trainer spells up to the selected
  level, free of training fees, on selection and the next safe BG login. Core
  race/class, skill, rank, and prerequisite checks still apply. Quest-only spells
  are not granted. Set `BattlegroundMode.AutoLearnClassTrainerSpells = 0` to disable
  future training; this does not remove spells already learned.
- Configurable health/mana restoration and neutral-city teleport on selection:
  levels 19–69 go to Shattrath, level 79 to Dalaran, for both factions.
  Edit `BattlegroundMode.Teleport.Shattrath.*`, `BattlegroundMode.Teleport.Dalaran.*`
  and `BattlegroundMode.Teleport.Dalaran.MinLevel` (default 79) to change routing.
  Replace the old Alliance/Horde destination settings in your installed config.
- Configurable restrictions on trading, sending/collecting/returning mail,
  auctions, guild-bank items/gold, and guild-funded NPC repairs.
- Mail addressed to a restricted character is rejected even while that character
  is offline. NPC vendors and personally funded repairs remain available.
- One character-database table: `guid`, `mode`, `locked_level`, and a one-time
  starter delivery marker (`starter_state`).
  Character deletion removes its record in the core's deletion transaction.
- Editable gear lists by bracket, tier, and class, with individual honor prices.
  Five tiers are laid out, with automatic discovery of additional tier keys.
  `.bggear <tier>` previews the matching list; it does not grant or sell items.
- Tier 1 is delivered once and auto-equipped by default, replacing matching
  equipment slots and preserving old gear in bags. Existing BG characters receive
  an unclaimed set on their next safe login. `.bgstarter` retries delivery after
  inventory/equip restrictions are resolved; it cannot duplicate a completed set.

Restrictions are **per character**. Existing characters without a module record
remain Normal. New characters stay pending until they choose and gain XP normally,
even if they close the menu. Reaching level 2 automatically saves Normal Mode
permanently. Configured economy restrictions apply while pending. At level 1,
use **`.bgmode`** to reopen the menu. Once chosen, the command reports the character's
mode instead of offering conversion.

On first login, the automatic menu waits for the intro cinematic to finish or be
skipped, then waits one second before opening. It also waits while selection is
unsafe (for example during combat or travel), rather than discarding the prompt.

Only level 1 characters can enroll. Characters above level 1 remain Normal,
including newly created Death Knights and characters on servers with higher
starting levels. Old pending choices automatically become Normal if the character
is above level 1. Existing Battleground Mode characters keep their saved locks.

## Installation

Use an AzerothCore build environment with **MySQL 8 client development headers**.
Debian's `default-libmysqlclient-dev` supplies MariaDB headers, which do not provide
the MySQL SSL API used by the supported core revision. The module sources compile
there, but a complete core build needs the compatible MySQL client library.

1. Put this checkout at `azerothcore-wotlk/modules/mod-battleground-mode`.
   Keep that directory name: the generated loader expects it.
2. Leave AzerothCore's startup database updater enabled in `worldserver.conf`:

   ```ini
   Updates.EnableDatabases = 7
   ```

   No manual SQL imports are required. The core discovers the compiled module's
   `data/sql/db-characters` and `data/sql/db-world` folders recursively and applies
   their SQL to the appropriate databases before loading world data. Keep this
   checkout available under the core source directory. If the server runs from a
   different installation, set `SourceDirectory` to the AzerothCore checkout.
   See [automatic database setup](docs/automatic-database-setup.md) for Windows.

3. Reconfigure, build, and install AzerothCore with modules enabled, using your
   usual build options. For example, from the AzerothCore root:

   ```sh
   cmake -S . -B build -DMODULES=static
   cmake --build build --target worldserver --parallel 2
   cmake --install build
   ```

4. Copy the installed `etc/modules/mod_battleground_mode.conf.dist` to
   `etc/modules/mod_battleground_mode.conf`, review its settings, and start the
   server. Paths are relative to your configured installation prefix. No world
   database migration, custom NPC, or client addon is needed for selection.

Apply `.reload config` for setting changes. Relog characters when enabling the
module after it was disabled. Turning the master switch off removes enforcement;
plan that change before using it on a server with a restricted economy.

For an optional clean test reset, see
[the manual reset instructions](docs/starter-gear.md#optional-clean-test-reset).
Normal installation and builds never reset module records.

Missing/unreadable schema or corrupt saved decisions cause login to fail closed,
with a server log error. Check the startup database updater log and resolve its
configuration or database permission errors before retrying.
Choices are saved and verified before character changes; if the server stops during
selection, the saved level is enforced on the next login. Teleport happens only on
selection; a failed or interrupted teleport may require GM assistance.

## Validation

Dependency-free policy tests:

```sh
cmake -S . -B build
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Optional SQL integration checks against a **local disposable MariaDB server**:

```sh
python3 tests/database_test.py --socket /path/to/test-server.sock
```

The SQL checks create a uniquely named database and drop it afterward. They require
database-creation permission and do not use or alter your game databases.

The module's C++ sources were compiled with GCC 14 against AzerothCore commit
`9eabf314a27b71d4e9cd513f6f2c08befe90d4ef`. Compilation and SQL/policy tests do
not establish live client behavior. Before deployment, run these in-game checks:

| Check | Expected result |
| --- | --- |
| First login, skip intro with Escape or watch it to completion | Menu opens about one second afterward without relogging |
| New character, close menu, relog, `.bgmode` | Choice remains pending and menu can reopen |
| Close menu, gain XP, reach level 2, then relog and `.bgmode` | Normal XP and leveling work; Normal Mode is saved permanently |
| Choose Normal, then `.bgmode` | Normal progression; conversion unavailable |
| Choose level 19 and relog/restart | Level 19 remains locked; confirmation cannot repeat |
| Confirm BG Mode with room for replaced gear and one staging item | Configured Tier 1 gear equips; replaced items move into bags |
| Relog, change Tier 1 config, or repeat `.bgstarter` after delivery | No second kit and no replacement of later upgrades |
| Full bags, then free space and `.bgstarter` | No partial grant while full; one successful delivery after space is available |
| Set `StarterGear.AutoEquip = 0` before delivery | Set goes into bags; equipped gear stays in place |
| Kill, quest, explore, or gain battleground XP | No XP gain for Battleground Mode |
| GM level command on a locked character | Level remains unchanged |
| Normal character on the same account | Regular XP and economy operations work |
| Trade in both directions, including gold and loot-trade items | Rejected with default settings |
| Player mail to an offline locked character; take/return mail | Rejected with default settings |
| Auction bid/buyout/listing; guild bank items/gold/tab purchase | Rejected with default settings |
| Repair with personal gold, then guild gold | Personal repair works; guild repair is blocked |
| Level selection with your existing talent module | Appropriate talent handling occurs once |
| Select each bracket with class training enabled | Available trainer spells through the bracket level are learned, including prerequisite ranks; higher-level spells are absent |
| Existing BG character logs in | Missing eligible trainer spells are learned once safely loaded; known spells stay intact |
| Normal/pending character or class training disabled | No automatic class-spell grants |
| Class spells requiring a talent, quest spell, or skill | Core prerequisites remain enforced; unselected talents and quest-only spells are not granted |
| Delete a character | Module record is removed |

## Next milestone

The config contains class heirlooms and scaling completion sets at every bracket,
and priced upgrade candidates. See [Tier 1 heirloom loadouts](docs/tier1-heirlooms.md).
See [gear configuration and research](docs/starter-gear.md) for editing syntax,
the exact tier coverage, source data, and role/affix limitations. Starter gear is
free; later items use `itemID:honorPrice` pairs. Empty higher-tier lists are
placeholders for further curation or custom gear.

Starter delivery gives one Frostweave Bag (20 slots, item 41599) first, equipping
it in the first empty bag slot before the gear. Configure this using
`BattlegroundMode.StarterGear.Bag.Enable` and `.Bag.Item`. Existing bags are not
replaced; with all four bag slots occupied, delivery uses existing storage. Bag
and gear share one claim/save transaction, so retries never add another bag.
Completed grants are not retroactively given a bag. This requires the updated
module build; no SQL schema change is needed.

Starter delivery is enabled and auto-equipped by default. Set
`BattlegroundMode.StarterGear.AutoEquip = 0` for bag delivery, or
`BattlegroundMode.StarterGear.Enable = 0` to disable grants. Auto-equip reserves
bag room for replaced gear plus one reusable staging slot; bag-only delivery
requires room for the whole kit. Newly granted pieces are soulbound by default;
class-appropriate mail/plate training at level 40+ is configurable.

Next: implement an NPC that purchases from the bracket/class catalogs with
server-side honor checks. No starting currency, purchases, or honor deductions
are implemented yet.

Keep auctions blocked for a restricted economy: buying a normal player's auction
transfers gold to its owner. Custom modules with shared storage, account wallets,
or alternate transfer paths need separate integration review. Other modules must
respect the core's level hooks; direct field manipulation cannot be prevented by
this module, though level mismatches are repaired periodically. The existing talent
module's source and a live game server are still needed to verify compatibility.

## Faction PvP vendor hubs

[Nine vendors per faction are placed in rows at the supplied city locations](docs/horde-pvp-hubs.md),
with Zarg/Jackspring centered and faction banners at the ends (36 NPCs, 8 banners). The core's startup
updater installs the world SQL automatically. Build the updated module once to
support location switches: `BattlegroundMode.VendorHubs.Shattrath.Enable` and
`BattlegroundMode.VendorHubs.Dalaran.Enable` (default `1`). Changing a switch
requires a restart. Stock inventories and requirements are retained; this is
separate from the configurable bracket catalog.

## Custom scaling caster set

[Priest, Mage and Warlock now receive a full caster starter set](docs/caster-starter-set.md)
at every bracket, with ten custom scaling completion items alongside existing
stock heirlooms. The original tested hood is included. SQL installs automatically
on startup; update the active caster Tier 1 config lists too. Client patch work
for custom-item icons/right-click equipping remains deferred.

## Agility leather completion set

[Rogue/Feral Druid now use custom scaling leather completion pieces at every bracket](docs/leather-starter-set.md).
Hunter and Enhancement Shaman get them at 19/29/39, using the separate mail
completion set from 40 onward. Stock heirloom chest/shoulders/weapons remain; old ordinary filler
is replaced. SQL installs automatically on restart; update active Tier 1 lists.

## Agility mail completion set

[Hunter/Enhancement Shaman use custom scaling mail completion pieces at 49/59/69/79](docs/mail-starter-set.md),
with existing heirloom chest, shoulders and weapons retained. Their lower brackets
remain leather. Update active Tier 1 config entries and restart; SQL installs
automatically and this gear-data update needs no additional rebuild.

## Strength plate completion set

[Warrior/Retribution Paladin now use scaling Valor completion pieces at every bracket](docs/plate-starter-set.md),
with stock heirloom chest, shoulders and two-handed weapons retained. Lower
brackets use the same native heirloom proficiency exception as the stock Valor
items. Update their active Tier 1 entries and restart; item SQL installs
automatically with no additional rebuild required.

## Alternate caster/healer leather

[Druid and low-level Shaman caster/healer leather](docs/caster-leather-starter-set.md)
is available with optional ready-to-copy loadouts in `conf/loadouts`. Default
physical class lists stay unchanged. The updater installs the items on restart;
copy the chosen example entries into the active config to make that class's
future starter grants use caster gear. No additional rebuild is required.

## Alternate Elemental/Restoration Shaman mail

[Caster/healer mail and a complete seven-bracket Shaman caster path](docs/caster-mail-starter-set.md)
are available as optional example configs. The combined example uses caster
leather below 40 and caster mail afterward. Enhancement remains the default;
replace the chosen active Tier 1 entries to test the caster path. SQL installs
on restart, with no additional rebuild required.

## Alternate Holy Paladin plate

[Holy Paladin Lightforge](docs/holy-paladin-starter-set.md) includes custom caster
plate, a stock one-handed caster mace and a scaling held tome. Retribution stays
the default; replace matching entries using `conf/loadouts/holy-paladin.conf.example`
to activate Holy for future grants. This update needs a rebuild for planned
main/offhand preflight; SQL still installs automatically on restart.
