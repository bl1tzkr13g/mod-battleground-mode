-- Battleground Mode base WORLD database setup.
-- Applied automatically to the WORLD database by the core startup updater.
-- Adds stock Horde and Alliance vendors at the four player-supplied locations.
-- Reimport updates only this script's tagged spawns, without duplicating them.
-- Original NPCs, creature templates, inventories and honor costs are untouched.
-- AUTO_INCREMENT assigns GUIDs; no fixed GUID range is reserved or overwritten.
-- Both cities use the supplied facing. Row runs perpendicular to that facing.
-- 3 yards between vendors, 4 per side; banners at +/-15 yards.
-- Constant center Z is a starting layout: inspect terrain/obstacles in game.
-- Decorative banners 192254/192252 are type 5 (GENERIC), not capture flags.

DROP TEMPORARY TABLE IF EXISTS bgmode_hub_centers;
CREATE TEMPORARY TABLE bgmode_hub_centers (
    faction VARCHAR(8), city VARCHAR(16), map SMALLINT UNSIGNED, zone SMALLINT UNSIGNED,
    area SMALLINT UNSIGNED, x DOUBLE, y DOUBLE, z DOUBLE, facing DOUBLE,
    banner INT UNSIGNED, PRIMARY KEY (faction,city)
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_hub_centers VALUES
('horde','Shattrath',530,3703,3703,-1760.0829,5321.308,-12.428,2.23232,192254),
('horde','Dalaran',571,4395,4613,5891.34,635.432,647.682,3.4383,192254),
('alliance','Shattrath',530,3703,3703,-1800.9764,5363.1265,-12.428,5.498769,192252),
('alliance','Dalaran',571,4395,4740,5729.1016,746.7268,641.76764,4.0660844,192252);

DROP TEMPORARY TABLE IF EXISTS bgmode_hub_vendors;
CREATE TEMPORARY TABLE bgmode_hub_vendors (
    faction VARCHAR(8), entry INT UNSIGNED PRIMARY KEY, slot SMALLINT, equipment TINYINT
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_hub_vendors VALUES
('horde',34043,-4,1), -- Lady Palanseer
('horde',12793,-3,1), -- Brave Stonehide
('horde',12795,-2,1), -- First Sergeant Hola'mahi
('horde',12788,-1,1), -- Legionnaire Teena
('horde',12794, 0,1), -- Stone Guard Zarg: exactly at supplied center
('horde',15350, 1,1), -- Horde Warbringer
('horde',34038, 2,1), -- Sergeant Thunderhorn
('horde',34063, 3,1), -- Blood Guard Zar'shi
('horde',34060, 4,0), -- Doris Volanthius
('alliance',34081,-4,1), -- Captain O'Neal
('alliance',12781,-3,1), -- Master Sergeant Biggins
('alliance',12785,-2,1), -- Sergeant Major Clate
('alliance',12778,-1,1), -- Lieutenant Rachel Vaccar
('alliance',12784, 0,1), -- Lieutenant Jackspring: exactly at supplied center
('alliance',15351, 1,1), -- Alliance Brigadier General
('alliance',34075, 2,1), -- Captain Dirgehammer
('alliance',34084, 3,1), -- Knight-Lieutenant Moonstrike
('alliance',34078, 4,1); -- Lieutenant Tristia

DROP TEMPORARY TABLE IF EXISTS bgmode_hub_creatures;
CREATE TEMPORARY TABLE bgmode_hub_creatures AS
SELECT v.entry, c.map, c.zone, c.area, v.equipment,
       c.x - SIN(c.facing) * v.slot * 3 AS x,
       c.y + COS(c.facing) * v.slot * 3 AS y, c.z, c.facing,
       CONCAT('mod-battleground-mode:',c.faction,'-hub:',c.city,':vendor:',v.entry) AS tag
FROM bgmode_hub_centers c JOIN bgmode_hub_vendors v ON v.faction=c.faction;

DROP TEMPORARY TABLE IF EXISTS bgmode_hub_banner_sides;
CREATE TEMPORARY TABLE bgmode_hub_banner_sides (side SMALLINT PRIMARY KEY);
INSERT INTO bgmode_hub_banner_sides VALUES (-1),(1);
DROP TEMPORARY TABLE IF EXISTS bgmode_hub_banners;
CREATE TEMPORARY TABLE bgmode_hub_banners AS
SELECT c.banner, c.map, c.zone, c.area,
       c.x - SIN(c.facing) * s.side * 15 AS x,
       c.y + COS(c.facing) * s.side * 15 AS y, c.z, c.facing,
       CONCAT('mod-battleground-mode:',c.faction,'-hub:',c.city,':banner:',s.side) AS tag
FROM bgmode_hub_centers c CROSS JOIN bgmode_hub_banner_sides s;

START TRANSACTION;
UPDATE creature c JOIN bgmode_hub_creatures h ON c.Comment = h.tag
SET c.id1=h.entry, c.id2=0, c.id3=0, c.map=h.map, c.zoneId=h.zone, c.areaId=h.area,
    c.spawnMask=1, c.phaseMask=1, c.equipment_id=h.equipment,
    c.position_x=h.x, c.position_y=h.y, c.position_z=h.z, c.orientation=h.facing,
    c.spawntimesecs=300, c.wander_distance=0, c.MovementType=0;
INSERT INTO creature
    (id1,map,zoneId,areaId,spawnMask,phaseMask,equipment_id,
     position_x,position_y,position_z,orientation,spawntimesecs,MovementType,Comment)
SELECT h.entry,h.map,h.zone,h.area,1,1,h.equipment,h.x,h.y,h.z,h.facing,300,0,h.tag
FROM bgmode_hub_creatures h JOIN creature_template t ON t.entry=h.entry
LEFT JOIN creature c ON c.Comment=h.tag
WHERE c.guid IS NULL;

UPDATE gameobject g JOIN bgmode_hub_banners h ON g.Comment=h.tag
SET g.id=h.banner, g.map=h.map, g.zoneId=h.zone, g.areaId=h.area,
    g.spawnMask=1, g.phaseMask=1, g.position_x=h.x, g.position_y=h.y,
    g.position_z=h.z, g.orientation=h.facing, g.rotation0=0, g.rotation1=0,
    g.rotation2=SIN(h.facing/2), g.rotation3=COS(h.facing/2),
    g.spawntimesecs=300, g.animprogress=255, g.state=1;
INSERT INTO gameobject
    (id,map,zoneId,areaId,spawnMask,phaseMask,position_x,position_y,position_z,
     orientation,rotation0,rotation1,rotation2,rotation3,spawntimesecs,animprogress,state,Comment)
SELECT h.banner,h.map,h.zone,h.area,1,1,h.x,h.y,h.z,h.facing,
       0,0,SIN(h.facing/2),COS(h.facing/2),300,255,1,h.tag
FROM bgmode_hub_banners h JOIN gameobject_template t ON t.entry=h.banner AND t.type=5
LEFT JOIN gameobject g ON g.Comment=h.tag
WHERE g.guid IS NULL;
COMMIT;

-- Expect 36 vendors and 8 banners. Missing stock templates are not fabricated.
SELECT COUNT(*) AS hub_vendors_expected_36 FROM creature c
JOIN bgmode_hub_creatures h ON c.Comment=h.tag;
SELECT COUNT(*) AS hub_banners_expected_8 FROM gameobject g
JOIN bgmode_hub_banners h ON g.Comment=h.tag;

DROP TEMPORARY TABLE bgmode_hub_banners;
DROP TEMPORARY TABLE bgmode_hub_banner_sides;
DROP TEMPORARY TABLE bgmode_hub_creatures;
DROP TEMPORARY TABLE bgmode_hub_vendors;
DROP TEMPORARY TABLE bgmode_hub_centers;
