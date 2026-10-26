# Agility leather starter completion set

Nine custom scaling pieces complete the leather armor, cloak and jewelry slots:

| Slot | Item | ID | Stock appearance |
|---|---|---:|---|
| Head | Stained Shadowcraft Cap | 900029 | Shadowcraft Cap (16707) |
| Back | Stained Shadowcraft Cloak | 900030 | Blackveil Cape (11626) |
| Waist | Stained Shadowcraft Belt | 900031 | Shadowcraft Belt (16713) |
| Wrists | Stained Shadowcraft Bracers | 900032 | Shadowcraft Bracers (16710) |
| Hands | Stained Shadowcraft Gloves | 900033 | Shadowcraft Gloves (16712) |
| Legs | Stained Shadowcraft Pants | 900034 | Shadowcraft Pants (16709) |
| Feet | Stained Shadowcraft Boots | 900035 | Shadowcraft Boots (16711) |
| Ring 2 | Stained Shadowcraft Signet | 900036 | Painweaver Band (13098) |
| Neck | Stained Shadowcraft Pendant | 900037 | Mark of Fordring (15411) |

## Classes and brackets

| Class / default role | Custom leather granted at | Existing heirloom chest / shoulders | Existing weapons |
|---|---|---|---|
| Rogue | 19, 29, 39, 49, 59, 69, 79 | Stained Shadowcraft Tunic / Spaulders | Balanced Heartseeker + Sharpened Scarlet Kris |
| Druid, Feral | 19, 29, 39, 49, 59, 69, 79 | Stained Shadowcraft Tunic / Spaulders | Repurposed Lava Dredger |
| Hunter | 19, 29, 39 | Champion's Deathdealer Breastplate / Champion Herod's Shoulder | Balanced Heartseeker + Charmed Ancient Bone Bow |
| Shaman, Enhancement | 19, 29, 39 | Champion's Deathdealer Breastplate / Champion Herod's Shoulder | Venerable Mass of McGowan |

Hunter/Shaman mail heirloom chest/shoulders already use the core's low-level
leather adaptation. Only the completion pieces change to actual leather. Their
level-49/59/69/79 lists now use the separate [agility mail completion set](mail-starter-set.md). Warriors and Paladins wear mail below 40, so they do not
receive this leather set. Death Knights remain ineligible for level-1 enrollment.

These are **agility/physical** defaults using the existing Shadowcraft heirloom
stat distribution (stock 335, copied from item 48689). They are not caster or
healing leather; [caster/healer leather alternatives](caster-leather-starter-set.md)
now have separate ready-to-copy loadouts. No automatic spec detection is introduced.

Every affected kit retains Dread Pirate Ring, Swift Hand of Justice and the
correct Alliance/Horde insignia. Both insignia IDs are listed, but only the
compatible one is delivered. All applicable races are permitted. Custom class
mask 1100 allows Hunter, Rogue, Shaman and Druid. Other classes are excluded.

The old ordinary helmet, belt, bracers, gloves, pants and boots are removed from
these 20 class/bracket lists. A head piece is now included at level 19 too.
Rogue/Hunter get 16 delivered items; Druid/Shaman get 15. Hunter/Shaman offhand,
Rogue ranged and Druid relic slots remain empty. No Dual Wield talent, shield,
ammunition, ranged Rogue weapon or Druid idol is granted by this update. The
existing weapon proficiency and class-trainer settings continue to apply.

## Scaling and economy

| Pieces | Mask | Initial approximation |
|---|---:|---|
| Head, pants | `0x200008` | Primary stats plus leather chest armor |
| Belt, gloves, boots | `0x41` | Shoulder stats plus leather shoulder armor |
| Cloak, bracers | `0xC0000` | Tertiary stats plus cloak armor |
| Neck, signet | `0x40000` | Tertiary stats without armor |

There are no separate stock scaling columns for every slot. In particular,
bracers use a cloak-armor approximation. These initial budgets require in-game
balance checks, not just database validation. The new pieces have no added XP
spells, resale/disenchant value, sockets, repair cost or item-set bonuses.
Existing stock heirlooms keep their normal effects. New templates and appearances
reuse existing client data; client patch work for custom IDs' icons and
right-click equip remains deferred, as for the cloth set.

## Install and test

Pull the update, copy the affected Tier 1 config lists into your active module
`.conf`, and restart worldserver. The normal database updater automatically
applies `mod_battleground_mode_heirloom_leather_set.sql`; expect nine custom items
and no missing/conflicting-ID rows. No manual SQL import is needed. Rebuild the
current module once for automatic starter bags and improved delivery. Existing
custom IDs and stock templates are never overwritten.

Use fresh eligible characters. A 20-slot Frostweave Bag now equips in the first
empty bag slot before gear, without replacing any existing bags. Auto-equip
reserves room for replaced gear plus one reusable working slot, then equips each
piece immediately. Bag-only delivery
still requires 15 slots for Druid/Shaman or 16 for Rogue/Hunter. `.bgstarter`
retries after clearing space; completed deliveries are not reset. The improved
auto-equip staging requires rebuilding the updated module once. Existing testers can add individual IDs above
with GM `.additem` to inspect them without resetting claims.

Verify at least: Rogue/Druid at 19 and 79, Hunter/Shaman at 19 and 39, their
mail completion set at 49, both factions and applicable races, item models and actual
stat changes on equip/unequip/relog, dagger hands and distinct rings/trinkets.
The new leather items' scaling and armor budgets have not been verified in-game.
