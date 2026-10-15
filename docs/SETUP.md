# FelForge — Setup

FelForge is a World of Warcraft *Legion* (7.3.5) server derived from
TrinityCore and LegionCore, with the playerbot subsystem built into the core.

---

## 1. Requirements

| Component | Version | Note |
|---|---|---|
| CMake | 3.9 or newer | |
| GCC | 9 or newer | Clang works as well |
| MySQL / MariaDB | MySQL 5.7+ or MariaDB 10.4+ | client library **and** a server |
| Boost | 1.60 or newer | 1.66+ takes the modern Asio path |
| OpenSSL | 1.0, 1.1 or 3.x | all three are handled |
| zlib, bzip2, readline | — | |

On Debian or Ubuntu:

```sh
sudo apt-get install -y build-essential cmake \
    libboost-all-dev default-libmysqlclient-dev libssl-dev \
    zlib1g-dev libbz2-dev libreadline-dev
```

Windows builds need MSVC 2017 or newer.

---

## 2. Build

> **State of the Linux port.** This tree used to be MSVC-only. The whole
> build — `common`, `shared`, `game`, `scripts`, `worldserver` and
> `bnetserver` — now completes under GCC with no errors and links without
> unresolved symbols. Verified on Ubuntu 24.04 with GCC 13.3, CMake 3.28,
> Boost 1.83, OpenSSL 3.0.13 and MariaDB 10.11. `worldserver` starts,
> reports the SSL and Boost versions it was linked against, and opens the
> auth, characters and hotfixes databases; it stops on the base
> TrinityCore world content, which this repository does not ship (see
> section 3).
>
> A full build takes roughly half an hour on eight cores and needs about
> 6 GB of disk for the objects; `libscripts.a` alone is around 200 MB.
> Build with `-DWITH_WARNINGS=1` when changing code.

