<div align="center">

# LegionPlayerBot

**A TrinityCore-derived World of Warcraft *Legion* (7.3.5) server
with player bots built into the core.**

[![Licence](https://img.shields.io/badge/licence-GPL--2.0-blue.svg)](COPYING)
[![Expansion](https://img.shields.io/badge/WoW-Legion%207.3.5-orange.svg)](#)
[![Build](https://img.shields.io/badge/build-GCC%2013.3%20%7C%20MSVC%202017%2B-brightgreen.svg)](#building)
[![Language](https://img.shields.io/badge/C%2B%2B-14-00599C.svg)](#)
[![Encoding](https://img.shields.io/badge/source-100%25%20UTF--8-success.svg)](#source-encoding)

**English** · [Deutsch](README.de.md)

[Setup](docs/SETUP.md) · [Windows](docs/SETUP-Windows.md) · [Legion defect report](docs/Fehlerbericht-Legion.md) · [Licence](COPYING)

</div>

---

## Contents

- [What this is](#what-this-is)
- [Building](#building)
- [Status at a glance](#status-at-a-glance)
- [Bot AI](#bot-ai)
  - [Class coverage](#class-coverage)
  - [Queueing](#queueing)
  - [Battleground objectives](#battleground-objectives)
  - [Loot](#loot)
- [Administration](#administration)
- [Databases and client data](#databases-and-client-data)
- [Content and repairs](#content-and-repairs)
- [Source encoding](#source-encoding)
- [Security](#security)
- [Known limitations](#known-limitations)
- [Credits and licence](#credits-and-licence)

---

## What this is

Bots here are not puppets. They are **real characters on real accounts**: they
log in, pick talents, queue for battlegrounds, arenas and the dungeon finder,
fight with class-specific combat AI, roll on loot, answer whispers, and populate
the open world.

The subsystem lives **inside** the core rather than beside it as a module, so a
bot goes through the same session, the same packet handlers and the same queues
as a human player. There is no scripting bridge and no second process to keep in
sync.

| | |
|---|---|
| Bot AI | ~42 000 lines across 81 files, five AI modes |
| Game scripts | 1 127 script files, Classic through Legion |
| Total source | ~1.38 M lines |

---

## Building

```sh
git clone https://github.com/puRe991/LegionPlayerBot.git
cd LegionPlayerBot
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Toolchain.** Boost 1.60+, OpenSSL 1.0/1.1/3.x and MySQL 5.7+ or MariaDB 10.4+
are all supported; the version differences are resolved in the build system
rather than pinned. Windows needs MSVC 2017 or newer.

**On Windows** there is a scripted path — `tools\windows\setup.bat` fetches the
toolchain through winget, builds Boost from a checksummed source archive into
the layout CMake expects, configures, compiles and collects the result. Plain
batch, using only what Windows ships: `curl`, `certutil`, `tar`, `winget`. It has
**not been executed on Windows**; see [docs/SETUP-Windows.md](docs/SETUP-Windows.md).

The tree used to be MSVC-only. It now builds end to end on Linux with GCC.

> [!IMPORTANT]
> **Before your first start**, run the checks in
> [§ 4a of the setup guide](docs/SETUP.md). Without the world database the
> server aborts with a misleading error — the first line it prints is
> `Table 'world.linked_respawn' doesn't exist`, not "no world database".

---

## Status at a glance

| | |
|---|---|
| **Linux / GCC** | ✅ builds — verified on GCC 13.3, CMake 3.28, Boost 1.83, OpenSSL 3.0, MariaDB 10.11 |
| **Windows / MSVC** | ✅ builds — MSVC 2017 or newer |
| **Artefacts** | `worldserver`, `bnetserver` — no unresolved symbols |
| **Startup** | starts, reads its configuration, opens the auth, characters and hotfixes databases |
| **Not included** | a TrinityCore 7.3.5 world database and extracted client data |

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

Bots choose their own talents, one per unlocked tier, matched to the active
specialisation.

### Class coverage

| Class | Coverage |
|---|---|
| Warrior, Paladin, Hunter, Rogue, Priest, Shaman, Mage, Warlock, Druid | complete |
| Monk, Demon Hunter | playable — Mistweaver healing, Brewmaster and Vengeance tanking; rotations shallower than the older classes' |
| Death Knight | complete except battlegrounds |

### Queueing

| Queue | Bot support |
|---|---|
| Battlegrounds | bots fill the queue when a real player is waiting |
| Skirmish arena | a single bot queues on its own |
| Rated arena | the bot manager forms a group of 2 or 3; the leader queues |
| Dungeon finder | bots fill the role the queue is short of and accept the group proposal automatically |

**Rated arena.** Legion has no arena teams — rated is queued by the leader of an
ordinary group of the right size. `PlayerBotMgr::AddTeamBotToRatedArena` builds
that group from idle max-level bots of one faction.

**Dungeon finder.** This works from the other end.
`LFGMgr::SearchLFGBotRequirement` scans the queue for a *real* player who is
waiting and reports which role is still missing; the bot manager then brings a
matching character online. Bot sessions are skipped in that scan on purpose —
otherwise bots would queue for each other and the queue would never drain. Since
a bot has no client to click "accept", `SendLfgUpdateProposal` enqueues the
answer itself.

### Battleground objectives

Objective AI — flags, bases, vehicles — is implemented for **Arathi Basin,
Warsong Gulch, Eye of the Storm, Alterac Valley and Isle of Conquest**. In the
remaining battlegrounds bots fight but ignore objectives.

> [!WARNING]
> **That code needs navigation data this repository does not ship.** The
> `aiwaypoints` table is created empty. Bots move around a battleground along a
> waypoint graph that an operator builds in-game and that is written back to the
> world database. Until those rows exist, objective AI has nothing to walk
> along — in the five battlegrounds above as much as anywhere else.

### Loot

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

## Databases and client data

Four databases are required: `auth`, `characters`, `world`, `hotfixes`.

1. **Import a TrinityCore 7.3.5 world database.** Not included here.
2. **Then apply `sql/base/`** — the custom tables this core needs: the bot
   navigation graph, name pools, chat lines and the tool socket allow list.
3. **Extract client data** into `DataDir` with `mapextractor`,
   `vmap4extractor`/`vmap4assembler` and `mmaps_generator`.
4. **Populate `auth.realmlist`.**

Order matters — apply `sql/base` *after* the world database, not before. Full
walkthrough in [docs/SETUP.md](docs/SETUP.md).

---

## Content and repairs

Beyond the bot subsystem this tree carries the TrinityCore script set for
Classic through Legion, audited expansion by expansion.

| Expansion | Work |
|---|---|
| **Classic** | Buru the Gorger, Ossirian the Unscarred and Viscidus written from empty shells; Ragefire Chasm, the Stockade and Dire Maul bound as instances |
| **Burning Crusade** | four missing instance scripts; the Karazhan chess event; the Sunwell epilogue |
| **Wrath** | the Icecrown Citadel gunship battle rebuilt against this core's transport API |
| **Cataclysm** | 21 disabled scripts re-enabled; the Well of Eternity restored from empty files |
| **Warlords** | Blackrock Foundry and Hellfire Citadel encounters implemented; Grimrail Depot added |
| **Legion** | see the [defect report](docs/Fehlerbericht-Legion.md) |

Registration integrity is checked mechanically. As of the current tree:

- every script class is bound — **0** defined but never registered
- **0** duplicate script names
- **0** empty source files
- every `.cpp` reaches the build
- **0** `Register()` bodies without an active handler
- **0** unmarked switch fall-throughs

---

## Source encoding

The whole source tree is UTF-8. It did not start that way — 26 files of the bot
subsystem were GBK and a handful were cp1251.

Player-visible strings that used to be Chinese literals went through
`consoleToUtf8`, which is a no-op on Linux and would have put raw bytes on the
wire; they are plain English UTF-8 strings now.

---

## Security

> [!CAUTION]
> `src/server/bnetserver/bnetserver.key.pem` is the TrinityCore **development
> key** and is therefore public. Replace it before exposing a realm.

Bot accounts are created with a fixed password — keep the auth database off
public networks.

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

## Credits and licence

Built on [TrinityCore](https://www.trinitycore.org/) and its LegionCore
derivative. Licensed under **GPL v2**, inherited from TrinityCore — see
[COPYING](COPYING).
