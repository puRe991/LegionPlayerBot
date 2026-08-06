<#
.SYNOPSIS
    Sets up a Windows build of LegionPlayerBot: fetches the dependencies, puts
    them where CMake expects them, configures and builds.

.DESCRIPTION
    Runs in stages so a failed step can be repeated without redoing the rest:

        -Prerequisites  install the toolchain through winget
        -Boost          download the Boost source, verify it, build the MSVC
                        libraries into the layout dep/boost/CMakeLists.txt looks for
        -Configure      run CMake
        -Build          compile
        -Package        collect the servers and the configuration files
        -All            all of the above, in order

    Dependency sources
    ------------------
    The toolchain comes from winget, which verifies the installer hash against
    Microsoft's manifest before running it. This script does not carry its own
    hashes for those; winget already does that job, and hard-coding hashes for
    packages that get revised would only rot.

    Boost is the exception: it is fetched straight from archives.boost.io and
    verified against a SHA-256 that is pinned below. That hash came from the
    official .json sidecar next to the archive.

.NOTES
    NOT TESTED ON WINDOWS. This script was written against the CMake files in
    this repository (cmake/macros/FindMySQL.cmake, FindOpenSSL.cmake,
    dep/boost/CMakeLists.txt) on a Linux machine, where no part of it can be
    executed. Treat the first run as a dry run and read what it reports.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\windows\Setup.ps1 -All
#>

[CmdletBinding()]
param(
    [switch] $Prerequisites,
    [switch] $Boost,
    [switch] $Configure,
    [switch] $Build,
    [switch] $Package,
    [switch] $All,

    [string] $BoostVersion  = '1.83.0',
    [string] $WorkRoot      = "$PSScriptRoot\..\..\.winbuild",
    [string] $BuildDir      = "$PSScriptRoot\..\..\build-windows",
    [string] $Generator     = 'Visual Studio 17 2022',
    [ValidateSet('Release','RelWithDebInfo','Debug')]
    [string] $Config        = 'Release'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Pinned; taken from https://archives.boost.io/release/1.83.0/source/boost_1_83_0.tar.gz.json
$BoostSha256 = @{
    '1.83.0' = 'c0685b68dd44cc46574cce86c4e17c0f611b15e195be9848dfd0769a0a207628'
}

$RepoRoot = (Resolve-Path "$PSScriptRoot\..\..").Path

function Write-Step  ($m) { Write-Host "`n=== $m" -ForegroundColor Cyan }
function Write-Note  ($m) { Write-Host "    $m" -ForegroundColor DarkGray }
function Write-Warn2 ($m) { Write-Host "    $m" -ForegroundColor Yellow }
function Fail        ($m) { Write-Host "`n!!! $m" -ForegroundColor Red; exit 1 }

function Test-Admin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    (New-Object Security.Principal.WindowsPrincipal $id).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
}

# --------------------------------------------------------------------------
# Stage: prerequisites
# --------------------------------------------------------------------------
function Install-Prerequisites {
    Write-Step 'Toolchain'

    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
        Fail @'
winget was not found. It ships with the App Installer on Windows 10 1809+
and Windows 11. Install it from the Microsoft Store ("App Installer"), or
install the four packages by hand:
  Visual Studio 2022 Build Tools (with the C++ workload), CMake, Git, MariaDB.
'@
    }

    if (-not (Test-Admin)) {
        Write-Warn2 'Not running elevated. winget will prompt per package, or fail on machine-wide installs.'
    }

    # id                                        purpose
    $packages = [ordered]@{
        'Microsoft.VisualStudio.2022.BuildTools' = 'MSVC compiler and linker'
        'Kitware.CMake'                          = 'build system'
        'Git.Git'                                = 'source control, needed for the revision stamp'
        'ShiningLight.OpenSSL.Dev'               = 'OpenSSL headers and import libraries'
        'MariaDB.Server'                         = 'database server and client library'
    }

    $missing = @()
    foreach ($id in $packages.Keys) {
        Write-Note "checking $id ($($packages[$id]))"
        # winget show returns non-zero when the id does not resolve
        & winget show --id $id --exact --disable-interactivity 1>$null 2>$null
        if ($LASTEXITCODE -ne 0) { $missing += $id }
    }

    if ($missing.Count) {
        Write-Warn2 'These package ids did not resolve in your winget sources:'
        $missing | ForEach-Object { Write-Warn2 "  $_" }
        Write-Warn2 'Package ids change over time. Find the current one with:  winget search <name>'
        Fail 'Aborting rather than installing something that was not asked for.'
    }

    foreach ($id in $packages.Keys) {
        Write-Note "installing $id"
        & winget install --id $id --exact --silent --accept-package-agreements --accept-source-agreements
        # winget returns a non-zero code for harmless outcomes too -- "already
        # installed", "no applicable update". Rather than hard-code codes that
        # differ between winget versions, report and carry on; the configure
        # stage is what actually proves whether a dependency is usable.
        if ($LASTEXITCODE -ne 0) {
            Write-Warn2 "winget returned $LASTEXITCODE for $id (often just 'already installed')"
        }
    }

    Write-Note 'The C++ workload is not selected by default in the Build Tools.'
    Write-Note 'If CMake later cannot find a compiler, run the Visual Studio Installer'
    Write-Note 'and add "Desktop development with C++".'
}

