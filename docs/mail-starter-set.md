# Agility mail starter completion set

Hunter and Enhancement Shaman now receive nine custom scaling completion pieces
in level-40+ brackets: **49, 59, 69 and 79** in the default config. Their 19/29/39
leather lists remain unchanged. This is the physical/agility mail baseline, not
an Elemental/Restoration caster-mail loadout. Warrior/Paladin and other classes
are unchanged by this update.

| Slot | Item | ID | Stock appearance |
|---|---|---:|---|
| Head | Champion's Deathdealer Cap | 900038 | Beaststalker's Cap (16677) |
| Back | Champion's Deathdealer Cloak | 900039 | Cape of the Black Baron (13340) |
| Waist | Champion's Deathdealer Belt | 900040 | Beaststalker's Belt (16680) |
| Wrists | Champion's Deathdealer Bindings | 900041 | Beaststalker's Bindings (16681) |
| Hands | Champion's Deathdealer Gloves | 900042 | Beaststalker's Gloves (16676) |
| Legs | Champion's Deathdealer Pants | 900043 | Beaststalker's Pants (16678) |
| Feet | Champion's Deathdealer Boots | 900044 | Beaststalker's Boots (16675) |
| Ring 2 | Champion's Deathdealer Signet | 900045 | Painweaver Band (13098) |
| Neck | Champion's Deathdealer Pendant | 900046 | Mark of Fordring (15411) |

The old ordinary head/belt/wrists/gloves/legs/boots are removed from all eight
applicable class/bracket lists. Existing Champion Herod's Shoulder (42950) and
Champion's Deathdealer Breastplate (48677) remain. Both classes keep Swift Hand
of Justice, Dread Pirate Ring, and the matching Alliance/Horde insignia.

Hunter retains Balanced Heartseeker and Charmed Ancient Bone Bow; Shaman retains
Venerable Mass of McGowan. Hunter gets 16 equipped pieces and Shaman 15, plus the
configurable starter bag. Offhand and Shaman relic slots stay empty; no new
Dual Wield talent, shield, relic or ammunition is introduced. Jewelry and cloaks
use the correct non-mail subclasses; only body armor is mail.

## Scaling and restrictions

All pieces reuse the existing Deathdealer Breastplate stat distribution (stock
331, copied from donor 48677). Class mask **68** permits Hunter and Shaman of any
applicable race. The data retains the normal heirloom level-1 requirement and
stock level scaling; config selection, not a new hard level requirement, selects
these items for the mail-level brackets. No new DBC records or models are added.

| Pieces | Mask | Initial approximation |
|---|---:|---|
| Head, pants | `0x400008` | Primary stats plus mail chest armor |
| Belt, gloves, boots | `0x81` | Shoulder stats plus mail shoulder armor |
| Bindings | `0x40080` | Tertiary stats plus mail shoulder armor |
| Cloak | `0xC0000` | Tertiary stats plus cloak armor |
| Pendant, signet | `0x40000` | Tertiary stats without armor |

These masks use existing supported columns and are initial budget approximations.
Bindings use mail armor rather than the cloak-armor approximation used by the
leather set. Each new piece still needs in-game balance checks. No XP spells,
item-set bonuses, resale/disenchant value, sockets or repair cost are added.
Stock item templates and occupied custom IDs are never overwritten.

Client patch work remains deferred: custom-ID icons/right-click manual equip
may remain incomplete. Automatic server-side starter equipping still uses the
correct template slots, and existing models are reused.

## Install and test

Pull the update, replace Hunter/Shaman Tier 1 entries for **49/59/69/79** in the
active module `.conf` with the new `.conf.dist` entries, and restart worldserver.
The normal updater applies `mod_battleground_mode_heirloom_mail_set.sql`
automatically; expect nine mail items and no missing/conflicting-ID rows.
No manual import or additional C++ rebuild is needed when the current starter
bag/heirloom support is already installed. The starter bag still equips first.
Completed starter deliveries are not repeated; test fresh eligible characters.

Check Hunter and Shaman at 49 and 79, then other mail brackets, both factions and
applicable races. Verify all slots, separate rings/trinkets, model visibility,
actual stat changes and relog persistence. Recheck level 39 still receives the
leather kit and item 41599 equips before gear when a bag slot is free. In-game
scaling and balance of the new mail items remain unverified.

`tests/mail_set_database_test.py` checks creation/reapplication, appearances,
mail/cloth/jewelry slot types, scaling masks, restrictions, donor preservation,
ID collisions and missing source templates on a disposable local database.
