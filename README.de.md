<div align="center">

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/assets/felforge-logo-dark.svg">
  <img alt="FelForge" src="docs/assets/felforge-logo.svg" width="560">
</picture>

**Ein quelloffener World-of-Warcraft-*Legion*-Server (7.3.5)
mit Spielerbots direkt im Kern.**

[![Version](https://img.shields.io/badge/Version-0.1.0-2ea043.svg)](#roadmap)
[![Lizenz](https://img.shields.io/badge/Lizenz-GPL--2.0-blue.svg)](COPYING)
[![Erweiterung](https://img.shields.io/badge/WoW-Legion%207.3.5-orange.svg)](#)
[![Build](https://img.shields.io/badge/Build-GCC%2013.3%20%7C%20MSVC%202017%2B-brightgreen.svg)](#bauen)
[![Sprache](https://img.shields.io/badge/C%2B%2B-11-00599C.svg)](#)
[![Kodierung](https://img.shields.io/badge/Quelltext-100%25%20UTF--8-success.svg)](#kodierung)

[English](README.md) · **Deutsch**

[Einrichtung](docs/SETUP.md) · [Windows](docs/SETUP-Windows.md) · [Roadmap](#roadmap) · [Mitmachen](CONTRIBUTING.md) · [Fehlerbericht Legion](docs/Fehlerbericht-Legion.md) · [Lizenz](COPYING)

</div>

---

## Inhalt

- [Warum FelForge](#warum-felforge)
- [Worum es geht](#worum-es-geht)
- [Bauen](#bauen)
- [Stand auf einen Blick](#stand-auf-einen-blick)
- [Bot-KI](#bot-ki)
  - [Klassenabdeckung](#klassenabdeckung)
  - [Warteschlangen](#warteschlangen)
  - [Schlachtfeldziele](#schlachtfeldziele)
  - [Beute](#beute)
- [Verwaltung](#verwaltung)
- [Datenbanken und Clientdaten](#datenbanken-und-clientdaten)
- [Inhalte und Instandsetzungen](#inhalte-und-instandsetzungen)
- [Kodierung](#kodierung)
- [Sicherheit](#sicherheit)
- [Bekannte Einschränkungen](#bekannte-einschränkungen)
- [Roadmap](#roadmap)
- [Mitmachen](#mitmachen)
- [Herkunft und Lizenz](#herkunft-und-lizenz)

---

## Warum FelForge

FelForge will **der beste Legion-Server-Emulator** werden — der vollständigste,
der stabilste und der, dessen Welt sich auch dann lebendig anfühlt, wenn nur
eine Handvoll Leute online ist. Ob das gelingt, entscheiden drei Dinge:

- **Bots als vollwertige Spieler.** Schlachtfelder, Arenen und der
  Dungeonfinder sollen auch auf einem kleinen Realm aufgehen, weil Bots sich
  anmelden, kämpfen und würfeln wie Spieler — über dieselben Codepfade, nicht
  an ihnen vorbei.
- **Inhalte, die funktionieren, Erweiterung für Erweiterung.** Skripte werden
  gegen diesen Kern geprüft und instand gesetzt, Classic bis Legion, und das
  Ergebnis wird festgehalten.
- **Aussagen, die man nachprüfen kann.** Was diese README als funktionierend
  bezeichnet, ist gebaut und geprüft; was noch nicht zur Laufzeit getestet ist,
  steht so da. Die [Roadmap](#roadmap) zeigt, was offen ist.

---

## Worum es geht

Die Bots hier sind keine Marionetten. Sie sind **echte Charaktere auf echten
Konten**: Sie melden sich an, wählen Talente, reihen sich für Schlachtfelder,
Arenen und den Dungeonfinder ein, kämpfen mit klassenspezifischer Kampf-KI,
würfeln auf Beute, antworten auf Flüstern und beleben die offene Welt.

Das Teilsystem sitzt **im** Kern statt daneben als Modul — ein Bot durchläuft
dieselbe Sitzung, dieselben Paketverarbeiter und dieselben Warteschlangen wie
ein menschlicher Spieler. Es gibt keine Skriptbrücke und keinen zweiten Prozess,
den man synchron halten müsste.

| | |
|---|---|
| Bot-KI | rund 42 000 Zeilen in 81 Dateien, fünf KI-Betriebsarten |
| Spielskripte | 1 127 Skriptdateien, Classic bis Legion |
| Quelltext gesamt | rund 1,38 Mio. Zeilen |

---

## Bauen

```sh
git clone https://github.com/puRe991/LegionPlayerBot.git FelForge
cd FelForge
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Werkzeugkette.** Boost ab 1.60, OpenSSL 1.0/1.1/3.x sowie MySQL ab 5.7 bzw.
MariaDB ab 10.4 werden unterstützt; die Versionsunterschiede löst das Bausystem
auf, statt sie festzunageln. Unter Windows wird MSVC 2017 oder neuer gebraucht.

**Für Windows** gibt es einen geskripteten Weg — `tools\windows\setup.bat` holt
die Werkzeugkette über winget, baut Boost aus einem prüfsummengesicherten
Quellarchiv in die von CMake erwartete Ablage, konfiguriert, übersetzt und sammelt
das Ergebnis ein. Reines Batch, nur mit Windows-Bordmitteln: `curl`, `certutil`,
`tar`, `winget`. Das Skript ist **nicht unter Windows ausgeführt worden**; siehe
[docs/SETUP-Windows.md](docs/SETUP-Windows.md).

Der Baum war früher reine MSVC-Kost. Er baut inzwischen unter Linux mit GCC
durchgehend durch.

> [!IMPORTANT]
> **Vor dem ersten Start** die Prüfschritte aus
> [Abschnitt 4a der Einrichtung](docs/SETUP.md) durchgehen. Ohne Weltdatenbank
> bricht der Server mit einer irreführenden Meldung ab: Die erste Zeile lautet
> `Table 'world.linked_respawn' doesn't exist` und nicht „keine Weltdatenbank".

---

## Stand auf einen Blick

| | |
|---|---|
| **Linux / GCC** | ✅ baut — geprüft mit GCC 13.3, CMake 3.28, Boost 1.83, OpenSSL 3.0, MariaDB 10.11 |
| **Windows / MSVC** | ✅ baut — MSVC 2017 oder neuer |
| **Ergebnisse** | `worldserver`, `bnetserver` — keine offenen Symbole |
| **Datenbanken** | der LegionCore-Satz vom 25.04.2020 plus `sql/legioncore` und `sql/base`: alle 1 414 vorbereiteten Anweisungen des Kerns laufen fehlerfrei |
| **Startlauf** | `bnetserver` läuft und meldet den Realm; `worldserver` lädt alle vier Datenbanken und hält erst bei den fehlenden Clientdaten an |
| **Spielerbots** | legen ihre Charaktere selbst an und kommen für Schlachtfelder, Arenen und den Dungeonfinder online — gebaut und geprüft, noch nicht in einer laufenden Welt erprobt |
| **Nicht enthalten** | die Datenbanken (herunterladbar, siehe unten) und die Clientdaten (aus dem eigenen Client) |

---

## Bot-KI

Fünf KI-Betriebsarten, jede mit Umsetzungen je Klasse:

| Betriebsart | Zweck |
|---|---|
| `BotFieldAI` | offene Welt |
| `BotGroupAI` | Gruppe und Dungeon |
| `BotBGAI` | Schlachtfelder |
| `BotArenaAI` | Arena |
| `BotDuelAI` | Duelle |

Die Bots wählen ihre Talente selbst, eines je freigeschalteter Stufe, passend
zur aktiven Spezialisierung.

### Klassenabdeckung

| Klasse | Abdeckung |
|---|---|
| Krieger, Paladin, Jäger, Schurke, Priester, Schamane, Magier, Hexenmeister, Druide | vollständig |
| Mönch, Dämonenjäger | spielbar — Heilerrolle Nebelwirker, Tankrollen Braumeister und Rachsucht; Rotationen flacher als bei den älteren Klassen |
| Todesritter | vollständig außer auf Schlachtfeldern |

### Warteschlangen

| Warteschlange | Bot-Unterstützung |
|---|---|
| Schlachtfelder | Bots füllen auf, sobald ein echter Spieler wartet |
| Scharmützelarena | ein einzelner Bot meldet sich allein an |
| Gewertete Arena | der Bot-Verwalter bildet eine Gruppe aus 2 oder 3; der Anführer meldet an |
| Dungeonfinder | Bots füllen die fehlende Rolle und nehmen den Gruppenvorschlag selbsttätig an |

**Gewertete Arena.** Legion kennt keine Arenateams mehr — angemeldet wird vom
Anführer einer gewöhnlichen Gruppe passender Größe.
`PlayerBotMgr::AddTeamBotToRatedArena` stellt diese Gruppe aus untätigen Bots
einer Fraktion auf Höchststufe zusammen.

**Dungeonfinder.** Der arbeitet von der anderen Seite.
`LFGMgr::SearchLFGBotRequirement` durchsucht die Warteschlange nach einem
*echten* wartenden Spieler und meldet, welche Rolle noch fehlt; der
Bot-Verwalter holt dann einen passenden Charakter online. Botsitzungen werden
bei dieser Suche bewusst übersprungen — sonst würden Bots sich gegenseitig
anmelden und die Warteschlange liefe nie leer. Da ein Bot keinen Client zum
Anklicken hat, reiht `SendLfgUpdateProposal` die Zusage selbst ein.

### Schlachtfeldziele

Die Zielsteuerung — Flaggen, Stützpunkte, Fahrzeuge — ist umgesetzt für
**Arathibecken, Warsongschlucht, Auge des Sturms, Alteractal und Insel der
Eroberung**. In den übrigen Schlachtfeldern kämpfen die Bots, kümmern sich aber
nicht um die Ziele.

Die Zielsteuerung läuft feste Punkte an — Flaggen, Stützpunkte, Friedhöfe,
Startpositionen —, die sie über ihre Kennung in der Tabelle `aiwaypoints`
nachschlägt. `sql/base/world_playerbot.sql` füllt sie mit diesen 61 Punkten für
die fünf Schlachtfelder und die Arena der Kanalisation von Dalaran.

### Beute

Bei einem Gruppenwurf würfelt ein Bot **Bedarf**, wenn der Gegenstand zu seiner
Klasse *und* zur aktuellen Spezialisierung passt, er ihn tatsächlich anlegen
kann und er besser ist als das, was den Platz belegt — dieselbe Prüfung, die
der Bot auch für seine eigene Ausrüstung benutzt. Sonst würfelt er Gier, oder
er passt, wenn der Wurf nichts anderes zulässt. Verhalten und Qualitätsgrenze
sind einstellbar.

---

## Verwaltung

`.playerbot` (kurz `.pbot`), ab Spielleiterrang:

| Befehl | Wirkung |
|---|---|
| `add <alliance\|horde> [Klasse] [Anzahl]` | Bots einreihen. Klasse als Name oder Kennung; ohne Angabe beliebig. |
| `remove <all\|Konto-ID>` | Bots abmelden. |
| `limit [Anzahl]` | Onlinegrenze dieser Sitzung lesen oder setzen (Administrator). |
| `status` | Onlinezahlen und aktuelle Beuteeinstellungen. |

Der Server richtet seine Bots selbst ein: Er legt die Botkonten an und im
Hintergrund nach dem Start auf den ersten 20 davon je einen Charakter jeder
Klasse, aufgeteilt auf Allianz und Horde. Einzelheiten unter
[Bot characters](docs/SETUP.md#bot-characters).

Wie viele Bots online sind, steht in der Konfiguration: `PlayerBot.MaxOnline`
begrenzt sie, `PlayerBot.MinOnline` hält eine Anzahl davon auch ohne
Warteschlange in der offenen Welt, und `PlayerBot.Accounts` legt fest, wie viele
Botkonten es gibt. `PlayerBot.Announce = 0` schaltet die Chatmeldung ab, die
jeder Spieler bekommt, wenn ein Bot online oder offline geht.

Der Block `PLAYERBOT SETTINGS` in `worldserver.conf.dist` beschreibt jede
Einstellung: Hauptschalter, Onlinegrenze, kontogebundene Bots, wie viele Konten
Charaktere bekommen, und das Würfelverhalten.

---

## Datenbanken und Clientdaten

Vier Datenbanken werden gebraucht: `auth`, `characters`, `world`, `hotfixes`.

1. **Die LegionCore-7.3.5-Datenbanken vom 25.04.2020 einspielen** — Welt und
   Hotfixes aus dem Release `LegionCore735.20041` des Legion Preservation
   Project, auth und characters aus demselben Tag. Die TrinityCore-Datenbanken
   7.3.5 passen zu diesem Kern **nicht**.
2. **`sql/legioncore/` anwenden** — zwei kleine Skripte, die die Lücken
   zwischen diesen Dumps und dem Kern schließen.
3. **`sql/base/` anwenden** — die Zusatztabellen, die dieser Kern braucht: das
   Navigationsnetz der Bots, Namensvorräte, Gesprächszeilen und die
   Zugangsliste des Werkzeug-Sockets.
4. **Clientdaten** nach `DataDir` extrahieren: `dbc`, `gt`, `cameras`, `maps`,
   `vmaps`, `mmaps`. Der Karten-Extraktor in diesem Repository ist veraltet;
   den aktuellen des Legion Preservation Project nehmen.
5. **Das Kennwort des vorbelegten Kontos `GM@GM` ändern** und
   `auth.realmlist` einrichten.

Die genauen Befehle in der richtigen Reihenfolge stehen in
[docs/SETUP.md](docs/SETUP.md).

---

## Inhalte und Instandsetzungen

Neben dem Bot-Teilsystem trägt dieser Baum den TrinityCore-Skriptbestand von
Classic bis Legion, Erweiterung für Erweiterung geprüft.

| Erweiterung | Arbeit |
|---|---|
| **Classic** | Buru der Verschlinger, Ossirian der Narbenlose und Viscidus aus leeren Hüllen geschrieben; Ragefire-Schlund, Sturmwind-Verlies und Düsterbruch als Instanzen gebunden |
| **Burning Crusade** | vier fehlende Instanzskripte; das Schachereignis in Karazhan; der Sonnenbrunnen-Epilog |
| **Wrath** | die Luftschiffschlacht der Eiskronenzitadelle gegen die Transport-Schnittstelle dieses Kerns neu gebaut |
| **Cataclysm** | 21 abgeschaltete Skripte wieder aktiviert; der Brunnen der Ewigkeit aus leeren Dateien wiederhergestellt |
| **Warlords** | Begegnungen der Schwarzfelsgießerei und der Höllenfeuerzitadelle umgesetzt; Grimmschienen-Depot ergänzt |
| **Legion** | siehe [Fehlerbericht](docs/Fehlerbericht-Legion.md) |

Die Registrierungslage wird maschinell geprüft. Stand des aktuellen Baums:

- jede Skriptklasse ist gebunden — **0** definiert, aber nie registriert
- **0** doppelte Skriptnamen
- **0** leere Quelldateien
- jede `.cpp` erreicht den Build
- **0** `Register()`-Rümpfe ohne aktiven Handler
- **0** unmarkierte Durchfallstellen in `switch`-Anweisungen

---

## Kodierung

Der gesamte Quellbaum ist UTF-8. Das war nicht immer so — 26 Dateien des
Bot-Teilsystems waren GBK, einige weitere cp1251.

Spielersichtbare Texte, die früher chinesische Literale waren, liefen über
`consoleToUtf8`; die Funktion ist auf Linux ein Durchreicher und hätte rohe
Bytes an den Client geschickt. Heute sind es schlichte englische
UTF-8-Zeichenketten.

---

## Sicherheit

> [!CAUTION]
> `src/server/bnetserver/bnetserver.key.pem` ist der **Entwicklungsschlüssel**
> von TrinityCore und damit öffentlich bekannt. Vor dem Betrieb eines
> erreichbaren Realms austauschen.

Botkonten (`PLAYERBOT<Nummer>`) werden mit dem festen Kennwort `botxxx`
angelegt — die auth-Datenbank gehört deshalb nicht in ein öffentliches
Netz.

---

## Bekannte Einschränkungen

- Außerhalb der fünf genannten Schlachtfelder kämpfen die Bots, kümmern sich
  aber nicht um die Ziele.
- Artefakt-Erwerbsszenarien gibt es für sieben von zwölf Klassen.
- Ein Teil der Kommentare im Bot-Teilsystem ist weiterhin chinesisch. Sie sind
  korrekt kodiert und lesbar — nur eben nicht übersetzt.

---

## Roadmap

**0.1 — der Stand.** Baut unter Linux mit GCC vollständig durch. Bots legen
ihre Charaktere selbst an, füllen Schlachtfelder, Scharmützel- und gewertete
Arena sowie den Dungeonfinder auf, und die Zielpunkte für fünf Schlachtfelder
liegen den Datenbankskripten bei.

**Als Nächstes.**

1. **Laufzeitprüfung.** Einen Realm mit den LegionCore-Datenbanken und
   Clientdaten betreiben und daraus einen wiederholbaren Testrealm machen:
   Bots melden sich an, reihen sich ein, betreten, kämpfen, verlassen.
2. **Kontinuierliche Integration.** Jeden Push unter Linux und Windows bauen,
   damit der Build-Badge eine Tatsache meldet und keine Behauptung.
3. **Epische Schlachtfelder.** Bots melden sich immer für das gewöhnliche
   Zufallsschlachtfeld an; die epische Warteschlange für Alteractal und Insel
   der Eroberung prüfen und anbinden.
4. **Mehr Ziele.** Zielsteuerung für Zwillingsgipfel, Schlacht um Gilneas,
   Silberbruchmine, Tempel von Kotmogu und Tiefenwindschlucht.
5. **Klassentiefe.** Vollere Rotationen für Mönch und Dämonenjäger;
   Schlachtfeld-KI für Todesritter.
6. **Legion-Inhalte.** Artefakt-Erwerbsszenarien für die fünf Klassen, denen
   sie fehlen; die offenen Punkte des
   [Fehlerberichts](docs/Fehlerbericht-Legion.md) abarbeiten.
7. **Aufräumen.** Einstellbares Kennwort für Botkonten; die restlichen
   chinesischen Kommentare übersetzen.

---

## Mitmachen

Fehlerberichte, Korrekturen und Inhaltsarbeit sind willkommen — wie man baut,
was ein guter Bericht enthält und wie Änderungen geprüft werden, steht in
[CONTRIBUTING.md](CONTRIBUTING.md).

---

## Herkunft und Lizenz

FelForge hieß anfangs *LegionPlayerBot*. Es baut auf
[TrinityCore](https://www.trinitycore.org/) und dessen LegionCore-Ableger auf
und behält deren Urheberrechtsvermerke. Lizenziert unter **GPL v2**, von
TrinityCore übernommen — siehe [COPYING](COPYING).

World of Warcraft und Legion sind Marken von Blizzard Entertainment. FelForge
steht in keiner Verbindung zu Blizzard Entertainment und wird von dort nicht
unterstützt.
