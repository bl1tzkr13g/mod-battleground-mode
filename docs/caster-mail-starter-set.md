# Elemental/Restoration Shaman mail completion set

Nine custom scaling pieces complete caster/healer mail for Shaman in the default
mail brackets **49, 59, 69 and 79**. They reuse Mystical Vest of Elements' existing
caster distribution (stock **332**, donor **48683**) and stock Elements models.
Enhancement, Elemental and Restoration are separate selectable starter builds.

| Slot | Item | ID | Stock appearance |
|---|---|---:|---|
| Head | Mystical Coif of Elements | 900065 | Coif of Elements (16667) |
| Back | Mystical Cloak of Elements | 900066 | Spritecaster Cape (11623) |
| Waist | Mystical Cord of Elements | 900067 | Cord of Elements (16673) |
| Wrists | Mystical Bindings of Elements | 900068 | Bindings of Elements (16671) |
| Hands | Mystical Gauntlets of Elements | 900069 | Gauntlets of Elements (16672) |
| Legs | Mystical Kilt of Elements | 900070 | Kilt of Elements (16668) |
| Feet | Mystical Boots of Elements | 900071 | Boots of Elements (16670) |
| Ring 2 | Mystical Signet of Elements | 900072 | Maiden's Circle (13001) |
| Neck | Mystical Pendant of Elements | 900073 | Star of Mystaria (12103) |

Class mask **64** permits Shaman of all applicable races. Body armor is mail,
cloaks are cloth, and jewelry uses subclass 0. Normal heirloom level scaling and
level-1 item requirements are retained; default bracket selection determines
when this family is used. No client DBC records, new artwork or extra XP spells
are added. Custom items have no sockets, item-set bonuses, resale/disenchant
value or repair costs. Stock items and occupied custom IDs are never overwritten.

## Selectable caster builds

The single module config includes Elemental and Restoration at every bracket:
caster leather at 19/29/39 and caster mail at 49/59/69/79. Select the build after
the bracket; see [configuration and migration](starter-gear.md).

Both builds retain Mystical Pauldrons of Elements (42951) and Mystical Vest (48683), plus
Dignified Headmaster's Charge (42947), Discerning Eye of the Beast (42992), Dread
Pirate Ring (50255) and the matching faction insignia. The staff occupies both
hands. [Bag alternatives](caster-weapon-alternatives.md) include a caster mace
and bracket shield; the relic slot remains untouched. This shared caster/healer baseline
has **15 equipped pieces**, plus the configurable starter bag.

## Scaling budgets

| Pieces | Mask | Initial approximation |
|---|---:|---|
| Coif, kilt | `0x400008` | Primary caster stats plus mail chest armor |
| Cord, gauntlets, boots | `0x81` | Shoulder stats plus mail shoulder armor |
| Bindings | `0x40080` | Tertiary stats plus mail shoulder armor |
| Cloak | `0xC0000` | Tertiary caster stats plus cloak armor |
| Signet, pendant | `0x40000` | Tertiary caster stats without armor |

These use existing supported columns; exact slot armor weights and role balance
remain initial approximations. The caster mail items still need in-game testing.
The core's existing plate/mail starter training remains available at 40+.
Client patch work for custom IDs' icons/right-click equip remains deferred.

## Install and test

Pull and restart worldserver with its normal updater enabled. It automatically
applies `mod_battleground_mode_heirloom_caster_mail_set.sql`; expect nine caster
mail templates and no missing/conflicting-ID rows. No manual SQL import or
manual activation of an alternate file is needed. The role-selection update
requires a rebuilt/installed worldserver. Test a fresh Shaman and choose the build. The starter bag continues to equip before the gear.

Check 19/39 with the combined config for leather, then 49/79 for mail; verify the
intervening brackets too. Check both factions/races, staff proficiency, distinct
rings/trinkets, actual stats on equip/unequip, appearance, bag-first delivery and
save/relog. Existing testers can inspect the new IDs with GM `.additem` without
resetting their completed grants. The SQL and loadout tests verify metadata,
repeat application, collisions, source preservation and complete slot coverage;
they do not simulate gameplay or prove final balance.
