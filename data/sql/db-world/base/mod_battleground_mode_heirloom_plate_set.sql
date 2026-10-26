-- Battleground Mode strength plate completion set, applied automatically on startup.
-- Intended starters: two-handed Warrior and Retribution Paladin at every bracket.
-- Uses the stock Valor heirloom profile from 48685 and native low-level adaptation.
-- No custom client DBC records or XP spells. Initial budget approximations need
-- in-game tuning. Other stock items and occupied custom IDs are never overwritten.
DROP TEMPORARY TABLE IF EXISTS bgmode_plate_heirlooms;
CREATE TEMPORARY TABLE bgmode_plate_heirlooms (
    entry INT UNSIGNED PRIMARY KEY, name VARCHAR(100), appearance INT UNSIGNED,
    inventory TINYINT UNSIGNED, item_class TINYINT UNSIGNED,
    subclass TINYINT UNSIGNED, scaling_mask INT UNSIGNED
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_plate_heirlooms VALUES
(900047,'Polished Helm of Valor',16731,1,4,4,8388616),
(900048,'Polished Cloak of Valor',13340,16,4,1,786432),
(900049,'Polished Belt of Valor',16736,6,4,4,257),
(900050,'Polished Bracers of Valor',16735,9,4,4,262400),
(900051,'Polished Gauntlets of Valor',16737,10,4,4,257),
(900052,'Polished Legplates of Valor',16732,7,4,4,8388616),
(900053,'Polished Boots of Valor',16734,8,4,4,257),
(900054,'Polished Signet of Valor',13098,11,4,0,262144),
(900055,'Polished Pendant of Valor',15411,2,4,0,262144);

INSERT INTO item_template
    (entry,class,subclass,name,displayid,Quality,Flags,FlagsExtra,
     BuyPrice,SellPrice,InventoryType,AllowableClass,AllowableRace,
     ItemLevel,RequiredLevel,ScalingStatDistribution,ScalingStatValue,
     armor,bonding,description,MaxDurability,RequiredDisenchantSkill,DisenchantID)
SELECT h.entry,h.item_class,h.subclass,h.name,appearance.displayid,7,
       donor.Flags,0,0,0,h.inventory,3,-1,1,1,
       donor.ScalingStatDistribution,h.scaling_mask,
       0,1,'An heirloom passed down through generations of battleground champions.',0,-1,0
FROM bgmode_plate_heirlooms h
JOIN item_template donor ON donor.entry=48685
JOIN item_template appearance ON appearance.entry=h.appearance
LEFT JOIN item_template occupied ON occupied.entry=h.entry
WHERE occupied.entry IS NULL
  AND donor.ScalingStatDistribution<>0 AND donor.ScalingStatValue<>0;

SELECT COUNT(*) AS bgmode_plate_items_expected_9
FROM bgmode_plate_heirlooms h JOIN item_template i ON i.entry=h.entry
WHERE i.name=h.name AND i.Quality=7 AND i.AllowableClass=3;
SELECT h.entry AS missing_or_conflicting_id,h.name AS expected_name,i.name AS actual_name
FROM bgmode_plate_heirlooms h LEFT JOIN item_template i ON i.entry=h.entry
WHERE i.entry IS NULL OR i.name<>h.name OR i.Quality<>7 OR i.AllowableClass<>3;
DROP TEMPORARY TABLE bgmode_plate_heirlooms;
