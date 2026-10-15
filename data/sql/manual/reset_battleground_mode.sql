-- OPTIONAL DESTRUCTIVE RESET. Run manually against the CHARACTER database with
-- the server STOPPED. Export/back up the database first if you want to retain it.
-- Removes this module's saved choices and starter claims, and clears the XP-off
-- flag only for recorded BG characters. Does NOT remove levels, gear, spells,
-- skills, or teleport changes already applied to characters.
-- Re-import db-characters/base/mod_battleground_mode.sql before restarting with
-- the module enabled. Existing characters then use Normal Mode; new characters
-- can choose again. Never use this reset as a routine install/build step.

SET @bgmode_table_exists = (
    SELECT COUNT(*) FROM information_schema.TABLES
    WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'mod_battleground_mode'
);
SET @bgmode_clear_xp_flags = IF(@bgmode_table_exists > 0,
    'UPDATE `characters` c INNER JOIN `mod_battleground_mode` b ON b.guid = c.guid SET c.playerFlags = c.playerFlags & ~33554432 WHERE b.mode = 2',
    'SELECT 1'
);
PREPARE bgmode_reset_flags FROM @bgmode_clear_xp_flags;
EXECUTE bgmode_reset_flags;
DEALLOCATE PREPARE bgmode_reset_flags;

DROP TABLE IF EXISTS `mod_battleground_mode`;
