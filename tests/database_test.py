#!/usr/bin/env python3
"""Exercise the migration and decision writes on a disposable MariaDB database.

Requires a running LOCAL test server with database-creation permission. Connects
over a Unix socket only. Creates a unique database and drops only that database.
"""

import argparse
from pathlib import Path
import subprocess
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", default="mariadb")
    parser.add_argument("--socket", required=True, help="Unix socket of a local test server")
    parser.add_argument("--user", default="root")
    args = parser.parse_args()
    database = "bgmode_test_" + uuid.uuid4().hex
    client = [args.client, "--no-defaults", "--protocol=socket", "--socket=" + args.socket,
              "--user=" + args.user, "--batch", "--skip-column-names"]
    checks = 0

    def sql(statement, selected=True, check=True):
        return subprocess.run(client + ([database] if selected else []), input=statement,
                              text=True, capture_output=True, check=check)

    def expect(statement, expected, label):
        nonlocal checks
        actual = sql(statement).stdout.strip()
        if actual != expected:
            raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")
        checks += 1

    def record(guid):
        return ("SELECT b.mode, b.locked_level FROM characters c "
                "LEFT JOIN mod_battleground_mode b ON b.guid=c.guid "
                f"WHERE c.guid={guid};")

    sql(f"CREATE DATABASE `{database}`", selected=False)
    try:
        sql("CREATE TABLE characters (guid INT UNSIGNED PRIMARY KEY) ENGINE=InnoDB;"
            "INSERT INTO characters VALUES (1),(2),(3);")
        result = sql(record(1), check=False)
        if result.returncode == 0:
            raise AssertionError("missing migration must fail instead of looking like normal mode")
        checks += 1
        migration = (Path(__file__).resolve().parents[1] /
                     "data/sql/db-characters/base/mod_battleground_mode.sql").read_text()
        sql(migration)
        sql(migration)  # Safe to import again without resetting choices.
        expect(record(1), "NULL\tNULL", "existing character has no module decision")
        sql("INSERT IGNORE INTO mod_battleground_mode VALUES (1,0,0),(2,0,0);")
        expect(record(1), "0\t0", "new character is pending")
        sql("UPDATE mod_battleground_mode SET mode=2,locked_level=19 WHERE guid=1 AND mode=0;")
        expect(record(1), "2\t19", "bracket decision persists across client connections")
        sql("INSERT IGNORE INTO mod_battleground_mode VALUES (1,0,0);")
        expect(record(1), "2\t19", "repeated enrollment cannot erase a permanent decision")
        sql("UPDATE mod_battleground_mode SET mode=2,locked_level=29 WHERE guid=1 AND mode=0;")
        expect(record(1), "2\t19", "duplicate or stale confirmation cannot change bracket")
        sql("UPDATE mod_battleground_mode SET mode=1,locked_level=0 WHERE guid=1 AND mode=0;")
        expect(record(1), "2\t19", "bracket character cannot switch to normal")
        sql("UPDATE mod_battleground_mode SET mode=1,locked_level=0 WHERE guid=2 AND mode=0;")
        sql("UPDATE mod_battleground_mode SET mode=2,locked_level=39 WHERE guid=2 AND mode=0;")
        expect(record(2), "1\t0", "normal decision cannot switch to bracket mode")
        sql(migration)
        expect(record(1), "2\t19", "migration repeat preserves existing decisions")
        sql("START TRANSACTION; DELETE FROM mod_battleground_mode WHERE guid=1; ROLLBACK;")
        expect(record(1), "2\t19", "failed character deletion preserves its decision")
        sql("START TRANSACTION; DELETE FROM mod_battleground_mode WHERE guid=1;"
            "DELETE FROM characters WHERE guid=1; COMMIT;")
        expect("SELECT COUNT(*) FROM mod_battleground_mode WHERE guid=1;", "0", "character deletion cleans up its decision")
        expect(record(3), "NULL\tNULL", "unrelated normal character is unchanged")
        print(f"{checks} database checks passed")
    finally:
        sql(f"DROP DATABASE `{database}`", selected=False)


if __name__ == "__main__":
    main()
