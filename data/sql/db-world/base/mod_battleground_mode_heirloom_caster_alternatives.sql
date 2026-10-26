-- One-handed caster alternatives. Applied automatically by the startup updater.
-- Uses existing DBC distributions, no new client records or stock modifications.
-- Occupied custom IDs are preserved and reported, never overwritten.
DROP TEMPORARY TABLE IF EXISTS bgmode_caster_alternatives;
CREATE TEMPORARY TABLE bgmode_caster_alternatives (
    entry INT UNSIGNED PRIMARY KEY, name VARCHAR(100), appearance INT UNSIGNED,
    donor INT UNSIGNED, item_class TINYINT UNSIGNED, subclass TINYINT UNSIGNED,
    inventory TINYINT UNSIGNED, scaling_mask INT UNSIGNED, weapon_delay INT UNSIGNED
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_caster_alternatives VALUES
(900086,'Dignified Scholar\'s Kris',13964,44094,2,15,21,34820,1600),
(900087,'Dignified Scholar\'s Tome',22319,48691,4,0,23,262144,0);

INSERT INTO item_template
    (entry,class,subclass,name,displayid,Quality,Flags,FlagsExtra,
     BuyPrice,SellPrice,InventoryType,AllowableClass,AllowableRace,
     ItemLevel,RequiredLevel,ScalingStatDistribution,ScalingStatValue,
     dmg_min1,dmg_max1,dmg_type1,delay,armor,block,bonding,description,
     MaxDurability,RequiredDisenchantSkill,DisenchantID)
SELECT h.entry,h.item_class,h.subclass,h.name,appearance.displayid,7,
       donor.Flags,0,0,0,h.inventory,1424,-1,1,1,
       donor.ScalingStatDistribution,h.scaling_mask,
       IF(h.item_class=2,1,0),0,0,h.weapon_delay,0,0,1,
       'An heirloom passed down through generations of battleground scholars.',0,-1,0
FROM bgmode_caster_alternatives h
JOIN item_template donor ON donor.entry=h.donor
JOIN item_template appearance ON appearance.entry=h.appearance
LEFT JOIN item_template occupied ON occupied.entry=h.entry
WHERE occupied.entry IS NULL
  AND donor.ScalingStatDistribution<>0 AND donor.ScalingStatValue<>0;

SELECT COUNT(*) AS bgmode_caster_alternatives_expected_2
FROM bgmode_caster_alternatives h JOIN item_template i ON i.entry=h.entry
WHERE i.name=h.name AND i.Quality=7 AND i.AllowableClass=1424;
SELECT h.entry AS missing_or_conflicting_id,h.name AS expected_name,i.name AS actual_name
FROM bgmode_caster_alternatives h LEFT JOIN item_template i ON i.entry=h.entry
WHERE i.entry IS NULL OR i.name<>h.name OR i.Quality<>7 OR i.AllowableClass<>1424;
DROP TEMPORARY TABLE bgmode_caster_alternatives;
