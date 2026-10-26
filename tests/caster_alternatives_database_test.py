#!/usr/bin/env python3
"""Test caster alternative templates on a disposable MariaDB database over a local Unix socket."""
import argparse
from pathlib import Path
import subprocess
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', default='mariadb')
    parser.add_argument('--socket', required=True)
    args = parser.parse_args()
    database = 'bgmode_caster_alternatives_' + uuid.uuid4().hex
    client = [args.client, '--no-defaults', '--protocol=socket', '--socket=' + args.socket,
              '--user=root', '--batch', '--skip-column-names']
    root = Path(__file__).resolve().parents[1]
    checks = 0

    def sql(statement, selected=True):
        return subprocess.run(client + ([database] if selected else []), input=statement,
                              text=True, capture_output=True, check=True).stdout.strip()

    def expect(statement, expected, label):
        nonlocal checks
        actual = sql(statement)
        if actual != expected:
            raise AssertionError(f'{label}: expected {expected!r}, got {actual!r}')
        checks += 1

    sql(f'CREATE DATABASE `{database}`', False)
    try:
        sql((root / 'tests/fixtures/heirloom_item_schema.sql').read_text())
        install = (root / 'data/sql/db-world/base/mod_battleground_mode_heirloom_caster_alternatives.sql').read_text()
        sql("INSERT INTO item_template (entry,name,displayid,Quality,Flags,ScalingStatDistribution,ScalingStatValue,spellid_1) VALUES "
            "(44094,'The Blessed Hammer of Grace',21956,7,134221824,295,34820,42),"
            "(48691,'Tattered Dreadmist Robe',17264,7,134221824,336,1048584,57353);")
        sql("INSERT INTO item_template (entry,name,displayid,spellid_1,RequiredLevel) VALUES "
            "(13964,'Witchblade',24775,9343,57),(22319,'Tome of Divine Right',34802,9315,56);")
        before = sql('SELECT * FROM item_template ORDER BY entry;')
        sql(install)
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900086 AND 900087;', '2', 'templates are installed exactly once')
        expect('SELECT * FROM item_template WHERE entry<900086 ORDER BY entry;', before, 'stock appearance and donor templates are preserved')
        expect('SELECT displayid,class,subclass,InventoryType,delay,ScalingStatDistribution,ScalingStatValue FROM item_template WHERE entry=900086;',
               '24775\t2\t15\t21\t1600\t295\t34820', 'dagger uses legal main hand with caster 1H stats, damage and spell-power scaling')
        expect('SELECT displayid,class,subclass,InventoryType,ScalingStatDistribution,ScalingStatValue FROM item_template WHERE entry=900087;',
               '34802\t4\t0\t23\t336\t262144', 'tome uses an existing caster profile and offhand stat budget')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900086 AND 900087 AND (Quality<>7 OR AllowableClass<>1424 OR AllowableRace<>-1 OR RequiredLevel<>1 OR ItemLevel<>1);',
               '0', 'Priest Mage Warlock Druid class mask and all-level metadata are correct')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900086 AND 900087 AND (armor<>0 OR block<>0 OR BuyPrice<>0 OR SellPrice<>0 OR DisenchantID<>0 OR MaxDurability<>0 OR itemset<>0 OR spellid_1<>0 OR spellid_2<>0 OR RandomProperty<>0 OR RandomSuffix<>0);',
               '0', 'no copied source spells, XP, shield armor/block, random affixes, resale or repair costs')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900086 AND 900087 AND (Flags<>134221824 OR FlagsExtra<>0 OR bonding<>1 OR RequiredSkill<>0 OR RequiredReputationFaction<>0);',
               '0', 'account flags and binding retained without faction/profession restrictions')
        sql("UPDATE item_template SET name='Other module template',displayid=42 WHERE entry=900086;")
        sql(install)
        expect('SELECT name,displayid FROM item_template WHERE entry=900086;', 'Other module template\t42', 'occupied custom ID is never overwritten')
        sql('DELETE FROM item_template WHERE entry IN (900086,13964);')
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry=900086;', '0', 'missing appearance cannot produce a broken dagger')
        sql('DELETE FROM item_template WHERE entry IN (900087,48691);')
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry=900087;', '0', 'missing stat donor cannot produce an unscaled tome')
        print(f'{checks} caster-alternatives database checks passed')
    finally:
        sql(f'DROP DATABASE `{database}`', False)


if __name__ == '__main__':
    main()
