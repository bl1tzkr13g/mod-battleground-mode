-- Battleground Mode caster Tier 1 completion set, automatically applied on startup.
-- Uses existing appearances and existing DBC profile 336 from heirloom robe 48691.
-- Initial slot budgets are approximations using stock scaling columns; client patch
-- for icons/right-click equip remains deferred. No new client DBC records are used.
-- Existing stock items and occupied custom IDs are never overwritten.
-- 900019 preserves the previously tested helmet. Other custom IDs are 900020..900028.
DROP TEMPORARY TABLE IF EXISTS bgmode_cloth_heirlooms;
CREATE TEMPORARY TABLE bgmode_cloth_heirlooms (
    entry INT UNSIGNED PRIMARY KEY, name VARCHAR(100), appearance INT UNSIGNED,
    inventory TINYINT UNSIGNED, item_class TINYINT UNSIGNED,
    subclass TINYINT UNSIGNED, scaling_mask INT UNSIGNED
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_cloth_heirlooms VALUES
(900019,'Tattered Dreadmist Hood',16698,1,4,1,1048584),
(900020,'Tattered Dreadmist Cloak',13122,16,4,1,786432),
(900021,'Tattered Dreadmist Belt',16702,6,4,1,33),
(900022,'Tattered Dreadmist Bracers',16703,9,4,1,786432),
(900023,'Tattered Dreadmist Wraps',16705,10,4,1,33),
(900024,'Tattered Dreadmist Leggings',16699,7,4,1,1048584),
(900025,'Tattered Dreadmist Sandals',16704,8,4,1,33),
(900026,'Tattered Dreadmist Wand',7001,26,2,19,278528),
(900027,'Tattered Dreadmist Signet',13001,11,4,0,262144),
(900028,'Tattered Dreadmist Pendant',12103,2,4,0,262144);

INSERT INTO item_template
    (entry,class,subclass,name,displayid,Quality,Flags,FlagsExtra,
     BuyPrice,SellPrice,InventoryType,AllowableClass,AllowableRace,
     ItemLevel,RequiredLevel,ScalingStatDistribution,ScalingStatValue,
     armor,bonding,description,MaxDurability,RequiredDisenchantSkill,DisenchantID,
     delay,dmg_min1,dmg_max1,dmg_type1)
SELECT h.entry,h.item_class,h.subclass,h.name,appearance.displayid,7,
       donor.Flags,0,0,0,h.inventory,400,-1,1,1,
       donor.ScalingStatDistribution,h.scaling_mask,
       0,1,'An heirloom passed down through generations of battleground champions.',0,-1,0,
       IF(h.item_class=2,appearance.delay,1000),
       IF(h.item_class=2,1,0),IF(h.item_class=2,1,0),
       IF(h.item_class=2,appearance.dmg_type1,0)
FROM bgmode_cloth_heirlooms h
JOIN item_template donor ON donor.entry=48691
JOIN item_template appearance ON appearance.entry=h.appearance
LEFT JOIN item_template occupied ON occupied.entry=h.entry
WHERE occupied.entry IS NULL
  AND donor.ScalingStatDistribution<>0 AND donor.ScalingStatValue<>0;

-- Expect 10 known caster items. Zero rows in the second result means no missing
-- source item or occupied-ID/name conflict. Never overwrite another module's item.
SELECT COUNT(*) AS bgmode_cloth_items_expected_10
FROM bgmode_cloth_heirlooms h JOIN item_template i ON i.entry=h.entry
WHERE i.name=h.name AND i.Quality=7 AND i.AllowableClass=400;
SELECT h.entry AS missing_or_conflicting_id,h.name AS expected_name,i.name AS actual_name
FROM bgmode_cloth_heirlooms h LEFT JOIN item_template i ON i.entry=h.entry
WHERE i.entry IS NULL OR i.name<>h.name OR i.Quality<>7 OR i.AllowableClass<>400;
DROP TEMPORARY TABLE bgmode_cloth_heirlooms;
