# Contributing to FelForge

Thanks for helping. FelForge aims to be the best Legion server emulator, and
that only works if every change can be checked. This page says how.

*Beiträge, Fehlerberichte und Diskussionen auf Deutsch sind genauso willkommen.*

---

## Building

The full walkthrough is in [docs/SETUP.md](docs/SETUP.md). Short form on
Debian or Ubuntu:

```sh
sudo apt-get install -y build-essential cmake ninja-build \
    libboost-all-dev default-libmysqlclient-dev libssl-dev \
    zlib1g-dev libbz2-dev libreadline-dev
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_WARNINGS=1 -G Ninja
ninja
```

A full build takes a while. After the first one, Ninja only rebuilds what
your change touches; a change to a widely included header such as
`PlayerBotMgr.h` still recompiles a few dozen files.

## Before you open a pull request

- **Build what you touched**, with `-DWITH_WARNINGS=1`, and do not add new
  warnings.
- **Keep one topic per pull request.** A bug fix and a refactoring are two
  pull requests.
- **Say what you verified.** Built only, tested in game, tested against which
  world database — write it in the description. "Not tested at runtime" is a
  perfectly good answer; an unstated one is not.
- **Keep the source tree clean:** UTF-8, existing line endings preserved
  (`.bat` files stay CRLF), no generated files, no build output.
- **Keep copyright notices.** FelForge is GPL-2.0 and builds on TrinityCore
  and LegionCore; their headers stay where they are.
- **No Blizzard data.** Client files, extracted maps, DB2 files or anything
  else taken from the game client must not be committed.

Commit messages may be in English or German. A short subject line in the
imperative, then a body that explains *why* the change is needed.

## Reporting bugs

Use the issue templates. A useful report contains:

- the **version line** the server prints at startup or on `.server info`,
  for example `FelForge 0.1.0 (a1b2c3d4e5) (Unix, Release)`;
- the **world database** you run (name and version);
- **steps to reproduce**, what you expected and what happened;
- the relevant part of `World.log` (in `LogsDir`) or the console output.

For playerbot problems also give the bot's class and specialisation, what it
was doing (open world, group, battleground, arena, dungeon finder) and the
`PlayerBot.*` settings you changed.

## Licence

By contributing you agree that your contribution is licensed under the
GNU General Public License v2, like the rest of the project — see
[COPYING](COPYING).
