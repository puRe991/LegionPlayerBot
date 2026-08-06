@echo off
rem ==========================================================================
rem  LegionPlayerBot - Windows-Einrichtung
rem
rem  Holt die Abhaengigkeiten, legt sie dorthin, wo CMake sie sucht,
rem  konfiguriert und uebersetzt.
rem
rem  Aufruf:
rem      setup.bat all          alle Stufen der Reihe nach
rem      setup.bat prereq       Werkzeugkette ueber winget
rem      setup.bat boost        Boost laden, pruefen, uebersetzen
rem      setup.bat configure    CMake
rem      setup.bat build        uebersetzen
rem      setup.bat package      Server und Konfigurationen einsammeln
rem
rem  Benutzt nur Bordmittel: curl, certutil, tar, winget.
rem  curl und tar liefert Windows ab 10/1803 mit, winget ab 10/1809.
rem
rem  NICHT UNTER WINDOWS AUSGEFUEHRT. Geschrieben gegen die CMake-Dateien
rem  dieses Repositoriums auf einer Linux-Maschine, auf der davon nichts
rem  laufen kann. Den ersten Lauf bitte mitlesen.
rem ==========================================================================

setlocal EnableDelayedExpansion EnableExtensions

rem --- Einstellungen ------------------------------------------------------
set "BOOST_VERSION=1.83.0"
set "BOOST_UNDERSCORE=1_83_0"
rem Aus https://archives.boost.io/release/1.83.0/source/boost_1_83_0.tar.gz.json
set "BOOST_SHA256=c0685b68dd44cc46574cce86c4e17c0f611b15e195be9848dfd0769a0a207628"
set "BOOST_TOOLSETDIR=lib64-msvc-14.3"

set "GENERATOR=Visual Studio 17 2022"
set "CONFIG=Release"

rem --- Pfade --------------------------------------------------------------
set "SCRIPT_DIR=%~dp0"
pushd "%SCRIPT_DIR%..\.." || (echo Konnte nicht ins Repositorium wechseln.& exit /b 1)
set "REPO_ROOT=%CD%"
popd

set "WORK_ROOT=%REPO_ROOT%\.winbuild"
set "BUILD_DIR=%REPO_ROOT%\build-windows"
set "DIST_DIR=%REPO_ROOT%\dist-windows"
set "DL_DIR=%WORK_ROOT%\download"
set "BOOST_SRC=%WORK_ROOT%\boost_%BOOST_UNDERSCORE%"
set "BOOST_TARGET=%WORK_ROOT%\boost"

rem --- Stufe waehlen ------------------------------------------------------
set "STAGE=%~1"
if "%STAGE%"=="" goto :usage
if /i "%STAGE%"=="-h"     goto :usage
if /i "%STAGE%"=="help"   goto :usage
if /i "%STAGE%"=="/?"     goto :usage

echo.
echo LegionPlayerBot - Windows-Einrichtung
echo   Repositorium : %REPO_ROOT%
echo   Arbeitsordner: %WORK_ROOT%
echo   Bauordner    : %BUILD_DIR%
echo   Stufe        : %STAGE%

if /i "%STAGE%"=="all"       goto :run_all
if /i "%STAGE%"=="prereq"    call :stage_prereq    & goto :done
if /i "%STAGE%"=="boost"     call :stage_boost     & goto :done
if /i "%STAGE%"=="configure" call :stage_configure & goto :done
if /i "%STAGE%"=="build"     call :stage_build     & goto :done
if /i "%STAGE%"=="package"   call :stage_package   & goto :done

echo.
echo Unbekannte Stufe: %STAGE%
goto :usage

:run_all
call :stage_prereq    || goto :failed
call :stage_boost     || goto :failed
call :stage_configure || goto :failed
call :stage_build     || goto :failed
call :stage_package   || goto :failed
goto :done


rem ==========================================================================
rem  Stufe: Werkzeugkette
rem ==========================================================================
:stage_prereq
echo.
echo === Werkzeugkette
where winget >nul 2>&1
if errorlevel 1 (
    echo.
    echo   winget wurde nicht gefunden. Es gehoert zum "App Installer" und ist
    echo   ab Windows 10/1809 dabei. Aus dem Microsoft Store nachinstallieren,
    echo   oder diese fuenf Pakete von Hand einrichten:
    echo     Visual Studio 2022 Build Tools ^(Arbeitslast "Desktopentwicklung mit C++"^)
    echo     CMake, Git, OpenSSL ^(Entwicklungsdateien^), MariaDB
    exit /b 1
)

