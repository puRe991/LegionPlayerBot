# LegionPlayerBot

LegionCore — a TrinityCore derivative for World of Warcraft *Legion* (7.3.5) —
with a playerbot subsystem built directly into the core rather than bolted on
as a module.

Bots are real characters on real accounts. They queue for battlegrounds and
arenas, fight with class-specific combat AI, follow group commands in chat, and
populate the open world.

**[Setup instructions](docs/SETUP.md)** — requirements, build, databases,
client data, configuration.

## Build

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Builds with GCC and Clang on Linux and with MSVC 2017+ on Windows. Boost 1.60+,
OpenSSL 1.0/1.1/3.x and MySQL 5.7+ or MariaDB 10.4+ are all supported.

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
Mistweaver healing and the Brewmaster and Vengeance tanking roles. Death Knight
is covered everywhere except battlegrounds.

Objective AI — flags, bases, vehicles — exists for Arathi Basin, Warsong Gulch,
Eye of the Storm, Alterac Valley and Isle of Conquest.

## Configuration

The `PLAYERBOT SETTINGS` block in `worldserver.conf.dist` documents every
option, including the online limit and how bots answer loot rolls.

## Database

`sql/base/` holds the custom tables the core requires. The base TrinityCore
7.3.5 content is not included and has to come from elsewhere.

## Administration

`.playerbot add`, `remove`, `limit` and `status` drive the subsystem in game.
See [the setup guide](docs/SETUP.md) for the details.

## Known limitations

- Rated arena and the dungeon finder are not wired up for bots.
- Outside the five battlegrounds listed above, bots fight but ignore
  objectives.

## Security

`src/server/bnetserver/bnetserver.key.pem` is the TrinityCore development key
and is public. Replace it before exposing a realm.

## Licence

GPL v2, inherited from TrinityCore. See [COPYING](COPYING).