# --------------------------------------------------------------------------
# Stage: Boost
# --------------------------------------------------------------------------
function Install-Boost {
    Write-Step "Boost $BoostVersion"

    if (-not $BoostSha256.ContainsKey($BoostVersion)) {
        Fail @"
No pinned SHA-256 for Boost $BoostVersion in this script.
Fetch the official one and add it to `$BoostSha256:
  https://archives.boost.io/release/$BoostVersion/source/boost_$($BoostVersion -replace '\.','_').tar.gz.json
Downloading without a checksum is not something this script will do.
"@
    }

    $underscore = $BoostVersion -replace '\.','_'
    $archive    = "boost_$underscore.tar.gz"
    $url        = "https://archives.boost.io/release/$BoostVersion/source/$archive"
    $dlDir      = Join-Path $WorkRoot 'download'
    $srcDir     = Join-Path $WorkRoot "boost_$underscore"
    $boostRoot  = Join-Path $WorkRoot 'boost'
    $archivePath= Join-Path $dlDir $archive

    New-Item -ItemType Directory -Force -Path $dlDir | Out-Null

    if (-not (Test-Path $archivePath)) {
        Write-Note "downloading $url"
        Invoke-WebRequest -Uri $url -OutFile $archivePath -UseBasicParsing
    } else {
        Write-Note 'archive already present, reusing it'
    }

    Write-Note 'verifying SHA-256'
    $actual = (Get-FileHash -Algorithm SHA256 -Path $archivePath).Hash.ToLower()
    $expect = $BoostSha256[$BoostVersion].ToLower()
    if ($actual -ne $expect) {
        Remove-Item $archivePath -Force
        Fail "checksum mismatch`n  expected $expect`n  got      $actual`nThe download was deleted."
    }
    Write-Note "ok  $actual"

    if (-not (Test-Path $srcDir)) {
        Write-Note 'unpacking (this takes a while)'
        tar -xzf $archivePath -C $WorkRoot
        if ($LASTEXITCODE -ne 0) { Fail 'tar failed. Windows 10 1803+ ships bsdtar; otherwise unpack by hand.' }
    }

    Push-Location $srcDir
    try {
        if (-not (Test-Path '.\b2.exe')) {
            Write-Note 'bootstrapping b2'
            & .\bootstrap.bat
            if ($LASTEXITCODE -ne 0) { Fail 'bootstrap.bat failed — is the MSVC environment available?' }
        }

        # dep/boost/CMakeLists.txt expects  $BOOST_ROOT/lib64-msvc-<toolset>  and
        # sets Boost_USE_STATIC_LIBS ON, so build static, multithreaded, dynamic runtime.
        $toolsetDir = 'lib64-msvc-14.3'
        Write-Note "building the six libraries CMake asks for into $toolsetDir"
        & .\b2.exe `
            --stagedir="$boostRoot" `
            --build-dir="$(Join-Path $WorkRoot 'boost-build')" `
            --with-system --with-filesystem --with-thread `
            --with-program_options --with-iostreams --with-regex `
            address-model=64 architecture=x86 `
            link=static runtime-link=shared threading=multi `
            variant=release `
            -j $env:NUMBER_OF_PROCESSORS `
            stage
        if ($LASTEXITCODE -ne 0) { Fail 'b2 failed' }

        # b2 stages into <stagedir>\lib; the CMake glue wants lib64-msvc-<v>
        $staged = Join-Path $boostRoot 'lib'
        $wanted = Join-Path $boostRoot $toolsetDir
        if ((Test-Path $staged) -and (-not (Test-Path $wanted))) {
            Move-Item $staged $wanted
        }

        # headers next to the libraries
        $incTarget = Join-Path $boostRoot 'boost'
        if (-not (Test-Path $incTarget)) {
            Write-Note 'copying headers'
            Copy-Item -Recurse -Path (Join-Path $srcDir 'boost') -Destination $incTarget
        }
    } finally { Pop-Location }

    Write-Note "setting BOOST_ROOT for your user account -> $boostRoot"
    [Environment]::SetEnvironmentVariable('BOOST_ROOT', $boostRoot, 'User')
    $env:BOOST_ROOT = $boostRoot

    Write-Note 'dep/boost/CMakeLists.txt aborts with a fatal error when BOOST_ROOT is unset,'
    Write-Note 'so this variable is what makes the configure step work at all.'
}

# --------------------------------------------------------------------------
# Stage: configure
# --------------------------------------------------------------------------
function Invoke-Configure {
    Write-Step 'CMake configure'

    if (-not $env:BOOST_ROOT) {
        $u = [Environment]::GetEnvironmentVariable('BOOST_ROOT','User')
        if ($u) { $env:BOOST_ROOT = $u } else { Fail 'BOOST_ROOT is not set. Run -Boost first.' }
    }

    # FindMySQL.cmake searches Program Files; give it a hand if MariaDB sits elsewhere.
    $mariaCandidates = @(
        "$env:ProgramFiles\MariaDB*\include\mysql"
        "$env:ProgramFiles\MariaDB*\include"
        "${env:ProgramFiles(x86)}\MariaDB*\include\mysql"
    )
    $mysqlInc = $mariaCandidates | ForEach-Object { Get-Item $_ -ErrorAction SilentlyContinue } |
                Select-Object -First 1
    $extra = @()
    if ($mysqlInc) {
        Write-Note "MySQL headers: $($mysqlInc.FullName)"
        $extra += "-DMYSQL_ADD_INCLUDE_PATH=$($mysqlInc.FullName)"
        $lib = Get-ChildItem -Path (Join-Path $mysqlInc.FullName '..\..\lib') -Filter 'libmariadb.lib' `
               -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($lib) {
            Write-Note "MySQL library: $($lib.FullName)"
            $extra += "-DMYSQL_ADD_LIBRARY_PATH=$($lib.DirectoryName)"
        }
    } else {
        Write-Warn2 'MariaDB headers not found under Program Files.'
        Write-Warn2 'If configure fails on MySQL, pass -DMYSQL_ADD_INCLUDE_PATH and -DMYSQL_ADD_LIBRARY_PATH yourself.'
    }

    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    & cmake -S $RepoRoot -B $BuildDir -G $Generator -A x64 `
        "-DCMAKE_BUILD_TYPE=$Config" @extra
    if ($LASTEXITCODE -ne 0) { Fail 'cmake configure failed' }
}

# --------------------------------------------------------------------------
# Stage: build
# --------------------------------------------------------------------------
function Invoke-Build {
    Write-Step "Build ($Config)"
    & cmake --build $BuildDir --config $Config --parallel $env:NUMBER_OF_PROCESSORS
    if ($LASTEXITCODE -ne 0) { Fail 'build failed' }
}

# --------------------------------------------------------------------------
# Stage: package
# --------------------------------------------------------------------------
function Invoke-Package {
    Write-Step 'Collecting the result'

    $out = Join-Path $RepoRoot 'dist-windows'
    New-Item -ItemType Directory -Force -Path $out | Out-Null

    $binaries = Get-ChildItem -Path $BuildDir -Recurse -Include 'worldserver.exe','bnetserver.exe' `
                -ErrorAction SilentlyContinue
    if (-not $binaries) { Fail "No worldserver.exe / bnetserver.exe under $BuildDir" }
    $binaries | ForEach-Object {
        Write-Note "server: $($_.Name)"
        Copy-Item $_.FullName $out -Force
    }

    # the .dist templates the servers read on first start
    Get-ChildItem -Path $BuildDir -Recurse -Filter '*.conf.dist' -ErrorAction SilentlyContinue |
        ForEach-Object {
            $target = Join-Path $out ($_.Name -replace '\.dist$','')
            Write-Note "config: $($_.Name) -> $(Split-Path $target -Leaf)"
            if (-not (Test-Path $target)) { Copy-Item $_.FullName $target }
            Copy-Item $_.FullName $out -Force
        }

    # runtime DLLs the servers link against
    $dllSources = @(
        "$env:ProgramFiles\MariaDB*\lib\libmariadb.dll"
        "$env:ProgramFiles\OpenSSL*\bin\libcrypto-*.dll"
        "$env:ProgramFiles\OpenSSL*\bin\libssl-*.dll"
    )
    foreach ($pattern in $dllSources) {
        Get-Item $pattern -ErrorAction SilentlyContinue | ForEach-Object {
            Write-Note "dll: $($_.Name)"
            Copy-Item $_.FullName $out -Force
        }
    }

    New-Item -ItemType Directory -Force -Path (Join-Path $out 'ClientData') | Out-Null

    Write-Host ''
    Write-Host "Result: $out" -ForegroundColor Green
    Write-Host @'

Still missing, and this script cannot supply either of them:

  1. A TrinityCore 7.3.5 world database. Import it first, then apply sql\base.
  2. ClientData. Build the extractors from this source tree and run them
     against a World of Warcraft 7.3.5 client you own:
        mapextractor, vmap4extractor + vmap4assembler, mmaps_generator
     Copy maps, vmaps, mmaps, dbc and cameras into dist-windows\ClientData.

  Neither is downloadable: the database is third-party content and the client
  data is Blizzard's. See docs\SETUP.md.
'@ -ForegroundColor Yellow
}

# --------------------------------------------------------------------------

if (-not ($Prerequisites -or $Boost -or $Configure -or $Build -or $Package -or $All)) {
    Get-Help $PSCommandPath -Detailed
    exit 0
}

Write-Host "LegionPlayerBot Windows setup" -ForegroundColor White
Write-Host "repository : $RepoRoot"
Write-Host "work area  : $WorkRoot"
Write-Host "build dir  : $BuildDir"

if ($All -or $Prerequisites) { Install-Prerequisites }
if ($All -or $Boost)         { Install-Boost }
if ($All -or $Configure)     { Invoke-Configure }
if ($All -or $Build)         { Invoke-Build }
if ($All -or $Package)       { Invoke-Package }

Write-Host "`nDone." -ForegroundColor Green
