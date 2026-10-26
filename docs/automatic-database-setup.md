# Automatic database setup

Update the module, rebuild/install worldserver, and start it. **No manual SQL
imports are required for installation or upgrades.** AzerothCore's database
updater runs before world data and module startup hooks. It reads the compiled
module list and recursively discovers this module's database folders:

- `data/sql/db-characters`: character choices and starter delivery schema.
- `data/sql/db-world`: vendor rows, banners and custom starter item templates.

The core tracks applied filenames and content hashes in each database's `updates`
table. Changed module SQL is reapplied with the default redundancy settings.
Both setup files are repeatable: existing choices, delivery markers, stock NPCs,
and hub GUIDs are preserved. Destructive uninstall/reset SQL remains under
`data/sql/manual`, outside these discovered folders, and is never auto-applied.

## Server settings

In the active **worldserver.conf**, keep the normal core updater enabled:

```ini
Updates.EnableDatabases = 7
Updates.Redundancy = 1
Updates.AllowRehash = 1
```

The database mask includes character (2) and world (4) updates. These are core
settings loaded before module scripts run; configure them in worldserver.conf.
The module does not override the server administrator's updater settings.

The module checkout must be present at
`<SourceDirectory>/modules/mod-battleground-mode` when starting the server. For
this AMP installation, set the following if the compiled default source path is
different or the executable is launched from another directory:

```ini
SourceDirectory = "D:/AMPDatastore/Instances/azerothcore01/azerothcore-wotlk"
```

The core updater also requires the MySQL command-line client. If it cannot find
it automatically, set `MySQLExecutable` in worldserver.conf to the existing
`mysql.exe` path for your installation. Database credentials come from the core's
normal database settings; the account needs permission to apply the schema and
spawn changes. Startup logs report any missing path, client, or permissions.

On the next start, the core applies module SQL to the correct databases and then
loads world data. Keep the module source files updated alongside the installed
binary. A C++ build does not itself connect to or modify the game databases.

## Existing installations

Prior manual imports are supported because both scripts are safe to apply again.
The startup updater records its own hashes when it first discovers the files.
Use the per-location config switches to hide/restore vendor hubs; this preserves
the DB rows and does not depend on reapplying unchanged SQL.

This workflow was checked against AzerothCore revision
`9eabf314a27b71d4e9cd513f6f2c08befe90d4ef`: worldserver supplies `AC_MODULES_LIST`
to DatabaseLoader, and UpdateFetcher discovers and recursively scans the module's
matching `db-characters`/`db-world` directories. The SQL is tested separately on a
disposable local MariaDB database; a full game-server startup is not available in
this workspace.
