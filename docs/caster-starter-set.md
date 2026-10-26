# Priest, Mage and Warlock Tier 1 set

All three classes use the same full cloth caster kit at levels **19, 29, 39, 49,
59, 69 and 79**. Ordinary caster filler pieces are removed from those lists.
Every starter piece is free. Higher tiers have been removed; PvP vendors supply upgrades.

| Slot | Item | ID | Existing appearance |
|---|---|---:|---|
| Head | Tattered Dreadmist Hood | 900019 | Dreadmist Mask (16698), previously tested |
| Neck | Tattered Dreadmist Pendant | 900028 | Star of Mystaria (12103) |
| Shoulders | Tattered Dreadmist Mantle | 42985 | Existing stock heirloom |
| Back | Tattered Dreadmist Cloak | 900020 | Dark Phantom Cape (13122) |
| Chest | Tattered Dreadmist Robe | 48691 | Existing stock heirloom |
| Wrists | Tattered Dreadmist Bracers | 900022 | Dreadmist Bracers (16703) |
| Hands | Tattered Dreadmist Wraps | 900023 | Dreadmist Wraps (16705) |
| Waist | Tattered Dreadmist Belt | 900021 | Dreadmist Belt (16702) |
| Legs | Tattered Dreadmist Leggings | 900024 | Dreadmist Leggings (16699) |
| Feet | Tattered Dreadmist Sandals | 900025 | Dreadmist Sandals (16704) |
| Ring 1 | Dread Pirate Ring | 50255 | Existing stock heirloom |
| Ring 2 | Tattered Dreadmist Signet | 900027 | Maiden's Circle (13001) |
| Trinket 1 | Discerning Eye of the Beast | 42992 | Existing stock heirloom |
| Trinket 2 | Inherited Insignia of the Alliance / Horde | 44098 / 44097 | Faction selected at delivery |
| Main hand | Dignified Headmaster's Charge | 42947 | Existing two-handed heirloom staff |
| Ranged | Tattered Dreadmist Wand | 900026 | Gravestone Scepter (7001), Nature damage |

The staff occupies both hands; the equipped offhand is empty.
[Bag alternatives](caster-weapon-alternatives.md) provide a scaling caster dagger
and held tome without replacing the staff automatically. Shirts and tabards
are not part of this kit. The config has 17 IDs because it lists both faction
insignias; delivery filters the incompatible one, yielding **16 equipped items**.
Custom items have class mask 400 (Priest/Mage/Warlock) and permit all their races.

## Scaling and appearance

Custom pieces reuse the existing Dreadmist robe distribution, stock ID 336.
They add no custom scaling DBC records or item-set bonuses. They have no added
XP spells, resale/disenchant value, sockets or repair cost. Stock heirlooms retain
their normal properties. Existing stock templates and occupied custom IDs are
never overwritten by the setup SQL.

| Pieces | Scaling mask | Initial budget |
|---|---:|---|
| Hood, leggings | `0x100008` | Primary stats and cloth chest armor (same as tested hood) |
| Belt, wraps, sandals | `0x21` | Shoulder stats and cloth shoulder armor |
| Cloak, bracers | `0xC0000` | Tertiary stats and cloak armor |
| Pendant, signet | `0x40000` | Tertiary stats, no armor |
| Wand | `0x44000` | Tertiary stats plus stock wand DPS; donor speed/school |

These are initial approximations with existing supported columns, not claims of
exact standard slot budgets. New pieces still require in-game stat and balance
checks across brackets. The hood's scaling and equipped bonuses were tested by
the user at levels 24 and 79; that does not verify the other items automatically.
The full set needs testing at least at 19 and 79, then the intervening brackets.

Client patch work remains deferred. New custom IDs can show question-mark icons
and require drag-and-drop to equip manually. Server-side automatic starter
equipping uses the template slots and does not rely on client right-click logic.
Existing models are reused; no new artwork is required.

## Install and test

1. Pull the module update. Custom item definitions are SQL/config only; rebuild
   and install the current module once for automatic starter bags and improved
   auto-equip delivery.
2. Replace the **Priest/Mage/Warlock Tier 1 entries for all brackets** in the active
   module `.conf` with the updated `.conf.dist` entries. Updating `.conf.dist`
   alone does not replace an existing active config.
3. Restart worldserver with its normal updater enabled. It automatically applies
   `mod_battleground_mode_heirloom_cloth_set.sql`; no manual SQL import is needed.
   Expect ten custom cloth-set templates and no missing/conflicting-ID rows.
4. Test a fresh eligible character in each caster class, in both factions. The
   default starter bag supplies 20 additional slots in the first empty equipped
   bag slot, before gear delivery. Auto-equip reserves room only for replaced
   gear, bag alternatives and **one reusable working slot**, not all primary pieces simultaneously.
   Starting food/hearthstone can remain. Free bag space and use `.bgstarter` if delivery was blocked.
   With `AutoEquip=0`, all 18 items (primary plus alternatives) must fit in bags. Specialized bags may not
   accept every piece. This delivery fix requires rebuilding the module once.
5. Check all slots, separate rings/trinkets, correct faction insignia, staff and
   ranged wand, appearance, actual stat changes on equip/unequip, and relog.
   Confirm the wand fires and its damage changes between low/high brackets.

Already-completed starter grants are not repeated. Existing testers can use GM
`.additem` with the IDs above to inspect missing pieces, without resetting their
claim. The automatic updater preserves the earlier hood ID and template.
