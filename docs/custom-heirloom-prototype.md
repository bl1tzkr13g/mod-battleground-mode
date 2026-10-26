# Experimental scaling helmet

The first prototype is **900019 — Tattered Dreadmist Hood**, a cloth helmet for
Priests, Mages and Warlocks of any race. It is created by the normal automatic
world SQL updater. Following successful user tests, it is now included in the
[complete caster Tier 1 set](caster-starter-set.md). The original prototype
implementation and test notes are retained below as historical context.

| Property | Source / behavior |
|---|---|
| Model | Dreadmist Mask, item 16698, stock display 31263 |
| Stat profile | Tattered Dreadmist Robe, item 48691, stock distribution 336 |
| Scaling mask | Donor's stock mask 1048584 (`0x00100008`) |
| Slot | Helmet |
| Quality | Heirloom |
| Level requirement | 1 |
| Binding | Bind on pickup, with the donor's account-bound flags |
| XP bonuses | None; no donor spell effects copied |
| Resale / disenchant | Disabled |

## What the core supports

AzerothCore selects a scaling stat distribution and a level-dependent scaling
value row when applying item stats. Those fields are also sent to the client in
the normal item-template query response. The new item reuses both values from an
existing stock heirloom, without adding custom client DBC records or new models.
It keeps the robe's **primary/chest budget** as an initial helmet experiment;
it does not invent a dedicated helmet budget. Armor uses the stock cloth-chest
column, so this is **not yet balanced as a normal helmet**.

The core also has an explicit cloak armor scaling column. It does not have one
separate scaling budget for every armor slot, and its scaling armor accessor has
no dedicated shield/block-value column. Belts, bracers, gloves, leggings, boots,
shields and offhands therefore need separate budget decisions and testing.
Changing server-only DBC data may change server stats while leaving client
tooltips out of sync. Reusing an existing mask avoids that particular mismatch,
but custom item-ID/model/cache behavior still needs a real client test.

Relevant supported-core code:

- `DBCStructure.h`: `ScalingStatValuesEntry::getssdMultiplier/getArmorMod`.
- `Player.cpp`: `Player::_ApplyItemBonuses` selects the character-level scaling row.
- `ItemHandler.cpp`: `HandleItemQuerySingleOpcode` sends distribution and mask.

No claim is made yet that a client patch will be unnecessary for the complete
custom collection. The first test checks that before extending the collection.
Exact level-19 and level-79 stat numbers require the game's actual scaling DBC
values; the repository's SQL DBC overlay tables contain no default records.

## Test in game

1. Update the module files and restart worldserver with the normal database
   updater enabled. This SQL-only prototype needs no new C++ build or manual
   import. The source checkout must remain visible to the core updater.
2. On a GM character, verify `.lookup item Tattered Dreadmist Hood` finds the
   expected prototype. Item 900019 is never overwritten if another module already
   occupies that ID; startup output reports a conflict or missing source item.
3. On a level-19 caster test character, use `.additem 900019`, equip the hood,
   and check model visibility, icon, tooltip, actual character stats, and relog.
4. Repeat on a level-79 caster test character. Use separate BG characters because
   the module correctly blocks changing a selected permanent bracket.
5. Check that removing it reverses its stat contributions, re-equipping restores
   them, and both Horde and Alliance characters can equip it.

The hood has since been promoted into caster Tier 1 lists, replacing the old
helmet above level 19 and adding the head slot at level 19. See the full caster
set documentation for installation and current delivery behavior.

`tests/heirloom_database_test.py` exercises repeat application, the real item
schema, source preservation, collision handling, scaling fields and clean item
properties on a disposable local database. It does not simulate the game client.
