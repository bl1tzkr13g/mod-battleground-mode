# Horde PvP vendor rows

The base world SQL adds all nine supplied stock Horde vendors to **both**
Shattrath and Dalaran. Each row has Stone Guard Zarg at the supplied center, four
vendors on either side at 3-yard intervals, and decorative Horde banners at the
ends (15 yards from center). All face the supplied orientation; the row runs
perpendicular to that direction.

| City | Map | Center X | Center Y | Center Z | Facing |
|---|---:|---:|---:|---:|---:|
| Shattrath | 530 | -1760.0829 | 5321.308 | -12.428 | 2.23232 |
| Dalaran, Sunreaver's Sanctuary | 571 | 5891.34 | 635.432 | 647.682 | 3.4383 |

| Slot | Vendor | Entry |
|---:|---|---:|
| -4 | Lady Palanseer | 34043 |
| -3 | Brave Stonehide | 12793 |
| -2 | First Sergeant Hola'mahi | 12795 |
| -1 | Legionnaire Teena | 12788 |
| 0 | Stone Guard Zarg | 12794 |
| +1 | Horde Warbringer | 15350 |
| +2 | Sergeant Thunderhorn | 34038 |
| +3 | Blood Guard Zar'shi | 34063 |
| +4 | Doris Volanthius | 34060 |

## Install

1. Stop worldserver and back up the world database.
2. Import [mod_battleground_mode.sql](../data/sql/db-world/base/mod_battleground_mode.sql)
   into your **world** database (normally `acore_world`). This is separate from
   the module's character database setup. The final counts should be **18**
   vendors and **4** banners. Missing stock templates are not fabricated.
3. Start worldserver and inspect both rows in game. A C++ rebuild is needed once for the location config switches; no rebuild is
   needed for later SQL placement changes. Import the world SQL as part of database setup; a C++ build alone does not import it.

The script reuses stock entries, retaining your server's vendor inventory, honor
prices, faction, and purchase requirements. It does not turn these inventories
into the module's configurable class/bracket catalogs or change required item
levels. Level 79 characters still cannot equip stock items requiring level 80.
The copies are ordinary world NPCs, available under their normal faction rules,
including to Normal Mode Horde characters. They are not exclusive to BG mode.
Original NPCs and inventories remain in place. The generic type-5 Horde Banner
(entry **192254**) is decorative and has no capture/PvP interaction.

The script assigns automatic spawn GUIDs and records module-specific `Comment`
tags. Reimporting updates those tagged spawns and adds only missing ones; it
preserves their GUIDs. Run imports while stopped, one at a time. Do not remove or
copy the tags to unrelated spawns. To adjust the layout, edit the centers/facing
or spacing in the SQL and reimport. A straight row is 24 yards wide (30 including
banners). All spawns initially use the center's Z; side positions need an in-game
terrain/obstacle check and may need individual height adjustments. Reimporting
restores the scripted positions, so preserve any manual adjustments in the SQL.

The existing level-selection teleport settings remain independent of these NPC
positions. This file does not move player arrival points.

## Location switches

After installing the spawn SQL, add these settings to your active module config:

```ini
BattlegroundMode.VendorHubs.Shattrath.Enable = 1
BattlegroundMode.VendorHubs.Dalaran.Enable = 1
```

Set either to `0` to suppress that city's nine vendor copies and two banners.
**Restart worldserver** after changes; `.reload config` does not apply spawn
changes. Both default to enabled. The module master switch set to `0` also
suppresses both hubs at restart. The startup hook removes only tagged hub spawn
data from the core's in-memory spawn cache before grid preloading. The database
rows remain intact. Set the location back to `1` and restart to restore it;
there is no need to reimport SQL. Original vendors are unaffected. This requires
building the updated module once; later toggle changes only require a restart.

Check all four enabled/disabled combinations in game after restarting, including
both rows with banners absent when disabled, restoration after re-enabling, and
original vendors remaining available.

## Remove

Stop worldserver, import
[remove_horde_pvp_hubs.sql](../data/sql/manual/remove_horde_pvp_hubs.sql) into the
world database, then restart. It deletes only tagged copies and banners. It can
be run again safely. Import the spawn script to reinstall.

## Validation

`tests/hub_database_test.py` uses a unique disposable local MariaDB database and
AzerothCore spawn table structures. It checks counts, centers, stationary spawns,
rotations, repeated imports with stable GUIDs, updates, removal, and preservation
of original spawns/templates. Use the local test server socket, for example:

```sh
python3 tests/hub_database_test.py --socket /path/to/test-mariadb.sock
```

In-game placement and vendor interactions still require verification on the
running server; there is no live game server in this workspace.
