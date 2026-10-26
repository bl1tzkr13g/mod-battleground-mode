#!/usr/bin/env python3
"""Test the agility leather completion set on a disposable database over a local Unix socket."""
import argparse
from pathlib import Path
import subprocess
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', default='mariadb')
    parser.add_argument('--socket', required=True)
    args = parser.parse_args()
    database = 'bgmode_leather_test_' + uuid.uuid4().hex
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
        install = (root / 'data/sql/db-world/base/mod_battleground_mode_heirloom_leather_set.sql').read_text()
        sources = [(16707,28180),(11626,26278),(16713,28177),(16710,24190),
                   (16712,28166),(16709,28161),(16711,28162),(13098,23608),(15411,23716)]
        sql("INSERT INTO item_template (entry,name,displayid,Quality,Flags,ScalingStatDistribution,ScalingStatValue,spellid_1) VALUES "
            "(48689,'Stained Shadowcraft Tunic',28160,7,134221824,335,2097160,57353);")
        sql("INSERT INTO item_template (entry,name,displayid) VALUES " +
            ','.join(f"({entry},'stock appearance',{display})" for entry, display in sources) + ';')
        before = sql('SELECT * FROM item_template ORDER BY entry;')
        sql(install)
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900029 AND 900037;', '9', 'all leather pieces installed exactly once')
        expect('SELECT * FROM item_template WHERE entry<900029 ORDER BY entry;', before, 'stock donor and appearances unchanged')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900029 AND 900037 AND (AllowableClass<>1100 OR AllowableRace<>-1 OR Quality<>7 OR RequiredLevel<>1 OR ScalingStatDistribution<>335);',
               '0', 'correct classes, all races, existing agility distribution')
        expect('SELECT COUNT(DISTINCT InventoryType) FROM item_template WHERE entry BETWEEN 900029 AND 900037;',
               '9', 'each custom piece occupies a distinct equipment slot')
        expect('SELECT displayid,InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900029;',
               '28180\t1\t2\t2097160', 'leather head appearance and major-slot scaling')
        expect('SELECT displayid,InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900030;',
               '26278\t16\t1\t786432', 'cloak uses cloth cloak armor despite agility profile')
        expect('SELECT InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900033;',
               '10\t2\t65', 'gloves use leather shoulder budget approximation')
        expect('SELECT InventoryType,subclass,ScalingStatValue FROM item_template WHERE entry=900037;',
               '2\t0\t262144', 'necklace uses tertiary stats without armor')
        expect('SELECT COUNT(*) FROM item_template WHERE entry BETWEEN 900029 AND 900037 AND (SellPrice<>0 OR BuyPrice<>0 OR DisenchantID<>0 OR MaxDurability<>0 OR spellid_1<>0 OR spellid_2<>0 OR itemset<>0);',
               '0', 'no added XP, set bonus, resale, disenchant or repair costs')
        sql("UPDATE item_template SET name='Other module item',displayid=42 WHERE entry=900035;")
        sql(install)
        expect('SELECT name,displayid FROM item_template WHERE entry=900035;',
               'Other module item\t42', 'ID collision preserves the existing template')
        sql('DELETE FROM item_template WHERE entry IN (900035,16711);')
        sql(install)
        expect('SELECT COUNT(*) FROM item_template WHERE entry=900035;', '0', 'missing appearance does not create broken boots')
        print(f'{checks} leather-set database checks passed')
    finally:
        sql(f'DROP DATABASE `{database}`', False)


if __name__ == '__main__':
    main()
