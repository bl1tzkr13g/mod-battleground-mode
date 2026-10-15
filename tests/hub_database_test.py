#!/usr/bin/env python3
"""Test base world SQL on a unique disposable database over a local Unix socket."""
import argparse
from pathlib import Path
import subprocess
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', default='mariadb')
    parser.add_argument('--socket', required=True)
    parser.add_argument('--user', default='root')
    args = parser.parse_args()
    database = 'bgmode_hub_test_' + uuid.uuid4().hex
    client = [args.client, '--no-defaults', '--protocol=socket', '--socket=' + args.socket,
              '--user=' + args.user, '--batch', '--skip-column-names']
    root = Path(__file__).resolve().parents[1]
    checks = 0

    def sql(statement, selected=True):
        result = subprocess.run(client + ([database] if selected else []), input=statement,
                                text=True, capture_output=True)
        if result.returncode:
            raise AssertionError(result.stderr)
        return result.stdout.strip()

    def expect(statement, expected, label):
        nonlocal checks
        actual = sql(statement)
        if actual != expected:
            raise AssertionError(f'{label}: expected {expected!r}, got {actual!r}')
        checks += 1

    sql(f'CREATE DATABASE `{database}`', False)
    try:
        sql((root / 'tests/fixtures/hub_world_schema.sql').read_text())
        entries = [12794,12795,12788,15350,34038,34063,34060,34043,12793]
        sql('INSERT INTO creature_template VALUES ' + ','.join(f"({i},'original')" for i in entries) + ';'
            "INSERT INTO gameobject_template VALUES (192254,5,'Horde Banner');"
            "INSERT INTO creature (guid,id1,map,Comment) VALUES (9000000,12794,1,'original vendor');"
            "INSERT INTO gameobject (guid,id,map,Comment) VALUES (9000000,192254,1,'original banner');")
        install = (root / 'data/sql/db-world/base/mod_battleground_mode.sql').read_text()
        remove = (root / 'data/sql/manual/remove_horde_pvp_hubs.sql').read_text()
        expect_install = '18\n4'
        if sql(install) != expect_install:
            raise AssertionError('expected 18 copied vendors and 4 banners')
        checks += 1
        expect('SELECT map,COUNT(*) FROM creature GROUP BY map ORDER BY map;',
               '1\t1\n530\t9\n571\t9', 'nine vendors per hub and original preserved')
        expect('SELECT map,COUNT(*) FROM gameobject GROUP BY map ORDER BY map;',
               '1\t1\n530\t2\n571\t2', 'two banners per hub and original preserved')
        expect('SELECT COUNT(*) FROM creature WHERE map IN (530,571) AND (MovementType<>0 OR wander_distance<>0 OR phaseMask<>1);',
               '0', 'vendors remain stationary and visible in phase one')
        expect('SELECT COUNT(*) FROM creature WHERE id1=12794 AND ((map=530 AND ABS(position_x+1760.0829)<0.001 AND ABS(position_y-5321.308)<0.001 AND ABS(position_z+12.428)<0.001) OR (map=571 AND ABS(position_x-5891.34)<0.001 AND ABS(position_y-635.432)<0.001 AND ABS(position_z-647.682)<0.001));',
               '2', 'Zarg is exactly at both supplied centers')
        expect('SELECT COUNT(*) FROM gameobject WHERE map IN (530,571) AND ABS(rotation2*rotation2+rotation3*rotation3-1)<0.00001;',
               '4', 'banner rotations are normalized')
        ids = sql('SELECT GROUP_CONCAT(guid ORDER BY guid) FROM creature;')
        banner_ids = sql('SELECT GROUP_CONCAT(guid ORDER BY guid) FROM gameobject;')
        sql("UPDATE creature SET position_x=0 WHERE Comment='mod-battleground-mode:horde-hub:Shattrath:vendor:12794';")
        if sql(install) != expect_install:
            raise AssertionError('repeat import counts differ')
        checks += 1
        expect('SELECT GROUP_CONCAT(guid ORDER BY guid) FROM creature;', ids, 'reimport preserves creature GUIDs')
        expect('SELECT GROUP_CONCAT(guid ORDER BY guid) FROM gameobject;', banner_ids, 'reimport preserves banner GUIDs')
        expect("SELECT COUNT(*) FROM creature WHERE Comment='mod-battleground-mode:horde-hub:Shattrath:vendor:12794' AND ABS(position_x+1760.0829)<0.001;",
               '1', 'reimport updates tagged positions')
        expect("SELECT COUNT(*) FROM creature_template WHERE name='original';", '9', 'stock templates unchanged')
        sql(remove)
        sql(remove)
        expect('SELECT guid,id1,map,Comment FROM creature;', '9000000\t12794\t1\toriginal vendor', 'removal preserves original NPC')
        expect('SELECT guid,id,map,Comment FROM gameobject;', '9000000\t192254\t1\toriginal banner', 'removal preserves original banner')
        if sql(install) != expect_install:
            raise AssertionError('reinstall after removal failed')
        checks += 1
        print(f'{checks} hub database checks passed')
    finally:
        sql(f'DROP DATABASE `{database}`', False)


if __name__ == '__main__':
    main()
