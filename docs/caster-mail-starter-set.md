# Elemental/Restoration Shaman mail completion set

Nine custom scaling pieces complete caster/healer mail for Shaman in the default
mail brackets **49, 59, 69 and 79**. They reuse Mystical Vest of Elements' existing
caster distribution (stock **332**, donor **48683**) and stock Elements models.
Enhancement's active default lists remain unchanged.

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

## Alternate configs

- [Higher-bracket caster mail](../conf/loadouts/caster-mail-shaman-high.conf.example)
  replaces only Shaman's 49/59/69/79 entries.
- [Complete Shaman caster/healer path](../conf/loadouts/caster-shaman.conf.example)
  replaces all seven Shaman entries: caster leather at 19/29/39, caster mail at
  49/59/69/79. Lower-level pieces come from the preceding caster-leather set.

Both retain Mystical Pauldrons of Elements (42951) and Mystical Vest (48683), plus
Dignified Headmaster's Charge (42947), Discerning Eye of the Beast (42992), Dread
Pirate Ring (50255) and the matching faction insignia. The staff occupies both
hands; no shield/offhand or relic is granted. This shared caster/healer baseline
has **15 equipped pieces**, plus the configurable starter bag.

These examples are **not loaded automatically**. Replace the matching Tier 1
entries in the active module `.conf` to make the chosen role the class default
for future unclaimed grants. Completed deliveries stay completed. No in-game
role picker, automatic specialization detection or role DB field is introduced.
Default physical kits and all Tier 2+ entries remain unchanged. The high-bracket
lists are recorded separately in [caster-mail-loadouts.csv](caster-mail-loadouts.csv).

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
additional rebuild is required with the current heirloom/bag build installed.
Copy the chosen alternate entries into the active config before testing a fresh
Shaman. The starter bag continues to equip before the gear.

Check 19/39 with the combined config for leather, then 49/79 for mail; verify the
intervening brackets too. Check both factions/races, staff proficiency, distinct
rings/trinkets, actual stats on equip/unequip, appearance, bag-first delivery and
save/relog. Existing testers can inspect the new IDs with GM `.additem` without
resetting their completed grants. The SQL and loadout tests verify metadata,
repeat application, collisions, source preservation and complete slot coverage;
they do not simulate gameplay or prove final balance.
