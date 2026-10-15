-- Existing installations: apply to the CHARACTER database while the server is stopped.
-- Safe to repeat on MySQL 8 and MariaDB; preserves all existing character decisions.
SET @bgmode_has_starter_state = (
    SELECT COUNT(*) FROM information_schema.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'mod_battleground_mode'
      AND COLUMN_NAME = 'starter_state'
);
SET @bgmode_starter_ddl = IF(@bgmode_has_starter_state = 0,
    'ALTER TABLE `mod_battleground_mode` ADD COLUMN `starter_state` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT ''0=pending, 1=claimed, 2=delivered''',
    'SELECT 1'
);
PREPARE bgmode_starter_upgrade FROM @bgmode_starter_ddl;
EXECUTE bgmode_starter_upgrade;
DEALLOCATE PREPARE bgmode_starter_upgrade;