The build has to happen outside the source tree; in-source builds are
rejected on purpose.

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/felforge
make -j$(nproc)
make install
```

Useful options:

| Option | Default | Effect |
|---|:--:|---|
| `-DSERVERS=0` | 1 | skip worldserver and bnetserver |
| `-DSCRIPTS=0` | 1 | build the core without scripts — does not link at the moment, the bot and BattlePay scripts are still called unconditionally |
| `-DTOOLS=1` | 0 | build the extractors as well (they need C++17, see section 4) |
| `-DWITH_WARNINGS=1` | 0 | show compiler warnings — off by default, worth turning on when changing code |
| `-DNOPCH=1` | 0 | disable precompiled headers |

---

## 3. Databases

Four databases are required: `auth`, `characters`, `world` and `hotfixes`.

The core connects over TCP, never over the unix socket. On a default
MariaDB installation `root` authenticates through `unix_socket` and will be
refused over `127.0.0.1`, so give the server a user of its own:

```sql
CREATE USER 'trinity'@'127.0.0.1' IDENTIFIED BY 'trinity';
GRANT ALL PRIVILEGES ON *.* TO 'trinity'@'127.0.0.1' WITH GRANT OPTION;
```

```sql
CREATE DATABASE auth       DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE characters DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE world      DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE hotfixes   DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
```

### Which databases

The build does not create databases, and this core has no updater that
would. It is LegionCore 7.3.5 as of 2020-04-25 and needs exactly that
schema. **TrinityCore's 7.3.5 databases do not fit:** checked against every
prepared statement of this core, 802 of 1414 fail (auth 15, characters 245,
world 17, hotfixes 525) — LegionCore added its own tables and columns
throughout.

The matching databases are published by The Legion Preservation Project,
which maintains LegionCore 7.3.5 under GPL-2.0:

| Database | Source |
|---|---|
| world, hotfixes | release [`LegionCore735.20041`](https://github.com/The-Legion-Preservation-Project/LegionCore-7.3.5/releases/tag/LegionCore735.20041), "Full world and hotfixes databases 2020-04-25" — `LegionCore_full_735.26972_2020_04_25.7z`, 33 MB, unpacks to 274 MB |
| auth, characters | `sql/base/auth_database.sql` and `characters_database.sql` at the same tag |

Use these versions. Later releases of that project belong to a newer core
whose schema has moved on.

```sh
curl -LO https://github.com/The-Legion-Preservation-Project/LegionCore-7.3.5/releases/download/LegionCore735.20041/LegionCore_full_735.26972_2020_04_25.7z
7z x LegionCore_full_735.26972_2020_04_25.7z
curl -LO https://raw.githubusercontent.com/The-Legion-Preservation-Project/LegionCore-7.3.5/LegionCore735.20041/sql/base/auth_database.sql
curl -LO https://raw.githubusercontent.com/The-Legion-Preservation-Project/LegionCore-7.3.5/LegionCore735.20041/sql/base/characters_database.sql
```

### Import

In this order — the two scripts under `sql/legioncore/` close the small
gaps between those dumps and this core, the `sql/base/` scripts add the
playerbot tables:

```sh
mysql auth       < auth_database.sql
mysql auth       < sql/legioncore/auth_update.sql
mysql auth       < sql/base/auth_playerbot.sql
mysql characters < characters_database.sql
mysql world      < LegionCore_world_735.26972_2020_04_25.sql
mysql world      < sql/base/world_playerbot.sql
mysql hotfixes   < LegionCore_hotfixes_735.26972_2020_04_03.sql
mysql hotfixes   < sql/legioncore/hotfixes_update.sql
```

This sequence was run end to end against MariaDB 10.11: every prepared
statement of the core then prepares without error, `bnetserver` starts and
lists the realm, and `worldserver` loads its databases and stops only at
the missing client data.

> [!CAUTION]
> `auth_database.sql` comes with an administrator account, `GM@GM`
> (GM level 6 on all realms), whose password hash is in a public dump.
> Delete it or give it a new password before anyone else can reach the
> server: `.account set password GM@GM <new> <new>` on the worldserver
> console.

The dump also contains one realm, named `LegionCore` at `127.0.0.1:8085`;
rename it and set its address in `auth.realmlist`.

### Playerbot tables

`sql/base/auth_playerbot.sql` and `sql/base/world_playerbot.sql` from the
import above create:

| Database | Table | Purpose |
|---|---|---|
| auth | `toolip` | allow list for the remote tool socket |
| auth | `playerbot_names` | name pool for generated bot characters |
| auth | `playerbot_arena` | name pool for generated arena teams |
| world | `aiwaypoints` | **navigation graph for the bots** |
| world | `bottalktext` | ambient chat lines |
| world | `talkstory` | scripted story lines |
| world | `playerbot_online` | bot accounts to bring online at startup |

`aiwaypoints` matters most: without waypoints the bots have no navigation
graph and fall back to passive behaviour in battlegrounds. The script fills
it with the 61 objective points (flags, bases, graveyards, starting
positions) for Warsong Gulch, Arathi Basin, Eye of the Storm, Alterac
Valley, Isle of Conquest and the Dalaran Sewers arena. The battleground
code looks these up by entry id, so keep the ids as they are. There is no
in-game command for editing waypoints.

---

## 4. Client data

`DataDir` (default `./ClientData`) has to contain six directories extracted
from a 7.3.5 client (build 26972): `dbc`, `gt`, `cameras`, `maps`, `vmaps`
and `mmaps`. They come from your own game client and are not
redistributable: neither this repository nor the database releases in
section 3 contain them.

The extractors in `src/tools` come from The Legion Preservation Project's
LegionCore-7.3.5 (commit `fed58f7`) and are adapted to this core:

- They are built against this core's collision library, so vmaps come out in
  exactly the format the worldserver reads.
- Every vmap model carries a second triangle list (`BSPX`) that this core
  uses for heights, areas and indoor checks; the first one serves line of
  sight. Without it this core's assembler writes no model file at all, which
  is why vmaps from other 7.3.5 extractors do not work here.
- Child maps without terrain of their own get their own vmap tiles, because
  this core does not fall back to the parent map's.

Build them with `-DTOOLS=1`. They need a C++17 compiler (GCC 7, Clang 5,
Visual Studio 2017 or newer); the core itself stays on C++11. Then run them
in the client directory, the one with `Data` and `.build.info` in it:

```sh
cd "/path/to/World of Warcraft"
/opt/felforge/bin/mapextractor        # dbc, gt, cameras, maps
/opt/felforge/bin/vmap4extractor      # Buildings (raw models)
/opt/felforge/bin/vmap4assembler Buildings vmaps
/opt/felforge/bin/mmaps_generator     # mmaps; reads dbc, maps and vmaps, takes hours
```

Each tool creates its output directory. `mapextractor -e` limits it to part
of the data (1 maps, 2 dbc, 4 cameras, 8 gt); `mmaps_generator <map id>`
builds a single map, which is handy for a first test. Copy the resulting
`dbc`, `gt`, `cameras`, `maps`, `vmaps` and `mmaps` directories into
`DataDir`; `Buildings` is only an intermediate step.

---

## 4a. Check before the first start

Both of the previous two steps bring in data this repository does not
contain. If either is missing the server aborts during startup, and the
first error it prints is misleading — a failed prepared statement rather
than "no world database". Confirm the state up front:

```sh
# Databases: the LegionCore 2020-04-25 set from section 3, not just the
# playerbot tables. Expect about 36, 145, 280 and 810 tables.
mysql -h127.0.0.1 -utrinity -ptrinity \
  -e "SELECT table_schema, COUNT(*) FROM information_schema.tables
        WHERE table_schema IN ('auth','characters','world','hotfixes') GROUP BY table_schema;
      SELECT COUNT(*) AS creature_template FROM world.creature_template;
      SELECT COUNT(*) AS realms FROM auth.realmlist;"

