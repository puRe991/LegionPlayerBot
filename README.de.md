<div align="center">

# LegionPlayerBot

**Ein von TrinityCore abgeleiteter World-of-Warcraft-*Legion*-Server (7.3.5)
mit Spielerbots direkt im Kern.**

[![Lizenz](https://img.shields.io/badge/Lizenz-GPL--2.0-blue.svg)](COPYING)
[![Erweiterung](https://img.shields.io/badge/WoW-Legion%207.3.5-orange.svg)](#)
[![Build](https://img.shields.io/badge/Build-GCC%2013.3%20%7C%20MSVC%202017%2B-brightgreen.svg)](#bauen)
[![Sprache](https://img.shields.io/badge/C%2B%2B-14-00599C.svg)](#)
[![Kodierung](https://img.shields.io/badge/Quelltext-100%25%20UTF--8-success.svg)](#kodierung)

[English](README.md) · **Deutsch**

[Einrichtung](docs/SETUP.md) · [Fehlerbericht Legion](docs/Fehlerbericht-Legion.md) · [Lizenz](COPYING)

</div>

---

## Inhalt

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
- [Herkunft und Lizenz](#herkunft-und-lizenz)

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
git clone https://github.com/puRe991/LegionPlayerBot.git
cd LegionPlayerBot
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Werkzeugkette.** Boost ab 1.60, OpenSSL 1.0/1.1/3.x sowie MySQL ab 5.7 bzw.
MariaDB ab 10.4 werden unterstützt; die Versionsunterschiede löst das Bausystem
auf, statt sie festzunageln. Unter Windows wird MSVC 2017 oder neuer gebraucht.

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
| **Startlauf** | startet, liest die Konfiguration, öffnet die Datenbanken auth, characters und hotfixes |
| **Nicht enthalten** | eine TrinityCore-Weltdatenbank 7.3.5 und entpackte Clientdaten |

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

> [!WARNING]
> **Dieser Code braucht Navigationsdaten, die dieses Repository nicht
> mitliefert.** Die Tabelle `aiwaypoints` wird leer angelegt. Bots bewegen sich
> im Schlachtfeld entlang eines Wegpunktnetzes, das ein Betreiber im Spiel setzt
> und das in die Weltdatenbank zurückgeschrieben wird. Solange diese Zeilen
> fehlen, hat die Zielsteuerung nichts, woran sie entlanglaufen könnte — in den
> fünf genannten Schlachtfeldern genauso wie überall sonst.

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

Der Block `PLAYERBOT SETTINGS` in `worldserver.conf.dist` beschreibt jede
Einstellung: Hauptschalter, Onlinegrenze, kontogebundene Bots und das
Würfelverhalten.

---

## Datenbanken und Clientdaten

Vier Datenbanken werden gebraucht: `auth`, `characters`, `world`, `hotfixes`.

1. **Eine TrinityCore-Weltdatenbank 7.3.5 einspielen.** Liegt hier nicht bei.
2. **Danach `sql/base/` anwenden** — die Zusatztabellen, die dieser Kern
   braucht: das Navigationsnetz der Bots, Namensvorräte, Gesprächszeilen und
   die Zugangsliste des Werkzeug-Sockets.
3. **Clientdaten entpacken** nach `DataDir`, mit `mapextractor`,
   `vmap4extractor`/`vmap4assembler` und `mmaps_generator`.
4. **`auth.realmlist` befüllen.**

Die Reihenfolge zählt — `sql/base` gehört *nach* die Weltdatenbank, nicht davor.
Ausführlich in [docs/SETUP.md](docs/SETUP.md).

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

Botkonten werden mit einem festen Kennwort angelegt — die auth-Datenbank gehört
deshalb nicht in ein öffentliches Netz.

---

## Bekannte Einschränkungen

- Außerhalb der fünf genannten Schlachtfelder kämpfen die Bots, kümmern sich
  aber nicht um die Ziele.
- Die gesamte Zielsteuerung bleibt wirkungslos, bis ein Betreiber das
  Wegpunktnetz `aiwaypoints` aufgebaut hat.
- Artefakt-Erwerbsszenarien gibt es für sieben von zwölf Klassen.
- Ein Teil der Kommentare im Bot-Teilsystem ist weiterhin chinesisch. Sie sind
  korrekt kodiert und lesbar — nur eben nicht übersetzt.

---

## Herkunft und Lizenz

Aufgebaut auf [TrinityCore](https://www.trinitycore.org/) und dessen
LegionCore-Ableger. Lizenziert unter **GPL v2**, von TrinityCore übernommen —
siehe [COPYING](COPYING).
