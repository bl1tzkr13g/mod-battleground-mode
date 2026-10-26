-- Battleground Mode caster/healer mail completion set, applied automatically on startup.
-- Alternate loadout: Elemental/Restoration Shaman in level-40+ brackets.
-- Default Enhancement loadouts are unchanged. Donor: Mystical Vest 48683.
-- No custom client DBC records or XP spells. Initial budget approximations need
-- in-game tuning. Other stock items and occupied custom IDs are never overwritten.
DROP TEMPORARY TABLE IF EXISTS bgmode_caster_mail_heirlooms;
CREATE TEMPORARY TABLE bgmode_caster_mail_heirlooms (
    entry INT UNSIGNED PRIMARY KEY, name VARCHAR(100), appearance INT UNSIGNED,
    inventory TINYINT UNSIGNED, item_class TINYINT UNSIGNED,
    subclass TINYINT UNSIGNED, scaling_mask INT UNSIGNED
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_caster_mail_heirlooms VALUES
(900065,'Mystical Coif of Elements',16667,1,4,3,4194312),
(900066,'Mystical Cloak of Elements',11623,16,4,1,786432),
(900067,'Mystical Cord of Elements',16673,6,4,3,129),
(900068,'Mystical Bindings of Elements',16671,9,4,3,262272),
(900069,'Mystical Gauntlets of Elements',16672,10,4,3,129),
(900070,'Mystical Kilt of Elements',16668,7,4,3,4194312),
(900071,'Mystical Boots of Elements',16670,8,4,3,129),
(900072,'Mystical Signet of Elements',13001,11,4,0,262144),
(900073,'Mystical Pendant of Elements',12103,2,4,0,262144);

INSERT INTO item_template
    (entry,class,subclass,name,displayid,Quality,Flags,FlagsExtra,
     BuyPrice,SellPrice,InventoryType,AllowableClass,AllowableRace,
     ItemLevel,RequiredLevel,ScalingStatDistribution,ScalingStatValue,
     armor,bonding,description,MaxDurability,RequiredDisenchantSkill,DisenchantID)
SELECT h.entry,h.item_class,h.subclass,h.name,appearance.displayid,7,
       donor.Flags,0,0,0,h.inventory,64,-1,1,1,
       donor.ScalingStatDistribution,h.scaling_mask,
       0,1,'An heirloom passed down through generations of battleground champions.',0,-1,0
FROM bgmode_caster_mail_heirlooms h
JOIN item_template donor ON donor.entry=48683
JOIN item_template appearance ON appearance.entry=h.appearance
LEFT JOIN item_template occupied ON occupied.entry=h.entry
WHERE occupied.entry IS NULL
  AND donor.ScalingStatDistribution<>0 AND donor.ScalingStatValue<>0;

SELECT COUNT(*) AS bgmode_caster_mail_items_expected_9
FROM bgmode_caster_mail_heirlooms h JOIN item_template i ON i.entry=h.entry
WHERE i.name=h.name AND i.Quality=7 AND i.AllowableClass=64;
SELECT h.entry AS missing_or_conflicting_id,h.name AS expected_name,i.name AS actual_name
FROM bgmode_caster_mail_heirlooms h LEFT JOIN item_template i ON i.entry=h.entry
WHERE i.entry IS NULL OR i.name<>h.name OR i.Quality<>7 OR i.AllowableClass<>64;
DROP TEMPORARY TABLE bgmode_caster_mail_heirlooms;
