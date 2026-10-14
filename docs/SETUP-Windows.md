# FelForge — Windows

A scripted path from a clean Windows machine to a running `worldserver.exe`.

> [!WARNING]
> **`tools\windows\setup.bat` has not been executed on Windows.** It was
> written against the CMake files in this repository — `dep/boost/CMakeLists.txt`,
> `cmake/macros/FindMySQL.cmake`, `cmake/macros/FindOpenSSL.cmake`,
> `cmake/platform/win/settings.cmake` — on a Linux machine, where no part of it
> can run. The logic follows what those files demand, but nothing here is
> verified on the target platform. Read what the script prints on the first
> run before letting it install anything.

---

## What the script does, and what it cannot

| Step | Automated | Notes |
|---|---|---|
| MSVC, CMake, Git, OpenSSL, MariaDB | ✅ | through `winget`, which verifies each installer against Microsoft's manifest |
| Boost 1.83 | ✅ | source from `archives.boost.io`, **SHA-256 pinned in the script**, built with `b2` into the layout CMake expects |
| `BOOST_ROOT` environment variable | ✅ | set for your user account |
| CMake configure and build | ✅ | |
| Collecting servers, configs and runtime DLLs | ✅ | into `dist-windows\` |
| **Databases** | ❌ | LegionCore 2020-04-25 set from the Legion Preservation Project, see below |
| **ClientData** (dbc, gt, cameras, maps, vmaps, mmaps) | ❌ | Blizzard's; must be extracted from a client you own |

The last two are the reason there is no single-click installer. Everything up
to them can be scripted; those two cannot be, legally or practically.

---

## Requirements

- Windows 10 1809 or newer, 64-bit — the script uses only what Windows brings along:
  `curl` and `tar` (from 1803), `certutil` for the checksum, `winget` (from 1809)
- ~30 GB free disk — Boost, the Build Tools and the object files are large
- A World of Warcraft **7.3.5** client, if you want a playable realm

---

## Run it

Open a **command prompt as Administrator**:

```bat
cd <repository>
tools\windows\setup.bat all
```

Stages can be repeated individually — useful, because Boost takes the longest
and rarely needs redoing:

```bat
tools\windows\setup.bat prereq      :: toolchain via winget
tools\windows\setup.bat boost       :: download, verify, build
tools\windows\setup.bat configure   :: cmake
tools\windows\setup.bat build       :: compile
tools\windows\setup.bat package     :: collect into dist-windows\
```

Settings live at the top of the script, as plain `set` lines — edit them there:

| Variable | Default | |
|---|---|---|
| `CONFIG` | `Release` | `Release`, `RelWithDebInfo` or `Debug` |
| `GENERATOR` | `Visual Studio 17 2022` | set to `Visual Studio 16 2019` for VS2019 |
| `BOOST_VERSION` / `BOOST_SHA256` | `1.83.0` | change both together, never one alone |
| `BOOST_TOOLSETDIR` | `lib64-msvc-14.3` | must match what `dep/boost/CMakeLists.txt` looks for |

Paths (`.winbuild`, `build-windows`, `dist-windows`) are derived from the
script's own location, so it works from any checkout.

---

## Why Boost is built from source

`dep/boost/CMakeLists.txt` insists on it:

```cmake
if(DEFINED ENV{BOOST_ROOT})
  ...
  list(APPEND BOOST_LIBRARYDIR ${BOOST_ROOT}/lib${PLATFORM}-msvc-14.2 ...)
else()
  message(FATAL_ERROR "No BOOST_ROOT environment variable could be found!")
