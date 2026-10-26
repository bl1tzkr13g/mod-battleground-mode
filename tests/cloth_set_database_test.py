#!/usr/bin/env python3
"""Test the caster completion set on a disposable database over a local Unix socket."""
import argparse
from pathlib import Path
import subprocess
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', default='mariadb')
    parser.add_argument('--socket', required=True)
    args = parser.parse_args()
    database = 'bgmode_cloth_test_' + uuid.uuid4().hex
    client = [args.client, '--no-defaults', '--protocol=socket', '--socket=' + args.socket,
              '--user=root', '--batch', '--skip-column-names']
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
        sql((root / 'tests/fixtures/heirloom_item_schema.sql').read_text())
        install = (root / 'data/sql/db-world/base/mod_battleground_mode_heirloom_cloth_set.sql').read_text()
        # Real source metadata (not the full stock world database).
        sources = [(16698,31263),(13122,28652),(16702,29793),(16703,29795),
                   (16705,29800),(16699,29797),(16704,29799),(7001,20824),
                   (13001,9833),(12103,23717)]
        sql("INSERT INTO item_template (entry,name,displayid,Quality,Flags,ScalingStatDistribution,ScalingStatValue,spellid_1) VALUES "
            "(48691,'Tattered Dreadmist Robe',29792,7,134221824,336,1048584,57353);")
        sql("INSERT INTO item_template (entry,name,displayid,delay,dmg_type1) VALUES " +
            ','.join(f"({entry},'stock appearance',{display},{1500 if entry == 7001 else 0},{5 if entry == 7001 else 0})" for entry, display in sources) + ';')
        before = sql('SELECT * FROM item_template ORDER BY entry;')
        # Simulate the earlier deployed hood independently of the retired SQL.
        # A set upgrade must preserve its ID and existing template.
        sql("INSERT INTO item_template (entry,name,class,subclass,displayid,Quality,AllowableClass,InventoryType,RequiredLevel,ScalingStatDistribution,ScalingStatValue) "
            "VALUES (900019,'Tattered Dreadmist Hood',4,1,31263,7,400,1,1,336,1048584);")
        hood = sql('SELECT * FROM item_template WHERE entry=900019;')
        sql(install)
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900019 AND 900028;', '10', 'all custom pieces installed exactly once')
        expect('SELECT * FROM item_template WHERE entry=900019;', hood, 'tested hood preserved during upgrade')
        expect('SELECT * FROM item_template WHERE entry<900019 ORDER BY entry;', before, 'stock templates are untouched')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900019 AND 900028 AND (AllowableClass<>400 OR AllowableRace<>-1 OR Quality<>7 OR RequiredLevel<>1 OR ScalingStatDistribution<>336);',
               '0', 'all pieces use caster restrictions and existing scaling distribution')
        expect('SELECT COUNT(DISTINCT InventoryType) FROM item_template WHERE entry BETWEEN 900019 AND 900028;',
               '10', 'all custom pieces occupy different slots')
        expect('SELECT displayid,InventoryType,ScalingStatValue FROM item_template WHERE entry=900020;',
               '28652\t16\t786432', 'cloak appearance and cloak armor scaling')
        expect('SELECT class,subclass,InventoryType,delay,dmg_type1,ScalingStatValue FROM item_template WHERE entry=900026;',
               '2\t19\t26\t1500\t5\t278528', 'wand has a legal ranged slot, school, speed and scaling DPS mask')
        expect('SELECT ScalingStatValue FROM item_template WHERE entry=900024;', '1048584', 'leggings use major-slot scaling')
        expect('SELECT ScalingStatValue FROM item_template WHERE entry=900022;', '786432', 'bracers use minor stats with cloak armor approximation')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900019 AND 900028 AND (SellPrice<>0 OR BuyPrice<>0 OR DisenchantID<>0 OR MaxDurability<>0 OR spellid_1<>0 OR spellid_2<>0 OR itemset<>0);',
               '0', 'no resale, disenchant, repair, XP or set-bonus effects')
        sql("UPDATE item_template SET name='Other module item',displayid=42 WHERE entry=900025;")
        sql(install)
        expect('SELECT name,displayid FROM item_template WHERE entry=900025;', 'Other module item\t42', 'ID collisions never overwrite another item')
        sql('DELETE FROM item_template WHERE entry IN (900025,16704);')
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry=900025;', '0', 'missing appearance source does not create broken boots')
        # A fresh install now needs only the full-set SQL, including its hood.
        sql('DELETE FROM item_template WHERE entry BETWEEN 900019 AND 900028;')
        sql("INSERT INTO item_template (entry,name,displayid) VALUES (16704,'stock boots',29799);")
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900019 AND 900028;',
               '10', 'fresh install creates the whole set without retired SQL')
        expect('SELECT name,displayid,InventoryType,Quality,AllowableClass FROM item_template WHERE entry=900019;',
               'Tattered Dreadmist Hood\t31263\t1\t7\t400', 'fresh hood is provided by the full-set SQL')
        print(f'{checks} cloth-set database checks passed')
    finally:
        sql(f'DROP DATABASE `{database}`', False)


if __name__ == '__main__':
    main()
