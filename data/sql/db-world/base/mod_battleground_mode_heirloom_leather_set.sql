-- Battleground Mode agility leather completion set, applied automatically on startup.
-- Intended starters: Rogue/Feral Druid at every bracket; Hunter/Enhancement Shaman
-- below 40. Uses the stock Shadowcraft heirloom profile from 48689.
-- No custom client DBC records or XP spells. Initial budget approximations need
-- in-game tuning. Other stock items and occupied custom IDs are never overwritten.
DROP TEMPORARY TABLE IF EXISTS bgmode_leather_heirlooms;
CREATE TEMPORARY TABLE bgmode_leather_heirlooms (
    entry INT UNSIGNED PRIMARY KEY, name VARCHAR(100), appearance INT UNSIGNED,
    inventory TINYINT UNSIGNED, item_class TINYINT UNSIGNED,
    subclass TINYINT UNSIGNED, scaling_mask INT UNSIGNED
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_leather_heirlooms VALUES
(900029,'Stained Shadowcraft Cap',16707,1,4,2,2097160),
(900030,'Stained Shadowcraft Cloak',11626,16,4,1,786432),
(900031,'Stained Shadowcraft Belt',16713,6,4,2,65),
(900032,'Stained Shadowcraft Bracers',16710,9,4,2,786432),
(900033,'Stained Shadowcraft Gloves',16712,10,4,2,65),
(900034,'Stained Shadowcraft Pants',16709,7,4,2,2097160),
(900035,'Stained Shadowcraft Boots',16711,8,4,2,65),
(900036,'Stained Shadowcraft Signet',13098,11,4,0,262144),
(900037,'Stained Shadowcraft Pendant',15411,2,4,0,262144);

INSERT INTO item_template
    (entry,class,subclass,name,displayid,Quality,Flags,FlagsExtra,
     BuyPrice,SellPrice,InventoryType,AllowableClass,AllowableRace,
     ItemLevel,RequiredLevel,ScalingStatDistribution,ScalingStatValue,
     armor,bonding,description,MaxDurability,RequiredDisenchantSkill,DisenchantID)
SELECT h.entry,h.item_class,h.subclass,h.name,appearance.displayid,7,
       donor.Flags,0,0,0,h.inventory,1100,-1,1,1,
       donor.ScalingStatDistribution,h.scaling_mask,
       0,1,'An heirloom passed down through generations of battleground champions.',0,-1,0
FROM bgmode_leather_heirlooms h
JOIN item_template donor ON donor.entry=48689
JOIN item_template appearance ON appearance.entry=h.appearance
LEFT JOIN item_template occupied ON occupied.entry=h.entry
WHERE occupied.entry IS NULL
  AND donor.ScalingStatDistribution<>0 AND donor.ScalingStatValue<>0;

SELECT COUNT(*) AS bgmode_leather_items_expected_9
FROM bgmode_leather_heirlooms h JOIN item_template i ON i.entry=h.entry
WHERE i.name=h.name AND i.Quality=7 AND i.AllowableClass=1100;
SELECT h.entry AS missing_or_conflicting_id,h.name AS expected_name,i.name AS actual_name
FROM bgmode_leather_heirlooms h LEFT JOIN item_template i ON i.entry=h.entry
WHERE i.entry IS NULL OR i.name<>h.name OR i.Quality<>7 OR i.AllowableClass<>1100;
DROP TEMPORARY TABLE bgmode_leather_heirlooms;
