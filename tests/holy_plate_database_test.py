#!/usr/bin/env python3
"""Test the Holy plate completion set on a disposable database over a local Unix socket."""
import argparse
from pathlib import Path
import subprocess
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', default='mariadb')
    parser.add_argument('--socket', required=True)
    args = parser.parse_args()
    database = 'bgmode_holy_plate_test_' + uuid.uuid4().hex
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
        install = (root / 'data/sql/db-world/base/mod_battleground_mode_heirloom_holy_plate_set.sql').read_text()
        sources = [(16727,31207),(11623,26137),(16723,29966),(16722,29968),
                   (16724,29970),(16728,29972),(16725,29967),(13001,9833),
                   (12103,23717),(16729,29971),(16726,29969),(22319,34802)]
        sql("INSERT INTO item_template (entry,name,displayid,Quality,Flags,ScalingStatDistribution,ScalingStatValue,spellid_1) VALUES "
            "(48683,'Mystical Vest of Elements',31416,7,134221824,332,4194312,57353);")
        sql("INSERT INTO item_template (entry,name,displayid) VALUES " +
            ','.join(f"({entry},'stock appearance',{display})" for entry, display in sources) + ';')
        before = sql('SELECT * FROM item_template ORDER BY entry;')
        sql(install)
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900074 AND 900085;', '12', 'all Holy plate pieces installed exactly once')
        expect('SELECT * FROM item_template WHERE entry<900074 ORDER BY entry;', before, 'stock donor and appearances unchanged')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900074 AND 900085 AND (AllowableClass<>2 OR AllowableRace<>-1 OR Quality<>7 OR RequiredLevel<>1 OR ScalingStatDistribution<>332);',
               '0', 'correct classes, all races, existing Holy plate distribution')
        expect('SELECT COUNT(DISTINCT InventoryType) FROM item_template WHERE entry BETWEEN 900074 AND 900085;',
               '12', 'each custom piece occupies a distinct equipment slot')
        expect('SELECT displayid,InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900074;',
               '31207\t1\t4\t8388616', 'Holy plate head appearance and major-slot scaling')
        expect('SELECT displayid,InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900075;',
               '26137\t16\t1\t786432', 'cloak uses cloth cloak armor with caster profile')
        expect('SELECT InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900078;',
               '10\t4\t257', 'gloves use plate shoulder budget approximation')
        expect('SELECT InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900077;',
               '9\t4\t262400', 'bindings use minor stats with plate armor, not cloak armor')
        expect('SELECT InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900082;',
               '2\t0\t262144', 'necklace uses tertiary stats without armor')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900074 AND 900085 AND (SellPrice<>0 OR BuyPrice<>0 OR DisenchantID<>0 OR MaxDurability<>0 OR spellid_1<>0 OR spellid_2<>0 OR itemset<>0);',
               '0', 'no added XP, set bonus, resale, disenchant or repair costs')
        expect('SELECT InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900083;',
               '3\t4\t257', 'custom Holy shoulders use caster stats with plate shoulder armor')
        expect('SELECT InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900084;',
               '5\t4\t8388616', 'custom Holy chest uses caster stats rather than physical Valor distribution')
        expect('SELECT InventoryType,subclass,ScalingStatValue,armor,block FROM item_template WHERE entry=900085;',
               '23\t0\t262144\t0\t0', 'tome is a legal held offhand without pretending to have shield armor/block')
        sql("UPDATE item_template SET name='Other module item',displayid=42 WHERE entry=900080;")
        sql(install)
        expect('SELECT name,displayid FROM item_template WHERE entry=900080;',
               'Other module item\t42', 'ID collision preserves the existing template')
        sql('DELETE FROM item_template WHERE entry IN (900080,16725);')
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry=900080;', '0', 'missing appearance does not create broken boots')
        print(f'{checks} Holy-plate database checks passed')
    finally:
        sql(f'DROP DATABASE `{database}`', False)


if __name__ == '__main__':
    main()