# Client data: all six directories have to exist under DataDir.
ls ClientData/dbc ClientData/gt ClientData/cameras ClientData/maps ClientData/vmaps ClientData/mmaps
```

Symptoms when a step was skipped:

| Symptom | Cause |
|---|---|
| `Table 'world.linked_respawn' doesn't exist` and dozens like it, exit code 1 | world database not imported (only the playerbot tables present) |
| `Unknown column` / `Table ... doesn't exist` for a few tables | databases from another core, e.g. TrinityCore 7.3.5, or the update scripts from section 3 not applied |
| `Some required *.txt GameTable files not found`, exit code 1 | `gt` missing in `DataDir` |
| `Some required *.db2 files not found` / missing map files | `DataDir` empty or incomplete |
| Client reaches the login screen but shows no realm | `auth.realmlist` empty |

---

## 5. Configuration

Copy the templates and adjust them:

```sh
cp worldserver.conf.dist worldserver.conf
cp bnetserver.conf.dist  bnetserver.conf
```

At minimum set the four `*DatabaseInfo` lines and `DataDir`.

### Playerbots

The `PLAYERBOT SETTINGS` block in `worldserver.conf.dist` documents every
option. The ones that matter:

| Key | Default | Meaning |
|---|:--:|---|
| `PlayerBot.Enable` | 1 | master switch |
| `PlayerBot.Accounts` | 180 | number of bot accounts |
| `PlayerBot.MaxOnline` | 10 | how many bots may be online at once; real players do not count |
| `PlayerBot.MinOnline` | 0 | how many bots are kept online even when nothing needs them; 0 = only on demand |
| `PlayerBot.Announce` | 1 | announce in chat when a bot comes online or goes offline; 0 = silent |
| `PlayerBot.AccountAll` | 1 | allow account-bound bots (group and friend list features) |
| `PlayerBot.CreateCharacters.Accounts` | 20 | bot accounts that get one character of every class created automatically; 0 = off |
| `PlayerBot.Loot.NeedRoll` | 2 | 2 = need on suitable upgrades, greed otherwise; 1 = never need; 0 = always pass |
| `PlayerBot.Loot.MinQuality` | 2 | lowest quality a bot rolls need on |