net session >nul 2>&1
if errorlevel 1 (
    echo   Hinweis: nicht als Administrator gestartet. winget fragt dann je
    echo            Paket nach oder scheitert an maschinenweiten Installationen.
)

rem Kennungen erst pruefen, dann installieren. Paketkennungen aendern sich;
rem lieber sauber abbrechen als etwas Falsches einrichten.
set "PKGS=Microsoft.VisualStudio.2022.BuildTools Kitware.CMake Git.Git ShiningLight.OpenSSL.Dev MariaDB.Server"
set "MISSING="
for %%P in (%PKGS%) do (
    echo   pruefe %%P
    winget show --id %%P --exact --disable-interactivity >nul 2>&1
    if errorlevel 1 set "MISSING=!MISSING! %%P"
)

if not "!MISSING!"=="" (
    echo.
    echo   Diese Paketkennungen liefert deine winget-Quelle nicht:
    for %%M in (!MISSING!) do echo     %%M
    echo.
    echo   Kennungen aendern sich mit der Zeit. Die aktuelle findest du mit:
    echo     winget search ^<Name^>
    echo   Danach die Liste PKGS oben in diesem Skript anpassen.
    exit /b 1
)

for %%P in (%PKGS%) do (
    echo   installiere %%P
    winget install --id %%P --exact --silent --accept-package-agreements --accept-source-agreements
    rem winget meldet auch bei harmlosen Faellen ungleich null, etwa
    rem "ist bereits installiert". Wir melden das und machen weiter; ob eine
    rem Abhaengigkeit wirklich brauchbar ist, zeigt erst die Configure-Stufe.
    if errorlevel 1 echo     ^(winget meldete !errorlevel! - meist "bereits installiert"^)
)

echo   Die C++-Arbeitslast ist in den Build Tools nicht vorausgewaehlt.
echo   Findet CMake spaeter keinen Compiler: Visual Studio Installer oeffnen
echo   und "Desktopentwicklung mit C++" nachtragen.
exit /b 0


rem ==========================================================================
rem  Stufe: Boost
rem ==========================================================================
:stage_boost
echo.
echo === Boost %BOOST_VERSION%

set "ARCHIVE=boost_%BOOST_UNDERSCORE%.tar.gz"
set "ARCHIVE_PATH=%DL_DIR%\%ARCHIVE%"
set "URL=https://archives.boost.io/release/%BOOST_VERSION%/source/%ARCHIVE%"

if not exist "%DL_DIR%" mkdir "%DL_DIR%" 2>nul

if exist "%ARCHIVE_PATH%" (
    echo   Archiv liegt schon da, wird wiederverwendet
) else (
    where curl >nul 2>&1
    if errorlevel 1 (
        echo   curl fehlt. Ab Windows 10/1803 ist es dabei.
        echo   Archiv sonst von Hand laden nach:
        echo     %ARCHIVE_PATH%
        echo   Quelle: %URL%
        exit /b 1
    )
    echo   lade %URL%
    curl -L --fail --retry 3 --retry-delay 2 -o "%ARCHIVE_PATH%" "%URL%"
    if errorlevel 1 (
        echo   Download fehlgeschlagen.
        if exist "%ARCHIVE_PATH%" del /q "%ARCHIVE_PATH%"
        exit /b 1
    )
)

echo   pruefe SHA-256
set "ACTUAL="
for /f "skip=1 tokens=* delims=" %%H in ('certutil -hashfile "%ARCHIVE_PATH%" SHA256 2^>nul') do (
    if not defined ACTUAL (
        set "LINE=%%H"
        rem Leerzeichen entfernen; aeltere certutil-Fassungen gruppieren die Bytes
        set "LINE=!LINE: =!"
        rem Die Erfolgsmeldung von certutil ueberspringen
        echo !LINE! | findstr /i /c:"certutil" >nul || set "ACTUAL=!LINE!"
    )
)

if not defined ACTUAL (
    echo   certutil lieferte keinen Hash - Pruefung nicht moeglich, Abbruch.
    exit /b 1
)

if /i not "!ACTUAL!"=="%BOOST_SHA256%" (
    echo.
    echo   Pruefsumme stimmt nicht:
    echo     erwartet %BOOST_SHA256%
    echo     erhalten !ACTUAL!
    echo   Der Download wird geloescht.
    del /q "%ARCHIVE_PATH%"
    exit /b 1
)
echo   ok  !ACTUAL!

if not exist "%BOOST_SRC%" (
    where tar >nul 2>&1
    if errorlevel 1 (
        echo   tar fehlt. Ab Windows 10/1803 ist es dabei; sonst von Hand
        echo   entpacken nach %WORK_ROOT%
        exit /b 1
    )
    echo   entpacke ^(dauert^)
    tar -xzf "%ARCHIVE_PATH%" -C "%WORK_ROOT%"
    if errorlevel 1 echo   tar meldete einen Fehler. & exit /b 1
)

