# LegionPlayerBot — Setup

LegionCore (a TrinityCore derivative for World of Warcraft *Legion*, 7.3.5)
with the playerbot subsystem built into the core.

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

The build has to happen outside the source tree; in-source builds are
rejected on purpose.

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/legioncore
make -j$(nproc)
make install
```

Useful options:

| Option | Default | Effect |
|---|:--:|---|
| `-DSERVERS=0` | 1 | skip worldserver and bnetserver |
| `-DSCRIPTS=0` | 1 | build the core without scripts (much faster) |
| `-DTOOLS=1` | 0 | build the extractors as well |
| `-DWITH_WARNINGS=1` | 0 | show compiler warnings — off by default, worth turning on when changing code |
| `-DNOPCH=1` | 0 | disable precompiled headers |

---

## 3. Databases

Four databases are required: `auth`, `characters`, `world` and `hotfixes`.

```sql
CREATE DATABASE auth       DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE characters DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE world      DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE hotfixes   DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
```

Import the base content first — this repository does not ship it. Use a
TrinityCore 7.3.5 database set matching this core.

Then apply the playerbot tables, which the core needs and which **are**
shipped here:

```sh
mysql auth  < sql/base/auth_playerbot.sql
mysql world < sql/base/world_playerbot.sql
```

These create:

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
graph and fall back to passive behaviour in battlegrounds.

---

## 4. Client data

`DataDir` (default `./ClientData`) has to contain the maps, vmaps, mmaps,
dbc and cameras extracted from a 7.3.5 client. Build the extractors with
`-DTOOLS=1` and run them against the client directory:

```sh
./mapextractor          # maps + dbc
./vmap4extractor        # vmaps (raw)
./vmap4assembler Buildings vmaps
./mmaps_generator       # mmaps — takes hours
```

Copy the resulting `maps`, `vmaps`, `mmaps`, `dbc` and `cameras`
directories into `DataDir`.

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
| `PlayerBot.MaxOnline` | 10 | how many bots may be online at once |
| `PlayerBot.AccountAll` | 1 | allow account-bound bots (group and friend list features) |
| `PlayerBot.Loot.NeedRoll` | 2 | 2 = need on suitable upgrades, greed otherwise; 1 = never need; 0 = always pass |
| `PlayerBot.Loot.MinQuality` | 2 | lowest quality a bot rolls need on |

Bots are brought online by the manager itself: it fills battlegrounds and
non-rated arenas once a real player queues, at most one bot per update
tick.

To drive them by hand, `.playerbot` (short form `.pbot`):

| Command | Level | Effect |
|---|---|---|
| `.playerbot add <alliance\|horde> [class] [count]` | Game Master | Queue bots. Class by name (`monk`, `dh`, …) or id, omitted means any. |
| `.playerbot remove <all\|account id>` | Game Master | Log bots out. |
| `.playerbot limit [count]` | Administrator | Read or set the online limit for this session. |
| `.playerbot status` | Game Master | Online counts and the current loot settings. |

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
- Rated arena and the dungeon finder are not wired up for bots.
