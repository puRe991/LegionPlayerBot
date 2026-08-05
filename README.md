# LegionPlayerBot

**A TrinityCore-derived World of Warcraft *Legion* (7.3.5) server with player bots built into the core.**

[Deutsch](README.de.md) · [Setup guide](docs/SETUP.md) · [Legion defect report](docs/Fehlerbericht-Legion.md)

---

Bots here are not puppets. They are real characters on real accounts: they log
in, pick talents, queue for battlegrounds, arenas and the dungeon finder, fight
with class-specific combat AI, roll on loot, answer whispers, and populate the
open world. The subsystem lives inside the core rather than sitting beside it as
a module, so a bot goes through the same session, the same packet handlers and
the same queues as a human player.

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## Status

| | |
|---|---|
| **Linux / GCC** | builds — verified on GCC 13.3, CMake 3.28, Boost 1.83, OpenSSL 3.0, MariaDB 10.11 |
| **Windows / MSVC** | builds — MSVC 2017 or newer |
| **Produces** | `worldserver`, `bnetserver`, no unresolved symbols |
| **Runs** | starts, reads its configuration, opens the auth, characters and hotfixes databases |
| **Needs** | a TrinityCore 7.3.5 world database and extracted client data — **neither ships here** |

The tree used to be MSVC-only. Boost 1.60+, OpenSSL 1.0/1.1/3.x and MySQL 5.7+
or MariaDB 10.4+ are all supported; the version differences are handled in the
build system rather than pinned.

> **Before your first start**, run the checks in
> [§ 4a of the setup guide](docs/SETUP.md). Without the world database the
> server aborts with a misleading error: the first line it prints is
> `Table 'world.linked_respawn' doesn't exist`, not "no world database".

---

## Bot AI

Five AI modes, each with per-class implementations:

| Mode | Purpose |
|---|---|
| `BotFieldAI` | open world |
| `BotGroupAI` | party and dungeon |
| `BotBGAI` | battlegrounds |
| `BotArenaAI` | arena |
| `BotDuelAI` | duels |

**Class coverage.** Complete for Warrior, Paladin, Hunter, Rogue, Priest,
Shaman, Mage, Warlock and Druid. Monk and Demon Hunter are playable — including
the Mistweaver healing role and the Brewmaster and Vengeance tanking roles —
though their rotations are shallower than the older classes'. Death Knight is
covered everywhere except battlegrounds.

Bots choose their own talents, one per unlocked tier, matched to the active
specialisation.

### Queueing

| Queue | Bot support |
|---|---|
| Battlegrounds | bots fill the queue when a real player is waiting |
| Skirmish arena | a single bot queues on its own |
| Rated arena | the bot manager forms a group of 2 or 3; the leader queues |
| Dungeon finder | bots fill the role the queue is short of and accept the group proposal automatically |

Rated arena in Legion has no arena teams — it is queued by the leader of an
ordinary group of the right size. `PlayerBotMgr::AddTeamBotToRatedArena` builds
that group from idle max-level bots of one faction.

The dungeon finder works from the other end.
`LFGMgr::SearchLFGBotRequirement` scans the queue for a **real** player who is
waiting and reports which role is still missing; the bot manager then brings a
matching character online. Bot sessions are skipped in that scan on purpose —
otherwise bots would queue for each other and the queue would never drain.
Since a bot has no client to click "accept", `SendLfgUpdateProposal` enqueues
the answer itself.

### Battleground objectives

Objective AI — flags, bases, vehicles — is implemented for **Arathi Basin,
Warsong Gulch, Eye of the Storm, Alterac Valley and Isle of Conquest**. In the
remaining battlegrounds bots fight but ignore objectives.

**That code needs navigation data this repository does not ship.** The
`aiwaypoints` table is created empty. Bots move around a battleground along a
waypoint graph that an operator builds in-game and that is written back to the
world database. Until those rows exist, objective AI has nothing to walk along —
in the five battlegrounds above as much as anywhere else.

---

## Loot

On a group loot roll a bot rolls **need** when the item suits its class *and*
current specialisation, it can actually equip it, and it beats what occupies
that slot — the same check the bot's own gearing uses. Otherwise it greeds, or
passes when the roll allows nothing else. The behaviour and the quality floor
are configurable.

---

## Administration

`.playerbot` (short `.pbot`), gated at Game Master:

| Command | Effect |
|---|---|
| `add <alliance\|horde> [class] [count]` | Queue bots. Class by name or id; omitted means any. |
| `remove <all\|account id>` | Log bots out. |
| `limit [count]` | Read or set the online limit for this session (Administrator). |
| `status` | Online counts and current loot settings. |

The `PLAYERBOT SETTINGS` block in `worldserver.conf.dist` documents every
option: the master switch, the online limit, account-bound bots, and how bots
answer loot rolls.

---

## Database

`sql/base/` holds the custom tables the core requires — the bot navigation
graph, name pools, chat lines and the tool socket allow list. The base
TrinityCore 7.3.5 content is not included and has to come from elsewhere.
Import it **before** applying `sql/base`.

---

## Content

Beyond the bot subsystem this tree carries the TrinityCore script set for
Classic through Legion, audited expansion by expansion. Notable repairs:

| Expansion | Work |
|---|---|
| Classic | Buru the Gorger, Ossirian the Unscarred and Viscidus written from empty shells; Ragefire Chasm, the Stockade and Dire Maul bound as instances |
| Burning Crusade | four missing instance scripts; the Karazhan chess event; the Sunwell epilogue |
| Wrath | the Icecrown Citadel gunship battle rebuilt against this core's transport API |
| Cataclysm | 21 disabled scripts re-enabled; the Well of Eternity restored from empty files |
| Warlords | Blackrock Foundry and Hellfire Citadel encounters implemented; Grimrail Depot added |
| Legion | see the [defect report](docs/Fehlerbericht-Legion.md) |

Registration integrity is checked mechanically: every script class in the tree
is bound, there are no duplicate script names, no empty source files, and every
`.cpp` reaches the build.

---

## Encoding

The whole source tree is UTF-8. It did not start that way — 26 files of the bot
subsystem were GBK and a handful were cp1251. Player-visible strings that used
to be Chinese literals went through `consoleToUtf8`, which is a no-op on Linux
and would have put raw bytes on the wire; they are plain English UTF-8 strings
now.

---

## Security

`src/server/bnetserver/bnetserver.key.pem` is the TrinityCore development key
and is therefore public. **Replace it before exposing a realm.** Bot accounts
are created with a fixed password, so keep the auth database off public
networks.

---

## Known limitations

- Outside the five battlegrounds listed above, bots fight but ignore
  objectives.
- All battleground objective AI stays inert until an operator builds the
  `aiwaypoints` navigation graph.
- Artifact acquisition scenarios exist for seven of twelve classes.
- Some comments in the bot subsystem are still Chinese. They are correctly
  encoded and readable — they simply have not been translated.

---

## Licence

GPL v2, inherited from TrinityCore. See [COPYING](COPYING).
