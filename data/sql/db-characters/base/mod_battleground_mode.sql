-- Apply to the CHARACTER database before enabling the module.
-- Only per-character decisions are persisted; server rules live in the config.
CREATE TABLE IF NOT EXISTS `mod_battleground_mode` (
    `guid` INT UNSIGNED NOT NULL,
    `mode` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=pending, 1=normal, 2=battleground',
    `locked_level` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `starter_state` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=pending, 1=claimed, 2=delivered',
    PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
