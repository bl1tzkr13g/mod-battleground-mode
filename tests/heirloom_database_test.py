#!/usr/bin/env python3
"""Test the opt-in GM helmet on a disposable database over a local Unix socket."""
import argparse
from pathlib import Path
import subprocess
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', default='mariadb')
    parser.add_argument('--socket', required=True)
    args = parser.parse_args()
    database = 'bgmode_helmet_test_' + uuid.uuid4().hex
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
        install = (root / 'data/sql/db-world/base/mod_battleground_mode_heirloom_prototype.sql').read_text()
        sql("INSERT INTO item_template (entry,name,displayid,Quality,Flags,ScalingStatDistribution,ScalingStatValue,spellid_1) VALUES "
            "(48691,'Tattered Dreadmist Robe',29792,7,134221824,336,1048584,57353),"
            "(16698,'Dreadmist Mask',31263,3,0,0,0,0);")
        before = sql('SELECT * FROM item_template ORDER BY entry;')
        sql(install)
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry=900019;', '1', 'repeat apply creates only one prototype')
        expect('SELECT name,displayid,class,subclass,InventoryType,Quality FROM item_template WHERE entry=900019;',
               'Tattered Dreadmist Hood\t31263\t4\t1\t1\t7', 'head slot, quality and existing appearance')
        expect('SELECT ScalingStatDistribution,ScalingStatValue,armor FROM item_template WHERE entry=900019;',
               '336\t1048584\t0', 'uses donor scaling profile without adding static armor')
        expect('SELECT AllowableClass,AllowableRace,RequiredLevel FROM item_template WHERE entry=900019;',
               '400\t-1\t1', 'caster classes and all their races, available from level one')
        expect('SELECT spellid_1,spellid_2,spellid_3,spellid_4,spellid_5,itemset FROM item_template WHERE entry=900019;',
               '0\t0\t0\t0\t0\t0', 'no inherited XP or set bonuses')
        expect('SELECT SellPrice,BuyPrice,DisenchantID,RequiredDisenchantSkill,MaxDurability FROM item_template WHERE entry=900019;',
               '0\t0\t0\t-1\t0', 'no resale, disenchant or repair costs')
        expect('SELECT * FROM item_template WHERE entry<>900019 ORDER BY entry;', before, 'source items remain unchanged')
        sql("UPDATE item_template SET name='Other module item',displayid=42 WHERE entry=900019;")
        sql(install)
        expect('SELECT name,displayid FROM item_template WHERE entry=900019;', 'Other module item\t42', 'occupied ID is preserved')
        sql('DELETE FROM item_template WHERE entry IN (900019,16698);')
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry=900019;', '0', 'missing appearance source does not create broken item')
        print(f'{checks} prototype database checks passed')
    finally:
        sql(f'DROP DATABASE `{database}`', False)


if __name__ == '__main__':
    main()
