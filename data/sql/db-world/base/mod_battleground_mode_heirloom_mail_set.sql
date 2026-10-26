-- Battleground Mode agility mail completion set, applied automatically on startup.
-- Intended starters: Hunter and Enhancement Shaman in level-40+ brackets.
-- Their 19/29/39 lists remain leather. Uses the stock Champion Deathdealer heirloom profile from 48677.
-- No custom client DBC records or XP spells. Initial budget approximations need
-- in-game tuning. Other stock items and occupied custom IDs are never overwritten.
DROP TEMPORARY TABLE IF EXISTS bgmode_mail_heirlooms;
CREATE TEMPORARY TABLE bgmode_mail_heirlooms (
    entry INT UNSIGNED PRIMARY KEY, name VARCHAR(100), appearance INT UNSIGNED,
    inventory TINYINT UNSIGNED, item_class TINYINT UNSIGNED,
    subclass TINYINT UNSIGNED, scaling_mask INT UNSIGNED
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_mail_heirlooms VALUES
(900038,'Champion''s Deathdealer Cap',16677,1,4,3,4194312),
(900039,'Champion''s Deathdealer Cloak',13340,16,4,1,786432),
(900040,'Champion''s Deathdealer Belt',16680,6,4,3,129),
(900041,'Champion''s Deathdealer Bindings',16681,9,4,3,262272),
(900042,'Champion''s Deathdealer Gloves',16676,10,4,3,129),
(900043,'Champion''s Deathdealer Pants',16678,7,4,3,4194312),
(900044,'Champion''s Deathdealer Boots',16675,8,4,3,129),
(900045,'Champion''s Deathdealer Signet',13098,11,4,0,262144),
(900046,'Champion''s Deathdealer Pendant',15411,2,4,0,262144);

INSERT INTO item_template
    (entry,class,subclass,name,displayid,Quality,Flags,FlagsExtra,
     BuyPrice,SellPrice,InventoryType,AllowableClass,AllowableRace,
     ItemLevel,RequiredLevel,ScalingStatDistribution,ScalingStatValue,
     armor,bonding,description,MaxDurability,RequiredDisenchantSkill,DisenchantID)
SELECT h.entry,h.item_class,h.subclass,h.name,appearance.displayid,7,
       donor.Flags,0,0,0,h.inventory,68,-1,1,1,
       donor.ScalingStatDistribution,h.scaling_mask,
       0,1,'An heirloom passed down through generations of battleground champions.',0,-1,0
FROM bgmode_mail_heirlooms h
JOIN item_template donor ON donor.entry=48677
JOIN item_template appearance ON appearance.entry=h.appearance
LEFT JOIN item_template occupied ON occupied.entry=h.entry
WHERE occupied.entry IS NULL
  AND donor.ScalingStatDistribution<>0 AND donor.ScalingStatValue<>0;

SELECT COUNT(*) AS bgmode_mail_items_expected_9
FROM bgmode_mail_heirlooms h JOIN item_template i ON i.entry=h.entry
WHERE i.name=h.name AND i.Quality=7 AND i.AllowableClass=68;
SELECT h.entry AS missing_or_conflicting_id,h.name AS expected_name,i.name AS actual_name
FROM bgmode_mail_heirlooms h LEFT JOIN item_template i ON i.entry=h.entry
WHERE i.entry IS NULL OR i.name<>h.name OR i.Quality<>7 OR i.AllowableClass<>68;
DROP TEMPORARY TABLE bgmode_mail_heirlooms;
