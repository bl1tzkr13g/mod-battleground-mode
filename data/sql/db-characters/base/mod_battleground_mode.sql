-- Apply to the CHARACTER database before enabling the module.
-- Only per-character decisions are persisted; server rules live in the config.
CREATE TABLE IF NOT EXISTS `mod_battleground_mode` (
    `guid` INT UNSIGNED NOT NULL,
    `mode` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=pending, 1=normal, 2=battleground',
    `locked_level` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `starter_state` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=pending, 1=claimed, 2=delivered',
    PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Also upgrades an earlier install when this same original file is re-imported.
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
