-- Battleground Mode caster/healer leather completion set, applied automatically on startup.
-- Alternate loadouts: Balance/Restoration Druid at every bracket; caster/healer
-- Shaman below 40. Default physical loadouts are unchanged. Donor: 48687.
-- No custom client DBC records or XP spells. Initial budget approximations need
-- in-game tuning. Other stock items and occupied custom IDs are never overwritten.
DROP TEMPORARY TABLE IF EXISTS bgmode_caster_leather_heirlooms;
CREATE TEMPORARY TABLE bgmode_caster_leather_heirlooms (
    entry INT UNSIGNED PRIMARY KEY, name VARCHAR(100), appearance INT UNSIGNED,
    inventory TINYINT UNSIGNED, item_class TINYINT UNSIGNED,
    subclass TINYINT UNSIGNED, scaling_mask INT UNSIGNED
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_caster_leather_heirlooms VALUES
(900056,'Preened Ironfeather Cowl',16720,1,4,2,2097160),
(900057,'Preened Ironfeather Cloak',11623,16,4,1,786432),
(900058,'Preened Ironfeather Belt',16716,6,4,2,65),
(900059,'Preened Ironfeather Bracers',16714,9,4,2,786432),
(900060,'Preened Ironfeather Gloves',16717,10,4,2,65),
(900061,'Preened Ironfeather Kilt',16719,7,4,2,2097160),
(900062,'Preened Ironfeather Boots',16715,8,4,2,65),
(900063,'Preened Ironfeather Signet',13001,11,4,0,262144),
(900064,'Preened Ironfeather Pendant',12103,2,4,0,262144);

INSERT INTO item_template
    (entry,class,subclass,name,displayid,Quality,Flags,FlagsExtra,
     BuyPrice,SellPrice,InventoryType,AllowableClass,AllowableRace,
     ItemLevel,RequiredLevel,ScalingStatDistribution,ScalingStatValue,
     armor,bonding,description,MaxDurability,RequiredDisenchantSkill,DisenchantID)
SELECT h.entry,h.item_class,h.subclass,h.name,appearance.displayid,7,
       donor.Flags,0,0,0,h.inventory,1088,-1,1,1,
       donor.ScalingStatDistribution,h.scaling_mask,
       0,1,'An heirloom passed down through generations of battleground champions.',0,-1,0
FROM bgmode_caster_leather_heirlooms h
JOIN item_template donor ON donor.entry=48687
JOIN item_template appearance ON appearance.entry=h.appearance
LEFT JOIN item_template occupied ON occupied.entry=h.entry
WHERE occupied.entry IS NULL
  AND donor.ScalingStatDistribution<>0 AND donor.ScalingStatValue<>0;

SELECT COUNT(*) AS bgmode_caster_leather_items_expected_9
FROM bgmode_caster_leather_heirlooms h JOIN item_template i ON i.entry=h.entry
WHERE i.name=h.name AND i.Quality=7 AND i.AllowableClass=1088;
SELECT h.entry AS missing_or_conflicting_id,h.name AS expected_name,i.name AS actual_name
FROM bgmode_caster_leather_heirlooms h LEFT JOIN item_template i ON i.entry=h.entry
WHERE i.entry IS NULL OR i.name<>h.name OR i.Quality<>7 OR i.AllowableClass<>1088;
DROP TEMPORARY TABLE bgmode_caster_leather_heirlooms;
