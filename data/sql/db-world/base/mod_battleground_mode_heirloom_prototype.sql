-- Original tested hood; applied automatically by the core startup updater.
-- Now part of the caster Tier 1 completion set. No client DBC tables are modified.
-- Existing IDs are never overwritten. Validate in game before enabling broadly.
-- 48691 supplies an existing stock heirloom scaling profile; 16698 supplies
-- the Dreadmist Mask appearance. Chest-budget scaling is a prototype, not a
-- claim of slot-balanced helmet stats. No XP spell or donor set bonus is copied.
INSERT INTO item_template
    (entry,class,subclass,name,displayid,Quality,Flags,FlagsExtra,
     BuyPrice,SellPrice,InventoryType,AllowableClass,AllowableRace,
     ItemLevel,RequiredLevel,ScalingStatDistribution,ScalingStatValue,
     armor,bonding,description,MaxDurability,RequiredDisenchantSkill,DisenchantID)
SELECT 900019,4,1,'Tattered Dreadmist Hood',appearance.displayid,7,
       donor.Flags,0,0,0,1,400,-1,1,1,
       donor.ScalingStatDistribution,donor.ScalingStatValue,
       0,1,'An heirloom passed down through generations of battleground champions.',0,-1,0
FROM item_template donor JOIN item_template appearance ON appearance.entry=16698
WHERE donor.entry=48691
  AND donor.ScalingStatDistribution<>0 AND donor.ScalingStatValue<>0
  AND NOT EXISTS (SELECT 1 FROM item_template occupied WHERE occupied.entry=900019);

SELECT CASE
    WHEN EXISTS (SELECT 1 FROM item_template WHERE entry=900019
        AND name='Tattered Dreadmist Hood' AND InventoryType=1 AND Quality=7)
        THEN 'Battleground Mode: hood 900019 available for the caster starter set.'
    WHEN EXISTS (SELECT 1 FROM item_template WHERE entry=900019)
        THEN 'Battleground Mode: ID 900019 is occupied; existing item preserved, prototype skipped.'
    ELSE 'Battleground Mode: prototype skipped; check source items 48691 and 16698 and scaling fields.'
END AS bgmode_heirloom_prototype;
