# LegionPlayerBot

**Ein von TrinityCore abgeleiteter World-of-Warcraft-*Legion*-Server (7.3.5) mit Spielerbots direkt im Kern.**

[English](README.md) · [Einrichtung](docs/SETUP.md) · [Fehlerbericht Legion](docs/Fehlerbericht-Legion.md)

---

Die Bots hier sind keine Marionetten. Sie sind echte Charaktere auf echten
Konten: Sie melden sich an, wählen Talente, reihen sich für Schlachtfelder,
Arenen und den Dungeonfinder ein, kämpfen mit klassenspezifischer Kampf-KI,
würfeln auf Beute, antworten auf Flüstern und beleben die offene Welt. Das
Teilsystem sitzt **im** Kern statt daneben als Modul — ein Bot durchläuft
dieselbe Sitzung, dieselben Paketverarbeiter und dieselben Warteschlangen wie
ein menschlicher Spieler.

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## Stand

| | |
|---|---|
| **Linux / GCC** | baut — geprüft mit GCC 13.3, CMake 3.28, Boost 1.83, OpenSSL 3.0, MariaDB 10.11 |
| **Windows / MSVC** | baut — MSVC 2017 oder neuer |
| **Erzeugt** | `worldserver`, `bnetserver`, keine offenen Symbole |
| **Läuft** | startet, liest die Konfiguration, öffnet die Datenbanken auth, characters und hotfixes |
| **Braucht** | eine TrinityCore-Weltdatenbank 7.3.5 und entpackte Clientdaten — **beides liegt hier nicht bei** |

Der Baum war früher reine MSVC-Kost. Boost ab 1.60, OpenSSL 1.0/1.1/3.x sowie
MySQL ab 5.7 bzw. MariaDB ab 10.4 werden unterstützt; die Versionsunterschiede
löst das Bausystem auf, statt sie festzunageln.

> **Vor dem ersten Start** die Prüfschritte aus
> [Abschnitt 4a der Einrichtung](docs/SETUP.md) durchgehen. Ohne Weltdatenbank
> bricht der Server mit einer irreführenden Meldung ab: Die erste Zeile lautet
> `Table 'world.linked_respawn' doesn't exist` und nicht „keine Weltdatenbank".

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

**Klassenabdeckung.** Vollständig für Krieger, Paladin, Jäger, Schurke,
Priester, Schamane, Magier, Hexenmeister und Druide. Mönch und Dämonenjäger
sind spielbar — einschließlich der Heilerrolle Nebelwirker und der Tankrollen
Braumeister und Rachsucht —, ihre Rotationen sind aber flacher als die der
älteren Klassen. Der Todesritter ist überall abgedeckt außer auf
Schlachtfeldern.

Die Bots wählen ihre Talente selbst, eines je freigeschalteter Stufe, passend
zur aktiven Spezialisierung.

### Warteschlangen

| Warteschlange | Bot-Unterstützung |
|---|---|
| Schlachtfelder | Bots füllen auf, sobald ein echter Spieler wartet |
| Scharmützelarena | ein einzelner Bot meldet sich allein an |
| Gewertete Arena | der Bot-Verwalter bildet eine Gruppe aus 2 oder 3; der Anführer meldet an |
| Dungeonfinder | Bots füllen die fehlende Rolle und nehmen den Gruppenvorschlag selbsttätig an |

Gewertete Arena kennt in Legion keine Arenateams mehr — angemeldet wird vom
Anführer einer gewöhnlichen Gruppe passender Größe.
`PlayerBotMgr::AddTeamBotToRatedArena` stellt diese Gruppe aus untätigen Bots
einer Fraktion auf Höchststufe zusammen.

Der Dungeonfinder arbeitet von der anderen Seite.
`LFGMgr::SearchLFGBotRequirement` durchsucht die Warteschlange nach einem
**echten** wartenden Spieler und meldet, welche Rolle noch fehlt; der
Bot-Verwalter holt dann einen passenden Charakter online. Botsitzungen werden
bei dieser Suche bewusst übersprungen — sonst würden Bots sich gegenseitig
anmelden und die Warteschlange liefe nie leer. Da ein Bot keinen Client zum
Anklicken hat, reiht `SendLfgUpdateProposal` die Zusage selbst ein.

