#!/usr/bin/env python3
"""Test the agility mail completion set on a disposable database over a local Unix socket."""
import argparse
from pathlib import Path
import subprocess
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', default='mariadb')
    parser.add_argument('--socket', required=True)
    args = parser.parse_args()
    database = 'bgmode_mail_test_' + uuid.uuid4().hex
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
        install = (root / 'data/sql/db-world/base/mod_battleground_mode_heirloom_mail_set.sql').read_text()
        sources = [(16677,31410),(13340,24013),(16680,31404),(16681,31405),
                   (16676,31406),(16678,31403),(16675,31408),(13098,23608),(15411,23716)]
        sql("INSERT INTO item_template (entry,name,displayid,Quality,Flags,ScalingStatDistribution,ScalingStatValue,spellid_1) VALUES "
            "(48677,'Champion''s Deathdealer Breastplate',28712,7,134221824,331,4194312,57353);")
        sql("INSERT INTO item_template (entry,name,displayid) VALUES " +
            ','.join(f"({entry},'stock appearance',{display})" for entry, display in sources) + ';')
        before = sql('SELECT * FROM item_template ORDER BY entry;')
        sql(install)
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900038 AND 900046;', '9', 'all mail pieces installed exactly once')
        expect('SELECT * FROM item_template WHERE entry<900038 ORDER BY entry;', before, 'stock donor and appearances unchanged')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900038 AND 900046 AND (AllowableClass<>68 OR AllowableRace<>-1 OR Quality<>7 OR RequiredLevel<>1 OR ScalingStatDistribution<>331);',
               '0', 'correct classes, all races, existing agility mail distribution')
        expect('SELECT COUNT(DISTINCT InventoryType) FROM item_template WHERE entry BETWEEN 900038 AND 900046;',
               '9', 'each custom piece occupies a distinct equipment slot')
        expect('SELECT displayid,InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900038;',
               '31410\t1\t3\t4194312', 'mail head appearance and major-slot scaling')
        expect('SELECT displayid,InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900039;',
               '24013\t16\t1\t786432', 'cloak uses cloth cloak armor despite agility profile')
        expect('SELECT InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900042;',
               '10\t3\t129', 'gloves use mail shoulder budget approximation')
        expect('SELECT InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900041;',
               '9\t3\t262272', 'bindings use minor stats with mail armor, not cloak armor')
        expect('SELECT InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900046;',
               '2\t0\t262144', 'necklace uses tertiary stats without armor')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900038 AND 900046 AND (SellPrice<>0 OR BuyPrice<>0 OR DisenchantID<>0 OR MaxDurability<>0 OR spellid_1<>0 OR spellid_2<>0 OR itemset<>0);',
               '0', 'no added XP, set bonus, resale, disenchant or repair costs')
        sql("UPDATE item_template SET name='Other module item',displayid=42 WHERE entry=900044;")
        sql(install)
        expect('SELECT name,displayid FROM item_template WHERE entry=900044;',
               'Other module item\t42', 'ID collision preserves the existing template')
        sql('DELETE FROM item_template WHERE entry IN (900044,16675);')
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry=900044;', '0', 'missing appearance does not create broken boots')
        print(f'{checks} mail-set database checks passed')
    finally:
        sql(f'DROP DATABASE `{database}`', False)


if __name__ == '__main__':
    main()
