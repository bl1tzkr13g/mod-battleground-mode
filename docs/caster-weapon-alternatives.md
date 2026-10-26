# Caster weapon alternatives in starter bags

Caster builds keep their configured primary weapons equipped and receive a
small, usable alternate combination in their bags through the same one-time
starter grant. All seven default brackets are covered.

| Build | Primary remains equipped | Bag alternatives |
|---|---|---|
| Priest, Mage, Warlock | Dignified Headmaster's Charge | Dignified Scholar's Kris + Tome |
| Balance/Restoration Druid | Dignified Headmaster's Charge | Dignified Scholar's Kris + Tome |
| Elemental/Restoration Shaman | Dignified Headmaster's Charge | Devout Aurastone Hammer (42948) + caster shield |
| Holy Paladin | Devout Aurastone Hammer + Blessed Lightforge Tome | Caster shield |

Players can equip the alternate main hand first, then the offhand/shield. The
staff moves into bags through native inventory handling. This does not select
talents, respec the player, or grant Dual Wield. Physical builds get no extra
caster items. Holy already has its one-handed weapon and tome, so neither is
duplicated. Paladin/Shaman relic slots remain unchanged.

## Scaling dagger and tome

| Item | ID | Source appearance | Existing scaling donor | Mask |
|---|---:|---|---|---:|
| Dignified Scholar's Kris | 900086 | Witchblade (13964) | The Blessed Hammer of Grace (44094), profile 295 | `0x8804` |
| Dignified Scholar's Tome | 900087 | Tome of Divine Right (22319) | Tattered Dreadmist Robe (48691), profile 336 | `0x40000` |

Both items allow Priest/Mage/Warlock/Druid (class mask **1424**) of all applicable
races and require level 1. The dagger is a **main-hand-only dagger**, speed 1.6,
with native one-handed caster damage/stat/spell-power scaling. Each listed class
can legally learn daggers. The tome uses the existing tertiary caster stat
budget, with no armor or block. These are initial approximate budgets that need
in-game balance checks. No new DBC curves or artwork are introduced.

Custom items have no copied XP spells, source procs, set bonuses, random affixes,
sockets, resale/disenchant value or repair cost. SQL preserves existing stock
templates and occupied custom IDs, reporting collisions. Custom-ID icons and
right-click equip still depend on the deferred client patch; manual dragging
uses the existing native inventory flow.

## Caster shields

| Bracket | Shield | ID | Required level | Base armor | Base block |
|---|---|---:|---:|---:|---:|
| 19 | Forest Buckler | 6383 | 19 | 478 | 9 |
| 29 | Dokebi Buckler | 14608 | 26 | 595 | 12 |
| 39 | Ravager's Shield | 14777 | 39 | 1380 | 20 |
| 49 | Gizlock's Hypertech Buckler | 17718 | 48 | 1835 | 32 |
| 59 | Observer's Shield | 18485 | 56 | 2089 | 38 |
| 69 | Draenei Honor Guard Shield | 31287 | 68 | 3615 | 80 |
| 79 | Tharon'ja's Aegis | 35636 | 74 | 6541 | 181 |

These stock shields have fixed intellect-oriented stats and native armor/block.
Despite its name, Draenei Honor Guard Shield is not race/faction restricted.
The choices have no profession/reputation requirement or random affix. Stock
spell effects and sell/disenchant properties remain. Existing heirloom tables
have no dedicated shield armor/block curve, so shields stay bracket-specific.
Base block above excludes strength/talent/rating contributions.

## One config, one claim

Primary lists keep their existing `Gear.LevelNN.Tier1.Class.Build` keys. Optional
bag lists add `Bag` after `Gear`, with the same free `itemID:0` format:

```ini
BattlegroundMode.Gear.Bag.Level19.Tier1.Mage.Damage = "900086:0,900087:0"
BattlegroundMode.Gear.Bag.Level19.Tier1.Shaman.Elemental = "42948:0,6383:0"
BattlegroundMode.Gear.Bag.Level19.Tier1.Paladin.Holy = "6383:0"
```

The distributed config places each bag list directly below its primary list.
An empty/missing bag list gives no alternatives. A bag list alone never creates
a selectable build. Duplicates across the primary and bag lists, malformed
recognized lists or invalid item metadata disable the entire affected build;
known valid other builds remain available. Native item-use checks verify the
actual character can use each alternative before claiming anything.

`TrainWeaponProficiency=1` now covers primary and bag weapons, including when
`AutoEquip=0`; it still grants only class-legal proficiencies, not skill ranks.
With `AutoEquip=1`, primary items equip first and alternatives are stored last.
Storage preflight reserves room for all alternatives, all displaced equipment
and one reusable staging item. With `AutoEquip=0`, primary plus alternate items
all go into bags. The 20-slot starter bag equips first as before. A complete
fresh caster grant needs at most 18 item slots after faction filtering.

All items are prepared and preflighted before the same durable starter claim.
Primary equipment and bag-item positions are verified, then saved together with
the completion marker in one asynchronous inventory transaction. No additional
entitlement or character schema is added. Full bags leave the claim pending;
free room and retry `.bgstarter`. Interrupted claims still require GM review.
Completed claims do not receive new alternatives retroactively, even on relog,
config change or repeated requests. Binding uses the existing starter setting.
`.bggear 1` labels primary equipment and bag alternatives separately.

## Install and test

Pull, **rebuild/install worldserver**, update the active module `.conf` with the
new bag lists, then restart. Updating `.conf.dist` alone does not change an active
config. The startup updater automatically applies
`mod_battleground_mode_heirloom_caster_alternatives.sql`; expect two templates and
no missing/conflicting-ID rows. No manual SQL import is needed.

Test fresh characters in each caster build at low/high and intervening brackets:
verify primary weapons stay equipped, bag alternatives are usable, the alternate
main hand equips before the offhand/shield, native stats change, and all items
survive relog. Test both factions, the class-legal dagger/mace proficiencies,
full-bag refusal/retry, `AutoEquip=0`, repeated grants, disabled bag lists and
invalid/duplicate bag entries. Database/config/reservation tests and core
compilation do not simulate these live inventory actions or establish balance.
