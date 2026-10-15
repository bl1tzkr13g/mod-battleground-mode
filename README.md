# Battleground Mode

An AzerothCore 3.3.5a C++ module for permanent battleground bracket characters.
This first implementation provides character selection and restrictions. Gear
catalogs, starter sets, vendors, and progression rewards are the next milestone.

## Implemented

- New level 1 characters choose **Normal Mode** or **Battleground Mode** through a
  player-owned gossip menu, with confirmation before either permanent decision.
- Default bracket levels: **19, 29, 39, 49, 59, 69, 79**, configurable up to 80
  with at most 31 distinct choices to stay within the gossip menu limit.
- Battleground characters cannot gain XP or change level through `GiveLevel`,
  including GM level commands. The lock is restored on login and checked every
  five seconds. Changing allowed levels never rewrites existing choices.
- Selection uses the core's normal level-change flow so other modules receive
  `OnPlayerLevelChanged`. This module does not grant talents or class spells.
- Configurable health/mana restoration and faction-capital teleport on selection.
- Configurable restrictions on trading, sending/collecting/returning mail,
  auctions, guild-bank items/gold, and guild-funded NPC repairs.
- Mail addressed to a restricted character is rejected even while that character
  is offline. NPC vendors and personally funded repairs remain available.
- One character-database table, containing only `guid`, `mode`, and `locked_level`.
  Character deletion removes its record in the core's deletion transaction.

Restrictions are **per character**. Existing characters without a module record
remain Normal. New characters stay pending until they choose; XP and configured
economy restrictions apply while pending. Closing the menu does not choose a mode:
use **`.bgmode`** to reopen it. Once chosen, the command reports the character's
mode instead of offering conversion.

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
2. Import the migration into your **character database** before starting the
   updated server. Substitute your own database name and use your normal secure
   database authentication:

   ```sh
   mysql acore_characters < modules/mod-battleground-mode/data/sql/db-characters/base/mod_battleground_mode.sql
   ```

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

Missing/unreadable schema or corrupt saved decisions cause login to fail closed,
with a server log error. Import the migration and resolve the error before retrying.
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
| New character, close menu, relog, `.bgmode` | Choice remains pending and menu can reopen |
| Choose Normal, then `.bgmode` | Normal progression; conversion unavailable |
| Choose level 19 and relog/restart | Level 19 remains locked; confirmation cannot repeat |
| Kill, quest, explore, or gain battleground XP | No XP gain for Battleground Mode |
| GM level command on a locked character | Level remains unchanged |
| Normal character on the same account | Regular XP and economy operations work |
| Trade in both directions, including gold and loot-trade items | Rejected with default settings |
| Player mail to an offline locked character; take/return mail | Rejected with default settings |
| Auction bid/buyout/listing; guild bank items/gold/tab purchase | Rejected with default settings |
| Repair with personal gold, then guild gold | Personal repair works; guild repair is blocked |
| Level selection with your existing talent module | Appropriate talent handling occurs once |
| Delete a character | Module record is removed |

## Next milestone

Add configurable starter equipment and bracket/class/role gear catalogs, followed
by vendors and gold/honor progression. Tier names, prices, and strongest-tier
unlock conditions remain to be designed. No starting currency or item grants are
implemented yet.

Keep auctions blocked for a restricted economy: buying a normal player's auction
transfers gold to its owner. Custom modules with shared storage, account wallets,
or alternate transfer paths need separate integration review. Other modules must
respect the core's level hooks; direct field manipulation cannot be prevented by
this module, though level mismatches are repaired periodically. The existing talent
module's source and a live game server are still needed to verify compatibility.
