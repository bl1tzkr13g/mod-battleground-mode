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
        sql("CREATE TABLE characters (guid INT UNSIGNED PRIMARY KEY, playerFlags INT UNSIGNED NOT NULL DEFAULT 0) ENGINE=InnoDB;"
            "INSERT INTO characters (guid) VALUES (1),(2),(3);")
        result = sql(record(1), check=False)
        if result.returncode == 0:
            raise AssertionError("missing migration must fail instead of looking like normal mode")
        checks += 1
        migration = (Path(__file__).resolve().parents[1] /
                     "data/sql/db-characters/base/mod_battleground_mode.sql").read_text()
        sql(migration)
        sql(migration)  # Safe to import again without resetting choices.
        expect(record(1), "NULL\tNULL", "existing character has no module decision")
        sql("INSERT IGNORE INTO mod_battleground_mode (guid,mode,locked_level) VALUES (1,0,0),(2,0,0);")
        expect(record(1), "0\t0", "new character is pending")
        sql("UPDATE mod_battleground_mode SET mode=2,locked_level=19 WHERE guid=1 AND mode=0;")
        expect(record(1), "2\t19", "bracket decision persists across client connections")
        sql("INSERT IGNORE INTO mod_battleground_mode (guid,mode,locked_level) VALUES (1,0,0);")
        expect(record(1), "2\t19", "repeated enrollment cannot erase a permanent decision")
        sql("UPDATE mod_battleground_mode SET mode=2,locked_level=29 WHERE guid=1 AND mode=0;")
        expect(record(1), "2\t19", "duplicate or stale confirmation cannot change bracket")
        sql("UPDATE mod_battleground_mode SET mode=1,locked_level=0 WHERE guid=1 AND mode=0;")
        expect(record(1), "2\t19", "bracket character cannot switch to normal")
        sql("UPDATE mod_battleground_mode SET mode=1,locked_level=0 WHERE guid=2 AND mode=0;")
        sql("UPDATE mod_battleground_mode SET mode=2,locked_level=39 WHERE guid=2 AND mode=0;")
        expect(record(2), "1\t0", "normal decision cannot switch to bracket mode")
        # Simulate an existing three-column install and apply the new migration.
        sql("ALTER TABLE mod_battleground_mode DROP COLUMN starter_state;")
        update = migration  # The original SQL now handles old and new installs.
        sql(update)
        sql(update)
        expect(record(1), "2\t19", "starter migration preserves existing bracket choices")
        expect("SELECT starter_state FROM mod_battleground_mode WHERE guid=1;", "0", "existing BG characters receive an unclaimed entitlement")
        sql("UPDATE mod_battleground_mode SET starter_state=1 WHERE guid=1 AND mode=2 AND starter_state=0;")
        expect("SELECT starter_state FROM mod_battleground_mode WHERE guid=1;", "1", "starter claim is durable before inventory changes")
        expect("UPDATE mod_battleground_mode SET starter_state=1 WHERE guid=1 AND mode=2 AND starter_state=0;"
               "SELECT ROW_COUNT();", "0", "interrupted delivery cannot be claimed again")
        # Minimal inventory fixture verifies the transaction boundary; live
        # Player inventory/equipment operations still need an in-game check.
        sql("CREATE TABLE starter_delivery_items (owner INT PRIMARY KEY, entry INT) ENGINE=InnoDB;")
        sql("START TRANSACTION; INSERT INTO starter_delivery_items VALUES (1,6573);"
            "UPDATE mod_battleground_mode SET starter_state=2 WHERE guid=1 AND starter_state=1; ROLLBACK;")
        expect("SELECT COUNT(*) FROM starter_delivery_items;", "0", "failed delivery rolls back inventory fixture")
        expect("SELECT starter_state FROM mod_battleground_mode WHERE guid=1;", "1", "failed delivery retains its safety claim")
        sql("START TRANSACTION; INSERT INTO starter_delivery_items VALUES (1,6573);"
            "UPDATE mod_battleground_mode SET starter_state=2 WHERE guid=1 AND starter_state=1; COMMIT;")
        expect("SELECT entry FROM starter_delivery_items WHERE owner=1;", "6573", "successful delivery persists item fixture")
        expect("SELECT starter_state FROM mod_battleground_mode WHERE guid=1;", "2", "successful delivery commits completion with inventory")
        expect("UPDATE mod_battleground_mode SET starter_state=1 WHERE guid=1 AND mode=2 AND starter_state=0;"
               "SELECT ROW_COUNT();", "0", "completed deliveries cannot regrant on relog or request")
        sql(update)
        expect("SELECT starter_state FROM mod_battleground_mode WHERE guid=1;", "2", "reapplying migration does not reset delivered grants")
        # The bag shares the starter claim and the final inventory transaction:
        # no second entitlement/DB column, and no repeated free bag after loss.
        sql("INSERT INTO characters (guid) VALUES (6);"
            "INSERT INTO mod_battleground_mode (guid,mode,locked_level) VALUES (6,2,19);"
            "CREATE TABLE starter_bag_items (owner INT, entry INT, PRIMARY KEY(owner,entry)) ENGINE=InnoDB;")
        expect("UPDATE mod_battleground_mode SET starter_state=1 WHERE guid=6 AND mode=2 AND starter_state=0; SELECT ROW_COUNT();",
               "1", "one durable claim covers bag and gear")
        sql("START TRANSACTION; INSERT INTO starter_bag_items VALUES (6,41599),(6,900019);"
            "UPDATE mod_battleground_mode SET starter_state=2 WHERE guid=6 AND starter_state=1; ROLLBACK;")
        expect("SELECT COUNT(*) FROM starter_bag_items WHERE owner=6;", "0", "failed save rolls back bag and gear together")
        expect("SELECT starter_state FROM mod_battleground_mode WHERE guid=6;", "1", "failed bag-and-gear save retains duplication guard")
        expect("UPDATE mod_battleground_mode SET starter_state=1 WHERE guid=6 AND mode=2 AND starter_state=0; SELECT ROW_COUNT();",
               "0", "interrupted delivery cannot regrant its bag")
        sql("START TRANSACTION; INSERT INTO starter_bag_items VALUES (6,41599),(6,900019);"
            "UPDATE mod_battleground_mode SET starter_state=2 WHERE guid=6 AND starter_state=1; COMMIT;")
        expect("SELECT GROUP_CONCAT(entry ORDER BY entry) FROM starter_bag_items WHERE owner=6;", "41599,900019", "bag and gear commit in one transaction")
        expect("SELECT starter_state FROM mod_battleground_mode WHERE guid=6;", "2", "bag and gear complete one entitlement")
        sql("DELETE FROM starter_bag_items WHERE owner=6 AND entry=41599;")
        expect("UPDATE mod_battleground_mode SET starter_state=1 WHERE guid=6 AND mode=2 AND starter_state=0; SELECT ROW_COUNT();",
               "0", "losing a completed starter bag does not allow another grant")
        sql(migration)
        expect("SELECT starter_state FROM mod_battleground_mode WHERE guid=6;", "2", "SQL reapplication preserves completed bag entitlement")
        expect("UPDATE mod_battleground_mode SET starter_state=1 WHERE guid=2 AND mode=2 AND starter_state=0; SELECT ROW_COUNT();",
               "0", "Normal Mode cannot claim a starter bag")
        sql("DELETE FROM starter_bag_items WHERE owner=6; DELETE FROM mod_battleground_mode WHERE guid=6; DELETE FROM characters WHERE guid=6;")
        sql(migration)
        expect(record(1), "2\t19", "migration repeat preserves existing decisions")
        sql("START TRANSACTION; DELETE FROM mod_battleground_mode WHERE guid=1; ROLLBACK;")
        expect(record(1), "2\t19", "failed character deletion preserves its decision")
        sql("START TRANSACTION; DELETE FROM mod_battleground_mode WHERE guid=1;"
            "DELETE FROM characters WHERE guid=1; COMMIT;")
        expect("SELECT COUNT(*) FROM mod_battleground_mode WHERE guid=1;", "0", "character deletion cleans up its decision")
        expect(record(3), "NULL\tNULL", "unrelated normal character is unchanged")
        sql("INSERT INTO characters (guid,playerFlags) VALUES (4,33554440),(5,33554448);")
        sql("INSERT INTO mod_battleground_mode (guid,mode,locked_level,starter_state) VALUES (4,2,19,2);")
        reset = (Path(__file__).resolve().parents[1] /
                 "data/sql/manual/reset_battleground_mode.sql").read_text()
        sql(reset)
        expect("SELECT playerFlags FROM characters WHERE guid=4;", "8", "reset clears the module XP flag and preserves other flags")
        expect("SELECT playerFlags FROM characters WHERE guid=5;", "33554448", "reset leaves unrelated characters' XP flags untouched")
        expect("SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='mod_battleground_mode';",
               "0", "reset removes only the module table")
        expect("SELECT COUNT(*) FROM characters;", "4", "reset does not delete characters")
        sql(reset)
        expect("SELECT playerFlags FROM characters WHERE guid=5;", "33554448", "repeating reset without the module table is safe")
        sql(migration)
        expect(record(4), "NULL\tNULL", "original SQL recreates a clean table after reset")
        expect("SELECT starter_state FROM mod_battleground_mode LIMIT 1;", "", "clean install retains no stale starter claims")
        print(f"{checks} database checks passed")
    finally:
        sql(f"DROP DATABASE `{database}`", selected=False)


if __name__ == "__main__":
    main()