endif()
set(Boost_USE_STATIC_LIBS ON)
```

So it needs `BOOST_ROOT` set, static libraries, and a directory named
`lib64-msvc-<toolset>`. The script builds exactly the six libraries the project
asks for — system, filesystem, thread, program_options, iostreams, regex —
static, multithreaded, against the dynamic runtime, and moves `b2`'s output
into that directory name.

The prebuilt Boost binaries floating around are compiled for one specific MSVC
toolset; building from a checksummed source archive works with whatever
compiler you actually have.

---

## After the build

`dist-windows\` will hold the servers, `worldserver.conf`, `bnetserver.conf`,
the runtime DLLs and an empty `ClientData\`. Two things are still missing.

### 1. Databases

Four are needed: `auth`, `characters`, `world`, `hotfixes`.

```sql
CREATE USER 'trinity'@'127.0.0.1' IDENTIFIED BY 'trinity';
GRANT ALL PRIVILEGES ON *.* TO 'trinity'@'127.0.0.1' WITH GRANT OPTION;

CREATE DATABASE auth       DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE characters DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE world      DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE hotfixes   DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
```

Download the LegionCore 7.3.5 databases of 2020-04-25 — TrinityCore's 7.3.5
databases do not fit this core. Where to get them and why exactly these is
in [SETUP.md, section 3](SETUP.md#3-databases). Then import, in this order:

```bat
set M=mysql -h127.0.0.1 -utrinity -ptrinity
%M% auth       < auth_database.sql
%M% auth       < sql\legioncore\auth_update.sql
%M% auth       < sql\base\auth_playerbot.sql
%M% characters < characters_database.sql
%M% world      < LegionCore_world_735.26972_2020_04_25.sql
%M% world      < sql\base\world_playerbot.sql
%M% hotfixes   < LegionCore_hotfixes_735.26972_2020_04_03.sql
%M% hotfixes   < sql\legioncore\hotfixes_update.sql
```

Then change the password of the preset administrator account `GM@GM`; see
the warning in SETUP.md.

### 2. ClientData

The map extractor in this source tree is outdated (it writes `v1.8` maps,
the core reads `v1.9`, and it does not extract `dbc`, `gt` or `cameras`).
Build the extractors of the Legion Preservation Project's current
LegionCore-7.3.5 with `-DTOOLS=1` and run them against your 7.3.5 client:

```bat
mapextractor.exe
vmap4extractor.exe
vmap4assembler.exe Buildings vmaps
mmaps_generator.exe          :: hours
```

Copy `dbc`, `gt`, `cameras`, `maps`, `vmaps` and `mmaps` into
`dist-windows\ClientData`.

### 3. Check before starting

```bat
mysql -h127.0.0.1 -utrinity -ptrinity -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='world'; SELECT COUNT(*) FROM world.creature_template; SELECT COUNT(*) FROM auth.realmlist;"

dir dist-windows\ClientData\gt
dir dist-windows\ClientData\maps
dir dist-windows\ClientData\dbc
```

About 280 tables in `world`, not four. If you see four, only the playerbot
tables were applied and the world database is missing.

---

## When it goes wrong

| Symptom | Cause |
|---|---|
| `No BOOST_ROOT environment variable could be found` | the `boost` stage did not finish, or the window predates `setx` — open a new command prompt |
| CMake finds no compiler | Build Tools installed without the C++ workload. Visual Studio Installer → *Desktop development with C++* |
| `MySQL wasn't found on your system` | MariaDB elsewhere than Program Files — pass `-DMYSQL_ADD_INCLUDE_PATH` and `-DMYSQL_ADD_LIBRARY_PATH` |
| `Table 'world.linked_respawn' doesn't exist` and dozens like it | the world database was never imported — see above |
| winget reports an unknown package id | ids change; `winget search <name>` and adjust `PKGS` in the script |
| `bootstrap.bat` fails | run from a *Developer Command Prompt for VS 2022*, so the MSVC environment is loaded |

---

## Portability of the result

The binaries link against the OpenSSL and MariaDB DLLs that the `package` stage copies
next to them, plus the Visual C++ runtime. On a machine without that runtime,
install the *Microsoft Visual C++ Redistributable* — or build with
`/MT` if you would rather not ship it.
