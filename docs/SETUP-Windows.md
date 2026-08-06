# LegionPlayerBot — Windows

A scripted path from a clean Windows machine to a running `worldserver.exe`.

> [!WARNING]
> **`tools\windows\Setup.ps1` has not been executed on Windows.** It was
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
| **World database** | ❌ | third-party content, not downloadable from here |
| **ClientData** (maps, vmaps, mmaps, dbc) | ❌ | Blizzard's; must be extracted from a client you own |

The last two are the reason there is no single-click installer. Everything up
to them can be scripted; those two cannot be, legally or practically.

---

## Requirements

- Windows 10 1809 or newer, 64-bit (`winget` and `tar` both ship from that version)
- ~30 GB free disk — Boost, the Build Tools and the object files are large
- A World of Warcraft **7.3.5** client, if you want a playable realm

---

## Run it

Open PowerShell **as Administrator**:

```powershell
cd <repository>
powershell -ExecutionPolicy Bypass -File tools\windows\Setup.ps1 -All
```

Stages can be repeated individually — useful, because Boost takes the longest
and rarely needs redoing:

```powershell
.\tools\windows\Setup.ps1 -Prerequisites   # toolchain via winget
.\tools\windows\Setup.ps1 -Boost           # download, verify, build
.\tools\windows\Setup.ps1 -Configure       # cmake
.\tools\windows\Setup.ps1 -Build           # compile
.\tools\windows\Setup.ps1 -Package         # collect into dist-windows\
```

Useful switches:

| Switch | Default | |
|---|---|---|
| `-Config` | `Release` | `Release`, `RelWithDebInfo` or `Debug` |
| `-Generator` | `Visual Studio 17 2022` | set to `Visual Studio 16 2019` for VS2019 |
| `-BoostVersion` | `1.83.0` | any version whose SHA-256 you add to the script |
| `-BuildDir` | `build-windows` | |
| `-WorkRoot` | `.winbuild` | downloads and the Boost tree land here |

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

Import a **TrinityCore 7.3.5 world database first**, then apply this
repository's tables:

```powershell
mysql -h127.0.0.1 -utrinity -ptrinity auth  < sql\base\auth_playerbot.sql
mysql -h127.0.0.1 -utrinity -ptrinity world < sql\base\world_playerbot.sql
```

Order matters — `sql\base` after the world database, never before.

### 2. ClientData

Build the extractors from this source tree with `-DTOOLS=1` and run them
against your 7.3.5 client:

```powershell
.\mapextractor.exe
.\vmap4extractor.exe
.\vmap4assembler.exe Buildings vmaps
.\mmaps_generator.exe          # hours
```

Copy `maps`, `vmaps`, `mmaps`, `dbc` and `cameras` into
`dist-windows\ClientData`.

### 3. Check before starting

```powershell
mysql -h127.0.0.1 -utrinity -ptrinity -e `
  "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='world';
   SELECT COUNT(*) FROM world.creature_template;
   SELECT COUNT(*) FROM auth.realmlist;"

dir dist-windows\ClientData\maps, dist-windows\ClientData\dbc
```

Several hundred tables in `world`, not four. If you see four, only the
playerbot tables were applied and the world database is missing.

---

## When it goes wrong

| Symptom | Cause |
|---|---|
| `No BOOST_ROOT environment variable could be found` | `-Boost` did not finish, or the shell predates it — open a new PowerShell |
| CMake finds no compiler | Build Tools installed without the C++ workload. Visual Studio Installer → *Desktop development with C++* |
| `MySQL wasn't found on your system` | MariaDB elsewhere than Program Files — pass `-DMYSQL_ADD_INCLUDE_PATH` and `-DMYSQL_ADD_LIBRARY_PATH` |
| `Table 'world.linked_respawn' doesn't exist` and dozens like it | the world database was never imported — see above |
| winget reports an unknown package id | ids change; `winget search <name>` and adjust the table in the script |
| `bootstrap.bat` fails | run from a *Developer PowerShell for VS 2022*, so the MSVC environment is loaded |

---

## Portability of the result

The binaries link against the OpenSSL and MariaDB DLLs that `-Package` copies
next to them, plus the Visual C++ runtime. On a machine without that runtime,
install the *Microsoft Visual C++ Redistributable* — or build with
`/MT` if you would rather not ship it.
