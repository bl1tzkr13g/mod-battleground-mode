-- OPTIONAL: import into the WORLD database with worldserver STOPPED.
-- Removes only the copied spawns tagged by spawn_horde_pvp_hubs.sql.
-- Original vendors, stock templates and vendor inventories remain unchanged.
START TRANSACTION;
DELETE FROM creature WHERE Comment LIKE 'mod-battleground-mode:horde-hub:%:vendor:%';
DELETE FROM gameobject WHERE Comment LIKE 'mod-battleground-mode:horde-hub:%:banner:%';
COMMIT;
