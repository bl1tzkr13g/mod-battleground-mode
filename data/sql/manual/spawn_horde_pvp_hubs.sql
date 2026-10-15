-- OPTIONAL: import into the WORLD database while worldserver is STOPPED.
-- Adds stock Horde vendors at the two player-supplied locations. No rebuild needed.
-- Reimport updates only this script's tagged spawns, without duplicating them.
-- Original NPCs, creature templates, inventories and honor costs are untouched.
-- AUTO_INCREMENT assigns GUIDs; no fixed GUID range is reserved or overwritten.
-- Both cities use the supplied facing. Row runs perpendicular to that facing.
-- 3 yards between vendors, 4 per side; banners at +/-15 yards.
-- Constant center Z is a starting layout: inspect terrain/obstacles in game.
-- Decorative banner 192254 is type 5 (GENERIC), not a capture flag.

DROP TEMPORARY TABLE IF EXISTS bgmode_hub_centers;
CREATE TEMPORARY TABLE bgmode_hub_centers (
    city VARCHAR(16) PRIMARY KEY, map SMALLINT UNSIGNED, zone SMALLINT UNSIGNED,
    area SMALLINT UNSIGNED, x DOUBLE, y DOUBLE, z DOUBLE, facing DOUBLE
) DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO bgmode_hub_centers VALUES
('Shattrath',530,3703,3703,-1760.0829,5321.308,-12.428,2.23232),
('Dalaran',571,4395,4613,5891.34,635.432,647.682,3.4383);

DROP TEMPORARY TABLE IF EXISTS bgmode_hub_vendors;
CREATE TEMPORARY TABLE bgmode_hub_vendors (
    entry INT UNSIGNED PRIMARY KEY, slot SMALLINT, equipment TINYINT
);
INSERT INTO bgmode_hub_vendors VALUES
(34043,-4,1), -- Lady Palanseer
(12793,-3,1), -- Brave Stonehide
(12795,-2,1), -- First Sergeant Hola'mahi
(12788,-1,1), -- Legionnaire Teena
(12794, 0,1), -- Stone Guard Zarg: exactly at supplied center
(15350, 1,1), -- Horde Warbringer
(34038, 2,1), -- Sergeant Thunderhorn
(34063, 3,1), -- Blood Guard Zar'shi
(34060, 4,0); -- Doris Volanthius

DROP TEMPORARY TABLE IF EXISTS bgmode_hub_creatures;
CREATE TEMPORARY TABLE bgmode_hub_creatures AS
SELECT v.entry, c.map, c.zone, c.area, v.equipment,
       c.x - SIN(c.facing) * v.slot * 3 AS x,
       c.y + COS(c.facing) * v.slot * 3 AS y, c.z, c.facing,
       CONCAT('mod-battleground-mode:horde-hub:',c.city,':vendor:',v.entry) AS tag
FROM bgmode_hub_centers c CROSS JOIN bgmode_hub_vendors v;

DROP TEMPORARY TABLE IF EXISTS bgmode_hub_banner_sides;
CREATE TEMPORARY TABLE bgmode_hub_banner_sides (side SMALLINT PRIMARY KEY);
INSERT INTO bgmode_hub_banner_sides VALUES (-1),(1);
DROP TEMPORARY TABLE IF EXISTS bgmode_hub_banners;
CREATE TEMPORARY TABLE bgmode_hub_banners AS
SELECT c.map, c.zone, c.area,
       c.x - SIN(c.facing) * s.side * 15 AS x,
       c.y + COS(c.facing) * s.side * 15 AS y, c.z, c.facing,
       CONCAT('mod-battleground-mode:horde-hub:',c.city,':banner:',s.side) AS tag
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
SET g.id=192254, g.map=h.map, g.zoneId=h.zone, g.areaId=h.area,
    g.spawnMask=1, g.phaseMask=1, g.position_x=h.x, g.position_y=h.y,
    g.position_z=h.z, g.orientation=h.facing, g.rotation0=0, g.rotation1=0,
    g.rotation2=SIN(h.facing/2), g.rotation3=COS(h.facing/2),
    g.spawntimesecs=300, g.animprogress=255, g.state=1;
INSERT INTO gameobject
    (id,map,zoneId,areaId,spawnMask,phaseMask,position_x,position_y,position_z,
     orientation,rotation0,rotation1,rotation2,rotation3,spawntimesecs,animprogress,state,Comment)
SELECT 192254,h.map,h.zone,h.area,1,1,h.x,h.y,h.z,h.facing,
       0,0,SIN(h.facing/2),COS(h.facing/2),300,255,1,h.tag
FROM bgmode_hub_banners h JOIN gameobject_template t ON t.entry=192254 AND t.type=5
LEFT JOIN gameobject g ON g.Comment=h.tag
WHERE g.guid IS NULL;
COMMIT;

-- Expect 18 vendors and 4 banners. Missing stock templates are not fabricated.
SELECT COUNT(*) AS hub_vendors_expected_18 FROM creature c
JOIN bgmode_hub_creatures h ON c.Comment=h.tag;
SELECT COUNT(*) AS hub_banners_expected_4 FROM gameobject g
JOIN bgmode_hub_banners h ON g.Comment=h.tag;

DROP TEMPORARY TABLE bgmode_hub_banners;
DROP TEMPORARY TABLE bgmode_hub_banner_sides;
DROP TEMPORARY TABLE bgmode_hub_creatures;
DROP TEMPORARY TABLE bgmode_hub_vendors;
DROP TEMPORARY TABLE bgmode_hub_centers;