### Schlachtfeldziele

Die Zielsteuerung — Flaggen, Stützpunkte, Fahrzeuge — ist umgesetzt für
**Arathibecken, Warsongschlucht, Auge des Sturms, Alteractal und Insel der
Eroberung**. In den übrigen Schlachtfeldern kämpfen die Bots, kümmern sich aber
nicht um die Ziele.

**Dieser Code braucht Navigationsdaten, die dieses Repository nicht
mitliefert.** Die Tabelle `aiwaypoints` wird leer angelegt. Bots bewegen sich
im Schlachtfeld entlang eines Wegpunktnetzes, das ein Betreiber im Spiel setzt
und das in die Weltdatenbank zurückgeschrieben wird. Solange diese Zeilen
fehlen, hat die Zielsteuerung nichts, woran sie entlanglaufen könnte — in den
fünf genannten Schlachtfeldern genauso wie überall sonst.

---

## Beute

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

## Datenbank

`sql/base/` enthält die Zusatztabellen, die der Kern braucht — das
Navigationsnetz der Bots, Namensvorräte, Gesprächszeilen und die Zugangsliste
des Werkzeug-Sockets. Der TrinityCore-Grundbestand 7.3.5 liegt nicht bei und
muss von woanders kommen. Ihn **vor** `sql/base` einspielen.

---

## Inhalte

Neben dem Bot-Teilsystem trägt dieser Baum den TrinityCore-Skriptbestand von
Classic bis Legion, Erweiterung für Erweiterung geprüft. Nennenswerte
Instandsetzungen:

| Erweiterung | Arbeit |
|---|---|
| Classic | Buru der Verschlinger, Ossirian der Narbenlose und Viscidus aus leeren Hüllen geschrieben; Ragefire-Schlund, Sturmwind-Verlies und Düsterbruch als Instanzen gebunden |
| Burning Crusade | vier fehlende Instanzskripte; das Schachereignis in Karazhan; der Sonnenbrunnen-Epilog |
| Wrath | die Luftschiffschlacht der Eiskronenzitadelle gegen die Transport-Schnittstelle dieses Kerns neu gebaut |
| Cataclysm | 21 abgeschaltete Skripte wieder aktiviert; der Brunnen der Ewigkeit aus leeren Dateien wiederhergestellt |
| Warlords | Begegnungen der Schwarzfelsgießerei und der Höllenfeuerzitadelle umgesetzt; Grimmschienen-Depot ergänzt |
| Legion | siehe [Fehlerbericht](docs/Fehlerbericht-Legion.md) |

Die Registrierungslage wird maschinell geprüft: Jede Skriptklasse im Baum ist
gebunden, es gibt keine doppelten Skriptnamen, keine leeren Quelldateien, und
jede `.cpp` erreicht den Build.

---

## Kodierung

Der gesamte Quellbaum ist UTF-8. Das war nicht immer so — 26 Dateien des
Bot-Teilsystems waren GBK, einige weitere cp1251. Spielersichtbare Texte, die
früher chinesische Literale waren, liefen über `consoleToUtf8`; die Funktion
ist auf Linux ein Durchreicher und hätte rohe Bytes an den Client geschickt.
Heute sind es schlichte englische UTF-8-Zeichenketten.

---

## Sicherheit

`src/server/bnetserver/bnetserver.key.pem` ist der Entwicklungsschlüssel von
TrinityCore und damit öffentlich bekannt. **Vor dem Betrieb eines erreichbaren
Realms austauschen.** Botkonten werden mit einem festen Kennwort angelegt —
die auth-Datenbank gehört deshalb nicht in ein öffentliches Netz.

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

## Lizenz

GPL v2, von TrinityCore übernommen. Siehe [COPYING](COPYING).