pushd "%BOOST_SRC%" || exit /b 1

if not exist "b2.exe" (
    echo   bootstrap
    call bootstrap.bat
    if errorlevel 1 (
        echo   bootstrap.bat fehlgeschlagen. Aus einer
        echo   "Developer Command Prompt for VS 2022" starten, damit die
        echo   MSVC-Umgebung geladen ist.
        popd
        exit /b 1
    )
)

rem dep/boost/CMakeLists.txt setzt Boost_USE_STATIC_LIBS ON und sucht in
rem lib64-msvc-<Toolset>. Also statisch, mehrfaedig, gegen die dynamische
rem Laufzeit - und genau die sechs Bibliotheken, die das Projekt anfordert.
echo   uebersetze die sechs benoetigten Bibliotheken
b2.exe ^
    --stagedir="%BOOST_TARGET%" ^
    --build-dir="%WORK_ROOT%\boost-build" ^
    --with-system --with-filesystem --with-thread ^
    --with-program_options --with-iostreams --with-regex ^
    address-model=64 architecture=x86 ^
    link=static runtime-link=shared threading=multi ^
    variant=release ^
    -j %NUMBER_OF_PROCESSORS% ^
    stage
if errorlevel 1 (
    echo   b2 fehlgeschlagen.
    popd
    exit /b 1
)

rem b2 legt nach <stagedir>\lib ab; CMake will lib64-msvc-<Toolset>
if exist "%BOOST_TARGET%\lib" (
    if not exist "%BOOST_TARGET%\%BOOST_TOOLSETDIR%" (
        echo   verschiebe lib -^> %BOOST_TOOLSETDIR%
        move /y "%BOOST_TARGET%\lib" "%BOOST_TARGET%\%BOOST_TOOLSETDIR%" >nul
    )
)

if not exist "%BOOST_TARGET%\boost" (
    echo   kopiere Kopfdateien
    xcopy /e /i /q /y "%BOOST_SRC%\boost" "%BOOST_TARGET%\boost" >nul
)

popd

echo   setze BOOST_ROOT dauerhaft fuer dein Benutzerkonto
setx BOOST_ROOT "%BOOST_TARGET%" >nul
set "BOOST_ROOT=%BOOST_TARGET%"
echo   BOOST_ROOT=%BOOST_TARGET%
echo   Ohne diese Variable bricht dep\boost\CMakeLists.txt mit FATAL_ERROR ab.
echo   setx wirkt erst in neuen Fenstern; in diesem Lauf ist sie bereits gesetzt.
exit /b 0


rem ==========================================================================
rem  Stufe: CMake
rem ==========================================================================
:stage_configure
echo.
echo === CMake

if not defined BOOST_ROOT (
    for /f "tokens=2,*" %%A in ('reg query "HKCU\Environment" /v BOOST_ROOT 2^>nul ^| findstr /i BOOST_ROOT') do set "BOOST_ROOT=%%B"
)
if not defined BOOST_ROOT (
    echo   BOOST_ROOT ist nicht gesetzt. Erst "setup.bat boost" laufen lassen,
    echo   oder ein neues Fenster oeffnen, damit setx wirkt.
    exit /b 1
)
echo   BOOST_ROOT=%BOOST_ROOT%

rem FindMySQL.cmake sucht unter Program Files. Liegt MariaDB woanders,
rem geben wir die Pfade mit.
set "MYSQL_INC="
set "MYSQL_LIB="
for /d %%D in ("%ProgramFiles%\MariaDB*") do (
    if exist "%%~D\include\mysql\mysql.h" set "MYSQL_INC=%%~D\include\mysql"
    if not defined MYSQL_INC if exist "%%~D\include\mysql.h" set "MYSQL_INC=%%~D\include"
    if exist "%%~D\lib\libmariadb.lib" set "MYSQL_LIB=%%~D\lib"
)

