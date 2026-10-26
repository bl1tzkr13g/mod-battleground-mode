# Tier 1 heirloom starters

Every enabled class list at **19, 29, 39, 49, 59, 69, and 79** includes the same
heirlooms. They scale through the core's normal heirloom system. Tier 2+ catalogs
and honor prices are unchanged. This remains one list per class; no specialization
selection is inferred. Hybrid defaults match the physical roles used previously.

| Class / default role | Shoulders | Chest | Weapon(s) | Stat trinket |
|---|---:|---:|---|---:|
| Warrior | 42949 | 48685 | 42943 — Bloodied Arcanite Reaper | 42991 |
| Paladin, Retribution | 42949 | 48685 | 44092 — Reforged Truesilver Champion | 42991 |
| Hunter | 42950 | 48677 | 42944 — Balanced Heartseeker; 42946 — Charmed Ancient Bone Bow | 42991 |
| Rogue | 42952 | 48689 | 42944 — Balanced Heartseeker; 44091 — Sharpened Scarlet Kris (offhand) | 42991 |
| Priest | 42985 | 48691 | 42947 — Dignified Headmaster's Charge | 42992 |
| Shaman, Enhancement | 42950 | 48677 | 48716 — Venerable Mass of McGowan | 42991 |
| Mage | 42985 | 48691 | 42947 — Dignified Headmaster's Charge | 42992 |
| Warlock | 42985 | 48691 | 42947 — Dignified Headmaster's Charge | 42992 |
| Druid, Feral | 42952 | 48689 | 48718 — Repurposed Lava Dredger | 42991 |

42991 is Swift Hand of Justice; 42992 is Discerning Eye of the Beast. Every kit
also defines **50255 — Dread Pirate Ring** and both faction insignias. Only the
matching insignia is actually delivered: **44098** for Alliance or **44097** for
Horde. It occupies the second trinket slot. Other race/class equip restrictions
remain enforced by the core. Hunters of every race get the bow; required bow
proficiency can be learned automatically. No race-specific gun substitution is
implicit. The full item names are comments above every list in the config.

Death Knight lists stay empty because enrollment remains restricted to new level
1 characters. The non-heirloom Ancestral Claymore (38691) and test shoulder (44090)
are excluded. Unused alternative heirlooms are not extra grants.

Priest, Mage and Warlock now use the [full caster set](caster-starter-set.md),
with custom scaling armor, cloak, jewelry and wand in place of all old filler.
Rogue/Feral Druid now use the [agility leather completion set](leather-starter-set.md)
at all brackets; Hunter/Enhancement Shaman receive it below 40. Other physical
lists still retain ordinary filler armor. Warrior/Paladin level-19 kits still
lack a helmet; shields and Hunter ammunition are not supplied. Shaman receives one mace, without implicitly granting Dual Wield.
The core's normal class-spell training teaches Rogue Dual Wield; if disabled or
blocked, the core still enforces that requirement before granting the kit.
Caster/healer hybrids need edits to their class lists before choosing BG mode.

`BattlegroundMode.StarterGear.TrainWeaponProficiency = 1` teaches only the legal
weapon-family proficiencies needed by configured starter weapons. It does not
grant talents, Dual Wield, or weapon skill ranks. Set to `0` to enforce existing
proficiencies without teaching any. Heirloom plate/mail below 40 uses the core's
built-in armor adaptation; ordinary plate/mail retains its level restrictions.
Paired rings, trinkets and one-handed weapons are preflighted into separate slots.
Auto-equip reserves space for displaced gear plus one working slot and equips
each piece immediately; bag-only delivery requires space for the complete kit.

Update the module, build/install, copy the updated Tier 1 lists into the active
`.conf`, then restart. Configured lists replace the old lists; a `.conf.dist` file
does not overwrite the active config. No additional SQL is needed. Completed
starter deliveries are not reset. Fresh eligible test characters receive the new
kits; claiming is still protected against repeated grants.

The default lists were checked against the pinned stock item-template snapshot
used by the original gear research. In-game checks should cover level 19 and 79,
both factions, Rogue dagger hands, trinket slots, Hunter bow, weapon proficiency,
and low-level mail/plate adaptation. Two-handed weapons leave the offhand empty;
replaced original equipment moves into the bags.