Bots are brought online by the manager itself: it fills battlegrounds,
arenas and the dungeon finder once a real player queues, at most one bot per
update tick. With `PlayerBot.MinOnline` above 0 it also keeps that many bots
online at all times, half alliance and half horde; they roam the open world
and are the first ones sent into a queue. Every bot that is online at the
same time needs its own bot account with characters, so keep
`PlayerBot.CreateCharacters.Accounts` at least as high as
`PlayerBot.MaxOnline`.

To drive them by hand, `.playerbot` (short form `.pbot`):

| Command | Level | Effect |
|---|---|---|
| `.playerbot add <alliance\|horde> [class] [count]` | Game Master | Queue bots. Class by name (`monk`, `dh`, …) or id, omitted means any. |
| `.playerbot remove <all\|account id>` | Game Master | Log bots out. |
| `.playerbot limit [count]` | Administrator | Read or set the online limit for this session. |
| `.playerbot status` | Game Master | Online counts and the current loot settings. |

### Bot characters

On the first start the server creates `PlayerBot.Accounts` bot accounts
(default 180) named `PLAYERBOT<number>`, all with the password `botxxx`. The numbers follow the
account ids, so they need not start at 1 or be contiguous:

```sql
SELECT id, username FROM auth.account WHERE username LIKE 'PLAYERBOT%';
```

A bot can only come online as a character that exists on its account. The
server creates those itself: the first `PlayerBot.CreateCharacters.Accounts`
bot accounts (default 20) each get one character of every class, all of one
faction, split evenly between alliance and horde. Creation starts once the
world is up and runs in the background, one character every two seconds —
with the defaults 240 characters, so about eight minutes. It goes account
by account, and an account only comes online once all of its characters
exist: the first one is ready after about 25 seconds, and another follows
every 25 seconds. If an account is online anyway when its turn comes (a GM
logged it in, or it was online before the restart), its characters wait
until it goes offline. Characters that are still missing are queued again on
the next start.

Names come from `auth.playerbot_names` first; once that pool is used up the
server makes up names from syllables. Every name is checked against the same
rules as a name typed by a player. A creation that fails anyway shows up in
the log as `PlayerBot: creating character ... failed`.

Level 1 characters are enough: before a battleground or skirmish the bot is
levelled and equipped for the bracket. Rated arena only picks bots that are
already at the maximum level.

Characters can still be created by hand: log in with the client as a bot
account and the password `botxxx`. Restart the worldserver afterwards; the
bot list of hand-made characters is only read at startup.

Check what the server has:

```sql
SELECT a.username, c.name, c.race, c.class, c.level
FROM auth.account a JOIN characters.characters c ON c.account = a.id
WHERE a.username LIKE 'PLAYERBOT%';
```

### Security

Two things must be changed before running this publicly:

1. **`src/server/bnetserver/bnetserver.key.pem` is a development key that
   ships with the source and is therefore public.** Replace the
   certificate and key.
2. Bot accounts are created with a fixed password. Keep the realm's
   auth database off the public network.

---

## 6. Starting

```sh
./bnetserver &
./worldserver
```

`bnetserver` handles logins, `worldserver` the game itself. Point the
client's `Config.wtf` at the bnetserver host.

---

## 7. Known limitations

Documented so nobody has to rediscover them:

- Bot objective AI exists for Arathi Basin, Warsong Gulch, Eye of the
  Storm, Alterac Valley and Isle of Conquest only. In other battlegrounds
  the bots fight but do not play the objectives.
- Parts of the source still carry mis-encoded comments and a few
  user-visible strings from the original Chinese sources.