set "EXTRA="
if defined MYSQL_INC (
    echo   MySQL-Kopfdateien: %MYSQL_INC%
    set "EXTRA=!EXTRA! -DMYSQL_ADD_INCLUDE_PATH=%MYSQL_INC%"
) else (
    echo   MariaDB-Kopfdateien nicht unter Program Files gefunden.
    echo   Scheitert Configure an MySQL, -DMYSQL_ADD_INCLUDE_PATH selbst setzen.
)
if defined MYSQL_LIB (
    echo   MySQL-Bibliothek:  %MYSQL_LIB%
    set "EXTRA=!EXTRA! -DMYSQL_ADD_LIBRARY_PATH=%MYSQL_LIB%"
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%" 2>nul

cmake -S "%REPO_ROOT%" -B "%BUILD_DIR%" -G "%GENERATOR%" -A x64 -DCMAKE_BUILD_TYPE=%CONFIG% !EXTRA!
if errorlevel 1 (
    echo   cmake configure fehlgeschlagen.
    exit /b 1
)
exit /b 0


rem ==========================================================================
rem  Stufe: uebersetzen
rem ==========================================================================
:stage_build
echo.
echo === Uebersetzen ^(%CONFIG%^)
cmake --build "%BUILD_DIR%" --config %CONFIG% --parallel %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo   Build fehlgeschlagen.
    exit /b 1
)
exit /b 0


rem ==========================================================================
rem  Stufe: einsammeln
rem ==========================================================================
:stage_package
echo.
echo === Ergebnis einsammeln

if not exist "%DIST_DIR%" mkdir "%DIST_DIR%" 2>nul

set "FOUND="
for /r "%BUILD_DIR%" %%F in (worldserver.exe bnetserver.exe) do (
    if exist "%%F" (
        echo   Server: %%~nxF
        copy /y "%%F" "%DIST_DIR%" >nul
        set "FOUND=1"
    )
)
if not defined FOUND (
    echo   Unter %BUILD_DIR% liegt keine worldserver.exe / bnetserver.exe.
    exit /b 1
)

rem Die .dist-Vorlagen, die die Server beim ersten Start lesen
for /r "%BUILD_DIR%" %%F in (*.conf.dist) do (
    echo   Konfiguration: %%~nxF
    copy /y "%%F" "%DIST_DIR%" >nul
    set "PLAIN=%%~nF"
    if not exist "%DIST_DIR%\!PLAIN!" copy /y "%%F" "%DIST_DIR%\!PLAIN!" >nul
)

rem Laufzeit-DLLs
for /d %%D in ("%ProgramFiles%\MariaDB*") do (
    if exist "%%~D\lib\libmariadb.dll" (
        echo   DLL: libmariadb.dll
        copy /y "%%~D\lib\libmariadb.dll" "%DIST_DIR%" >nul
    )
)
for /d %%D in ("%ProgramFiles%\OpenSSL*") do (
    for %%F in ("%%~D\bin\libcrypto-*.dll" "%%~D\bin\libssl-*.dll") do (
        if exist "%%~F" (
            echo   DLL: %%~nxF
            copy /y "%%~F" "%DIST_DIR%" >nul
        )
    )
)

if not exist "%DIST_DIR%\ClientData" mkdir "%DIST_DIR%\ClientData" 2>nul

echo.
echo   Ergebnis: %DIST_DIR%
echo.
echo   Es fehlen noch zwei Dinge, die dieses Skript nicht liefern kann:
echo.
echo     1. Eine TrinityCore-Weltdatenbank 7.3.5. Zuerst einspielen,
echo        danach sql\base anwenden - nicht umgekehrt.
echo.
echo     2. ClientData. Die Entpacker aus diesem Quellbaum bauen und gegen
echo        einen World-of-Warcraft-Client 7.3.5 laufen lassen, den du besitzt:
echo          mapextractor, vmap4extractor + vmap4assembler, mmaps_generator
echo        Danach maps, vmaps, mmaps, dbc und cameras nach
echo        %DIST_DIR%\ClientData kopieren.
echo.
echo   Beides ist nicht herunterladbar: die Datenbank ist Fremdinhalt,
echo   die Clientdaten gehoeren Blizzard. Siehe docs\SETUP-Windows.md
exit /b 0


rem ==========================================================================
:usage
echo.
echo LegionPlayerBot - Windows-Einrichtung
echo.
echo   setup.bat all          alle Stufen der Reihe nach
echo   setup.bat prereq       Werkzeugkette ueber winget
echo   setup.bat boost        Boost laden, pruefen, uebersetzen
echo   setup.bat configure    CMake
echo   setup.bat build        uebersetzen
echo   setup.bat package      Server und Konfigurationen einsammeln
echo.
echo   Einstellungen stehen oben im Skript: Boost-Fassung samt Pruefsumme,
echo   Generator ^(derzeit "%GENERATOR%"^) und Konfiguration ^(%CONFIG%^).
echo.
echo   Ausfuehrlich: docs\SETUP-Windows.md
exit /b 0

:failed
echo.
echo Abgebrochen.
endlocal
exit /b 1

:done
echo.
echo Fertig.
endlocal
exit /b 0
