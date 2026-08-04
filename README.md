# LegionPlayerBot

LegionCore — a TrinityCore derivative for World of Warcraft *Legion* (7.3.5) —
with a playerbot subsystem built directly into the core rather than bolted on
as a module.

Bots are real characters on real accounts. They queue for battlegrounds and
arenas, fight with class-specific combat AI, take part in group loot, follow
commands in chat, and populate the open world.

**[Setup instructions](docs/SETUP.md)** — requirements, build, databases,
client data, configuration.

## Build

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Boost 1.60+, OpenSSL 1.0/1.1/3.x and MySQL 5.7+ or MariaDB 10.4+ are all
supported; the version differences are handled in the build system rather than
pinned. Windows builds need MSVC 2017 or newer.

The tree was MSVC-only and is being ported to GCC and Clang. `common` and
`shared` build cleanly; the port of `game` is essentially finished but the
library has not linked end to end yet, and `scripts` and the two server
binaries have not been through a compiler other than MSVC. See
[docs/SETUP.md](docs/SETUP.md) for what that means in practice.

## Bot AI

Five separate AI modes, each with per-class implementations:

| Mode | Purpose |
|---|---|
| `BotBGAI` | battlegrounds |
| `BotFieldAI` | open world |
| `BotGroupAI` | party and dungeon |
| `BotDuelAI` | duels |
| `BotArenaAI` | arena |

Class coverage is complete for Warrior, Paladin, Hunter, Rogue, Priest, Shaman,
Mage, Warlock and Druid. Monk and Demon Hunter are playable, including the
Mistweaver healing role and the Brewmaster and Vengeance tanking roles, though
their combat rotations are shallower than the older classes'. Death Knight is
covered everywhere except battlegrounds.

Bots pick their talents themselves, one per unlocked tier, matched to the
active specialisation.

Objective AI — flags, bases, vehicles — exists for Arathi Basin, Warsong Gulch,
Eye of the Storm, Alterac Valley and Isle of Conquest.

## Loot

On a group loot roll a bot rolls **need** when the item suits its class *and*
current specialisation, it can actually equip it, and it beats what occupies
that slot — the same check the bot's own gearing uses. Otherwise it greeds, or
passes when the roll allows nothing else. The behaviour and the quality floor
are configurable.

## Configuration

The `PLAYERBOT SETTINGS` block in `worldserver.conf.dist` documents every
option: the master switch, the online limit, account-bound bots, and how bots
answer loot rolls.

## Administration

`.playerbot` (short `.pbot`), gated at Game Master:

| Command | Effect |
|---|---|
| `add <alliance\|horde> [class] [count]` | Queue bots. Class by name or id; omitted means any. |
| `remove <all\|account id>` | Log bots out. |
| `limit [count]` | Read or set the online limit for this session (Administrator). |
| `status` | Online counts and current loot settings. |

## Database

`sql/base/` holds the custom tables the core requires — the bot navigation
graph, name pools, chat lines and the tool socket allow list. The base
TrinityCore 7.3.5 content is not included and has to come from elsewhere.

## Known limitations

- Rated arena and the dungeon finder are not wired up for bots.
- Outside the five battlegrounds listed above, bots fight but ignore
  objectives.
- Parts of the source still carry mis-encoded comments and a few
  user-visible strings from the original Chinese sources.

## Security

`src/server/bnetserver/bnetserver.key.pem` is the TrinityCore development key
and is therefore public. Replace it before exposing a realm. Bot accounts are
created with a fixed password, so keep the auth database off public networks.

## Licence

GPL v2, inherited from TrinityCore. See [COPYING](COPYING).
