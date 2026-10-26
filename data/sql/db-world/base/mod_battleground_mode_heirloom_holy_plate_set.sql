-- Battleground Mode Holy Paladin caster plate set, applied automatically on startup.
-- Alternate Holy loadout at every bracket; Retribution defaults unchanged.
-- Caster distribution comes from 48683; Lightforge items supply appearances only.
-- No custom client DBC records or XP spells. Initial budget approximations need
-- in-game tuning. Other stock items and occupied custom IDs are never overwritten.
DROP TEMPORARY TABLE IF EXISTS bgmode_holy_plate_heirlooms;
CREATE TEMPORARY TABLE bgmode_holy_plate_heirlooms (
    entry INT UNSIGNED PRIMARY KEY, name VARCHAR(100), appearance INT UNSIGNED,
    inventory TINYINT UNSIGNED, item_class TINYINT UNSIGNED,
    subclass TINYINT UNSIGNED, scaling_mask INT UNSIGNED
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_holy_plate_heirlooms VALUES
(900074,'Blessed Lightforge Helm',16727,1,4,4,8388616),
(900075,'Blessed Lightforge Cloak',11623,16,4,1,786432),
(900076,'Blessed Lightforge Belt',16723,6,4,4,257),
(900077,'Blessed Lightforge Bracers',16722,9,4,4,262400),
(900078,'Blessed Lightforge Gauntlets',16724,10,4,4,257),
(900079,'Blessed Lightforge Legplates',16728,7,4,4,8388616),
(900080,'Blessed Lightforge Boots',16725,8,4,4,257),
(900081,'Blessed Lightforge Signet',13001,11,4,0,262144),
(900082,'Blessed Lightforge Pendant',12103,2,4,0,262144),
(900083,'Blessed Lightforge Spaulders',16729,3,4,4,257),
(900084,'Blessed Lightforge Breastplate',16726,5,4,4,8388616),
(900085,'Blessed Lightforge Tome',22319,23,4,0,262144);

INSERT INTO item_template
    (entry,class,subclass,name,displayid,Quality,Flags,FlagsExtra,
     BuyPrice,SellPrice,InventoryType,AllowableClass,AllowableRace,
     ItemLevel,RequiredLevel,ScalingStatDistribution,ScalingStatValue,
     armor,bonding,description,MaxDurability,RequiredDisenchantSkill,DisenchantID)
SELECT h.entry,h.item_class,h.subclass,h.name,appearance.displayid,7,
       donor.Flags,0,0,0,h.inventory,2,-1,1,1,
       donor.ScalingStatDistribution,h.scaling_mask,
       0,1,'An heirloom passed down through generations of battleground champions.',0,-1,0
FROM bgmode_holy_plate_heirlooms h
JOIN item_template donor ON donor.entry=48683
JOIN item_template appearance ON appearance.entry=h.appearance
LEFT JOIN item_template occupied ON occupied.entry=h.entry
WHERE occupied.entry IS NULL
  AND donor.ScalingStatDistribution<>0 AND donor.ScalingStatValue<>0;

SELECT COUNT(*) AS bgmode_holy_plate_items_expected_12
FROM bgmode_holy_plate_heirlooms h JOIN item_template i ON i.entry=h.entry
WHERE i.name=h.name AND i.Quality=7 AND i.AllowableClass=2;
SELECT h.entry AS missing_or_conflicting_id,h.name AS expected_name,i.name AS actual_name
FROM bgmode_holy_plate_heirlooms h LEFT JOIN item_template i ON i.entry=h.entry
WHERE i.entry IS NULL OR i.name<>h.name OR i.Quality<>7 OR i.AllowableClass<>2;
DROP TEMPORARY TABLE bgmode_holy_plate_heirlooms;
