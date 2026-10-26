# Starter gear and build selection

All playable starter builds live in the one module config:
[mod_battleground_mode.conf.dist](../conf/mod_battleground_mode.conf.dist).
Only Tier 1 remains. Players earn upgrades from the stock PvP vendors; tiers
2–5, further tier discovery and the planned custom progression vendor are retired.

## Editing a character's gear list

```ini
BattlegroundMode.Gear.Level19.Tier1.Paladin.Retribution = "900047:0,..."
BattlegroundMode.Gear.Level19.Tier1.Paladin.Holy = "900074:0,..."
```

Use real complete item lists from the distributed config; the ellipses above
only illustrate key names. Entries remain comma-separated `itemID:0` pairs.
Every price must be zero, duplicate item IDs are rejected, and item metadata is
validated for the class/bracket. Empty or rejected lists never appear as choices.
Keep a one-handed main weapon before its held offhand. Both faction insignias
can be listed; the receiver gets only the correct faction's version.

| Class | Available builds |
|---|---|
| Warrior | Damage (two-handed strength), Protection |
| Paladin | Retribution, Holy, Protection |
| Hunter, Rogue, Mage, Warlock | Damage |
| Priest | Damage (shared caster/healer kit) |
| Shaman | Enhancement, Elemental, Restoration |
| Druid | Feral, Balance, Restoration |

These 16 builds are present at 19/29/39/49/59/69/79. Death Knights start above
level 1 and remain ineligible. [Protection Warrior/Paladin](protection-starter-set.md)
uses scaling Valor gear and a one-handed sword with bracket-appropriate stock
shields. The shields retain their fixed native armor/block values.
New brackets need complete class/build keys too. No kit is borrowed from another
bracket, class or role. Balance/Restoration and Elemental/Restoration currently
share caster/healer item lists but persist as separate choices for future tuning.

## Current runtime behavior

The flow is **Normal/BG → bracket → build → final confirmation**. Browsing or
closing the menu does not alter level, XP or inventory. Reopen with `.bgmode`.
Leveling beyond 1 before confirmation permanently selects Normal Mode as before.
The final confirmation saves mode, bracket and build in one conditional write
before applying levels, spells, gear or teleport. Stale/duplicate clicks cannot
change a completed decision. Class/build validity and the current catalog are
checked again on confirmation; unsupported/empty/removed kits are rejected.

Build selection controls starter items only: no talent assignment, respec,
automatic spec detection or additional Dual Wield grant is performed. A saved
choice stays fixed even after later talent changes. `.bggear 1` previews that
character's chosen starter kit; other tiers direct players to PvP vendors.
`.bgstarter` retries an unclaimed delivery for the saved build, never another role.
The one-time claim still covers the bag and gear together. Relogging, changing
roles in config or using old menu packets cannot yield a second kit.

## Upgrade an existing installation

Pull and **rebuild/install worldserver**. Replace the entire old gear-catalog
section in the active module `.conf` with the new section from `.conf.dist`,
preserving your server's other settings, then restart. Do not append duplicate
class keys. Separate `conf/loadouts` examples are retired and merged here.
The core startup updater adds `starter_role` automatically; no SQL import is needed.

Legacy `.Tier1.Class` keys are accepted as the old default build only, so a
legacy config offers only those existing builds until updated. Use explicit
`.Tier1.Class.Build` keys in the new config. Tiers above 1 are rejected rather
than exposed. Existing BG records with role zero keep the old default: Retribution,
Enhancement, Feral, or Damage for other classes. Existing completed/interrupted
starter claims are preserved. Editing config never resets or regrants gear.
A disabled/removed selected kit blocks an unclaimed delivery with a message;
it never silently substitutes another role's kit.

The stable database role IDs are: 0 legacy default, 1 Damage, 2 Holy,
3 Retribution, 4 Feral, 5 Balance, 6 Restoration, 7 Enhancement, 8 Elemental,
9 Protection. These IDs must not be renumbered. Role zero is only for legacy
records and non-BG decisions; new BG confirmations save a nonzero role.

## One-time starter delivery and auto-equip

```ini
BattlegroundMode.StarterGear.Enable = 1
BattlegroundMode.StarterGear.AutoEquip = 1
BattlegroundMode.StarterGear.TrainArmorProficiency = 1
BattlegroundMode.StarterGear.BindToCharacter = 1
```

On confirming Battleground Mode, the character receives its configured Tier 1
list once. With auto-equip enabled, every piece is equipped using the native
inventory swap flow. Existing gear in the same slots moves into bags; slots
absent from the configured kit remain unchanged. Old items are not deleted.
With `AutoEquip = 0`, the entire set goes into bags without changing equipment.
Changing the setting afterward does not reissue or auto-equip an already
delivered set. Characters can equip their delivered bag items manually.

Existing Battleground Mode characters receive an unclaimed starter set on their
next safe login, including replacement of their currently equipped matching
slots. Grants wait until the player is loaded, alive, out of combat, and outside
travel, trades, battlegrounds, and queues. Normal characters never get this grant.
Disabling starter delivery leaves the entitlement unclaimed for later use.

