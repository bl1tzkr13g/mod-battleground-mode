-- OPTIONAL UNINSTALL: WORLD database, worldserver STOPPED.
-- Removes only module-tagged faction hub copies, leaving all originals intact.
START TRANSACTION;
DELETE FROM creature WHERE Comment LIKE 'mod-battleground-mode:horde-hub:%:vendor:%'
    OR Comment LIKE 'mod-battleground-mode:alliance-hub:%:vendor:%';
DELETE FROM gameobject WHERE Comment LIKE 'mod-battleground-mode:horde-hub:%:banner:%'
    OR Comment LIKE 'mod-battleground-mode:alliance-hub:%:banner:%';
COMMIT;