Auto-equip preflights every item and rejects kits with conflicting destination
slots. Auto-equip reserves bag room for displaced gear and one reusable working
slot, checking incoming item restrictions before claiming. Each item is stored
and equipped immediately so empty equipment slots release staging capacity.
Starting food/hearthstone do not need to be removed just to hold a complete kit.
Bag-only delivery still reserves room for the entire kit. If preflight checks
fail, nothing is delivered and the claim stays available. Use **`.bgstarter`**
after resolving the restriction. Success never grants a second set or overwrites
later upgrades. An unexpected native swap failure after claiming stops delivery
and requires GM reconciliation; the claim prevents automatic duplicate grants.

### Starter bag

```ini
BattlegroundMode.StarterGear.Bag.Enable = 1
BattlegroundMode.StarterGear.Bag.Item = 41599
```

A Frostweave Bag (20 slots) is included by default and equipped **before** the
gear, in the first empty equipped bag slot. Existing bags are never replaced.
If all four slots are occupied, no bag is added and the kit uses existing space.
Only ordinary generic bags with 1..36 slots are accepted; specialized bags are
rejected. The bag is bound when `BindToCharacter` is enabled. It equips even with
`AutoEquip=0`, since that setting controls gear delivery rather than the bag.

The bag is prepared and checked before the existing durable starter claim. Only
after that claim is verified does it enter inventory. Gear storage preflight can
account for the planned generic bag, allowing delivery from a full backpack when
the new bag alone holds the required reservation. Ownership and unique-item
restrictions remain checked. Bag and all kit items are saved in the same final
inventory transaction with the completion marker. No extra state column or
entitlement is introduced. Losing/selling the bag after a completed grant does
not allow another. Unexpected post-claim failures still require GM review.

Pull and rebuild/install the updated module, then restart. The default bag works
without adding config keys; copy them into the active config to customize or
disable it. No manual SQL import or new schema migration is needed. Already
completed starter grants stay completed; use a fresh character for bag testing.
Verify first-empty-slot behavior, existing bag preservation, a full backpack
with an empty bag slot, both gear delivery modes, repeat requests/relog, and
survival of bag contents after save/relog. The role-selection flow still needs an in-game check.

The optional training teaches only legal plate proficiency for level-40+
Warriors/Paladins and mail for level-40+ Hunters/Shamans. It does not grant weapon
skill ranks or talents. Configurable starter weapon proficiency training now
teaches class-legal skills needed to equip the supplied weapons; class-trainer
combat spells are managed separately by AutoLearnClassTrainerSpells. New starter pieces are soulbound when
`BindToCharacter` is enabled. Honor is not charged for Tier 1.

The core startup database updater automatically applies
[`mod_battleground_mode.sql`](../data/sql/db-characters/base/mod_battleground_mode.sql)
to the character database, creating fresh schemas and upgrading earlier ones
without resetting choices or completed deliveries. No manual import is required;
see [automatic database setup](automatic-database-setup.md).

The single `starter_state` column stores pending (0), claimed (1), or delivered
(2). A durable claim precedes inventory changes, then item instances, positions,
and completion are queued together on the core's asynchronous database worker
before the city teleport. Delivery is confirmed after that transaction succeeds.
Player-save statements require this worker connection; a synchronous direct
commit causes a missing prepared-statement assertion. A server crash
or exceptional failure after claiming requires a GM to reconcile inventory;
state 1 is deliberately not retried automatically. Do not blindly reset that
state, since some items may already have been received. This prevents repeated
free grants when a delivery's outcome is uncertain.

## Optional clean test reset

To clear test setup, stop the server and back up the character database if you
want to retain its current state. Manually import
[`reset_battleground_mode.sql`](../data/sql/manual/reset_battleground_mode.sql)
into the character database, then import the original
[`mod_battleground_mode.sql`](../data/sql/db-characters/base/mod_battleground_mode.sql)
before restarting with the module enabled.

The reset clears the core XP-disabled flag only for recorded Battleground Mode
characters, preserving their other flags. It drops this module's own table,
including saved choices and starter claims. It does not delete characters or
alter unrelated characters' XP settings.

Already-applied levels, equipment, armor proficiency, and positions remain; the
module has no record of their prior values. Fully reversing those changes needs
a suitable backup or deliberate manual character edits. Existing characters
without module records become Normal Mode; create a new level-1 character to
retest first-login selection. The reset is optional and destructive, never a
normal install/build/startup step.

## Verification and item information

[gear-catalog.csv](gear-catalog.csv) snapshots every configured class/build list.
See the individual armor-set documents for source appearances and approximate
scaling budgets. Custom item icons/right-click support still awaits the client
patch. Test new characters across brackets, both factions, each offered build,
menu Back/close/reopen, stale confirmations, full-bag retry, and save/relog.
Policy/config/database tests and module compilation do not prove in-game balance.
