# Fehlerbericht — Legion-Emulatortest (Kette Classic → Legion)

Stand: 2026-08-05 · Zweig `claude/projekt-status-bericht-od9a98` · Build GCC 13.3 / CMake 3.28 / Boost 1.83 / OpenSSL 3.0.13

---

## 0. Was geprüft wurde — und was nicht

Die angeforderten Spieltests (Dämonenjäger einloggen, Artefakt aufwerten, Schlüsselstein setzen, Bosse legen) **konnten nicht ausgeführt werden**. Der Grund ist Befund **DEP-001**: dieses Repository liefert keine Weltdatenbank und kein `ClientData`-Verzeichnis. Der Weltserver bricht beim Start ab, bevor irgendein Skript geladen wird.

Ich habe deshalb **keine** Reproduktionsschritte durchgespielt und **kein** „Actual Result" aus dem laufenden Spiel notiert. Alles unten Stehende ist entweder

* **statisch verifiziert** — aus dem Quelltext belegbar, Zeilennummer angegeben, oder
* **Werkzeugbefund** — Vollbuild, Symboltabelle, Registrierungsabgleich, Kodierungsprüfung.

Wo ein Feld nicht belegbar war, steht das ausdrücklich da. Erfundene Beobachtungen enthält dieser Bericht nicht.

### Tatsächlich ausgeführte Prüfungen

| Prüfung | Ergebnis |
|---|---|
| Vollbuild `make -j$(nproc)` (alle Ziele) | fehlerfrei, `worldserver` gelinkt (80,5 MB) |
| Startlauf `./worldserver -c worldserver.conf` | Abbruch, Exit 1 — siehe DEP-001 |
| `AddSC_*` definiert vs. in `ScriptLoader.cpp` aufgerufen (projektweit) | 1125 definiert / 1126 aufgerufen, 0 echte Lücken |
| Doppelte Skriptnamen (`CreatureScript("…")` u. a.) | 1 Kollision gefunden → LEG-001 |
| Leere Quelldateien projektweit | keine |
| `.cpp` ohne Objektdatei im Build | keine (außer PCH) |
| Nicht-UTF-8-Dateien projektweit | 4 gefunden → LEG-006 |
| Abgeschaltete Registrierungen in `AddSC`-Rümpfen (Legion + Legion-Szenarien) | 29 gefunden, davon 24+5 lebend → LEG-002…005 |

### Systeminventar Legion (Kernserver)

| System | Umfang | Zustand |
|---|---|---|
| Artefaktwaffen | `ArtifactHandler`, 2 Dateien / 460 Zeilen | alle 6 Handler mit echtem Rumpf; Artefaktwissen über `CURRENCY_TYPE_ARTIFACT_KNOWLEDGE`; `ApplyArtifactPowers`/`ApplyArtifactPowerRank` vorhanden |
| Klassenhallen | `Garrison/`, 8 Dateien / 6862 Zeilen | `GARRISON_TYPE_CLASS_ORDER`, Anhänger, Missionen, `StartClassHallUpgrade` implementiert |
| Mythisch+ | `Challenge/`, 4 Dateien / 2039 Zeilen | Schlüsselstein, Timer, `GetAffixes`/`HasAffix`, 12-Wochen-Affixrotation, manuelle Affixe per Konfiguration |
| Weltquests | `QuestData.h/.cpp`, `WorldQuestInfo` | Vorlagen, Rotation, Zonenzuordnung, Belohnungstypen inkl. Artefaktmacht |
| Legendäre Gegenstände | `Player::CalculateLegendaryDropChance`, `GetRateLegendaryDrop`, `UpgradeLegendary` | Kill-Punkte-Modell, Anlegelimit über `CONFIG_PLAYER_LEGION_LEGENDARY_EQUIP_COUNT` |
| PvP | `HonorInfo` (Ehrestufe max. 50), Prestige, `LearnPvpTalent`/`AddPvPTalent`/`IsAreaThatActivatesPvpTalents` | vollständig |
| Sammlungen | `CollectionMgr` (Reittiere, Spielzeug, Erbstücke, Erscheinungsbilder), `TransmogrificationHandler` | vollständig |
| Obliterum | `CONFIG_OBLITERUM_LEVEL_ENABLE/START/MIN/MAX`, ausgewertet in `DB2Stores.cpp:2249/2316` | vorhanden |

### Skriptinventar Legion

150 `.cpp`, 121 322 Zeilen. Alle 5 Schlachtzüge, alle 13 Dungeons und alle 4 Invasionsszenarien gebunden:

| Schlachtzug | Karte | Instanzskript | Bosse |
|---|---|---|---|
| Smaragdgrüner Albtraum | 1520 | ja | 7 (+3 Hilfs-AI) |
| Prüfung der Tapferkeit | 1648 | ja | 3 |
| Nachtfestung | 1530 | ja | 10 |
| Grabmal Sargeras' | 1676 | ja | 9 |
| Antorus | 1712 | ja | 11 (+1) |

Broken Isles/Broken Shore: `Scenario/BrokenIslands` (4269 Zeilen, Karte 1460), `Scenario/AssaultBrokenShore` (Karte 1666), Invasionsszenarien Azsuna/Val'sharah/Höhenbergen/Sturmheim (Karten 1704–1707). Dämonenjäger-Startgebiet: `Legion/mardum.cpp` (2850 Zeilen, Karte 1481).

---

## 1. Sperrender Befund

### DEP-001 — Weltdatenbank und Client-Daten fehlen; Server startet nicht

| Feld | Inhalt |
|---|---|
| **Expansion** | alle |
| **System** | Auslieferung / Betrieb |
| **Subsystem** | Weltdatenbank, `DataDir` |
| **Priorität** | **P0** (kein Betrieb möglich) |
| **Reproduktionsschritte** | 1. `make worldserver` 2. `cd src/server/worldserver` 3. `./worldserver -c worldserver.conf` |
| **Expected Result** | Server lädt DB2-Stores, Karten, Weltdaten und meldet „World initialized". |
| **Actual Result** | **Beobachtet.** Abbruch mit Exit 1. Ab der ersten vorbereiteten Anweisung: `Table 'world.linked_respawn' doesn't exist`, danach dutzende gleichartige Fehler (`world.creature_template`, `world.gameobject`, `world.quest_template` …). `world` enthält 4 Tabellen (`aiwaypoints`, `bottalktext`, `playerbot_online`, `talkstory`) — ausschließlich Playerbot-Zusatztabellen aus `sql/base`. `./ClientData` existiert nicht. `auth.realmlist` fehlt ebenfalls. |
| **Root Cause** | Das Repository liefert nur die Playerbot-SQL (`sql/base`, 16 KB) und `sql/justbot.rar`. Das LegionCore-Weltschema samt Daten und die aus dem Client extrahierten DB2/Karten/vmaps/mmaps sind nicht Bestandteil des Projekts und in keinem Einrichtungsschritt referenziert. |
| **Affected Files** | `sql/base/*`, `docs/SETUP.md`, `src/server/worldserver/worldserver.conf.dist` |
| **Affected Database Tables** | `world.*` (gesamtes Schema), `auth.realmlist`, `auth.account`, `hotfixes.*` |
| **Suggested Fix** | Kein Codefix. Erforderlich: (a) LegionCore-Weltdatenbank (7.3.5) importieren, (b) Playerbot-SQL aus `sql/base` **danach** einspielen, (c) `ClientData` mit `mapextractor`/`vmap4extractor`/`mmaps_generator` aus einem 7.3.5-Client erzeugen und unter `DataDir` ablegen, (d) `auth.realmlist` befüllen. Diese vier Schritte gehören ausdrücklich in `docs/SETUP.md`. |
| **Regression Risk** | keiner (kein Codeeingriff) |
| **Status** | **offen — extern.** Blockiert jede Laufzeitprüfung dieses Berichts. |

---

## 2. Behobene Befunde

### LEG-001 — Skriptnamenkollision `boss_archimonde` (Hyjal ↔ Höllenfeuerzitadelle)

| Feld | Inhalt |
|---|---|
| **Expansion** | TBC + Draenor |
| **System** | Schlachtzüge |
| **Subsystem** | Skriptregistrierung |
| **Priorität** | **P1** |
| **Reproduktionsschritte** | 1. Weltserver starten. 2. Schlacht um den Berg Hyjal betreten, Archimonde (NPC 17968) pullen. 3. Höllenfeuerzitadelle betreten, Archimonde (NPC 91331) pullen. |
| **Expected Result** | Jeder Boss läuft mit seiner eigenen KI. |
| **Actual Result** | **Nicht beobachtet** (siehe DEP-001). Statisch belegt: zwei `CreatureScript("boss_archimonde")` — `Kalimdor/CavernsOfTime/BattleForMountHyjal/boss_archimonde.cpp:237` und `Draenor/HellfireCitadel/boss_hfc_upper.cpp:783`. |
| **Root Cause** | `ScriptMgr::AddScript` indiziert über den Skriptnamen (`GetScriptId(name)`), nicht über die Kreatur. Zwei Registrierungen desselben Namens überschreiben sich; welche gewinnt, hängt von der Ladereihenfolge in `ScriptLoader.cpp` ab. Zusätzlich lag eine ODR-Verletzung vor: derselbe Klassenname `boss_archimonde` mit unterschiedlicher Definition in zwei Übersetzungseinheiten — vom Linker nicht diagnostiziert, Verhalten undefiniert. Die Kollision stammt aus meiner eigenen Umsetzung der Zitadellenbosse in dieser Sitzung. |
| **Affected Files** | `src/server/scripts/Draenor/HellfireCitadel/boss_hfc_upper.cpp:780–916` |
| **Affected Database Tables** | `world.creature_template.ScriptName` für NPC 91331 |
| **Suggested Fix** | Umbenannt in `boss_hfc_archimonde` (C++-Typ und Skriptname). |
| **Regression Risk** | **niedrig** — der Hyjal-Archimonde ist unverändert. Der Zitadellen-Archimonde braucht in der Weltdatenbank `creature_template.ScriptName = 'boss_hfc_archimonde'` für Eintrag 91331. |
| **Status** | **behoben** (Commit `306babf`), Build fehlerfrei, Duplikatprüfung danach leer. |

### LEG-002 — Mikrofeiertag „Un'Goro-Wahn" vollständig abgeschaltet

| Feld | Inhalt |
|---|---|
| **Expansion** | Legion |
| **System** | Weltereignisse |
| **Subsystem** | Mikrofeiertage |
| **Priorität** | **P2** |
| **Reproduktionsschritte** | 1. Spielereignis „Un'Goro-Wahn" aktivieren. 2. Nach Un'Goro-Krater reisen. 3. Klauenmutter Zavas, Tyrantus, Königin Zavra, Dadanga, Sherazin usw. angreifen. |
| **Expected Result** | Die Ereignis-NPCs benutzen ihre Fähigkeiten (Dino-Mojo, Anpassung, Teerschlag, Blüte, Teergruben usw.). |
| **Actual Result** | **Nicht beobachtet.** Statisch belegt: `AddSC_UnGoroMadness()` enthielt 23 auskommentierte Registrierungen und keine einzige aktive. 1579 Zeilen fertiger, kompilierter Code waren zur Laufzeit unerreichbar; die NPCs hätten ausschließlich SmartAI bzw. gar keine KI bekommen. |
| **Root Cause** | Registrierungen im Auslieferungszustand abgeschaltet (kein Kommentar, der einen Grund nennt). Zusätzlich hatte `npc_sherazin` (Zeile 690) **überhaupt keine** Registrierungszeile — auch keine auskommentierte. |
| **Affected Files** | `src/server/scripts/Legion/MicroHolidays/UnGoroMadness.cpp:1555–1580` |
| **Affected Database Tables** | `world.creature_template.AIName`/`ScriptName` für 118271 u. a., `world.game_event` |
| **Suggested Fix** | Alle 23 Registrierungen aktiviert, `RegisterCreatureAI(npc_sherazin)` ergänzt (24 gesamt). |
| **Regression Risk** | **mittel.** Registrierte `CreatureScript`s schlagen in `AI/CreatureAISelector.cpp:38` **SmartAI**, weil `sScriptMgr->GetCreatureAI()` vor `creature->GetAIName()` befragt wird. Falls die Weltdatenbank für diese NPCs bereits SmartAI-Skripte pflegt, gewinnt ab jetzt der C++-Code. Betroffen ist nur der Un'Goro-Mikrofeiertag. |
| **Status** | **behoben** (Commit `306babf`), Build fehlerfrei, Symbole in `libscripts.a` nachgewiesen. |

### LEG-003 — Nachtfestung: Sternzeichen-Mechanik von Sterndeuter Etraeus nicht gebunden

| Feld | Inhalt |
|---|---|
| **Expansion** | Legion |
| **System** | Schlachtzüge |
| **Subsystem** | Nachtfestung — Sterndeuter Etraeus |
| **Priorität** | **P3** |
| **Reproduktionsschritte** | 1. Nachtfestung betreten. 2. Sterndeuter Etraeus legen bis „Große Konjunktion" (Grand Conjunction). 3. Zwei Spieler mit gleichem Sternzeichen zusammenlaufen lassen. |
| **Expected Result** | Gleiche Sternzeichen heben sich auf, unterschiedliche laufen ab und verursachen Schaden. |
| **Actual Result** | **Nicht beobachtet.** Statisch belegt: `//RegisterAreaTriggerAI(at_augur_star_sign);` in `AddSC_boss_star_augur_etraeus()`, während `struct at_augur_star_sign : AreaTriggerAI` (Zeile 1431–1495) vollständig implementiert ist — `ActionOnUpdate` behandelt beide Fälle korrekt. |
| **Root Cause** | Registrierung abgeschaltet. Der bereits vorhandene alternative Pfad `IsValidTarget` steht im selben Typ als Blockkommentar; offenbar wurde beim Umbau auf `ActionOnUpdate` vergessen, die Registrierung wieder zu aktivieren. |
| **Affected Files** | `src/server/scripts/Legion/TheNighthold/boss_star_augur_etraeus.cpp:1522` |
| **Affected Database Tables** | `world.areatrigger_template` / `areatrigger_create_properties` für Zauber 205429 ff. |
| **Suggested Fix** | Registrierung aktiviert. |
| **Regression Risk** | **niedrig** — ein zusätzlicher AreaTrigger-Handler, kein bestehendes Verhalten überschrieben. |
| **Status** | **behoben** (Commit `306babf`). |

### LEG-004 — Frühlingsballonfest: zwei NPC-KIs nicht gebunden

| Feld | Inhalt |
|---|---|
| **Expansion** | Legion (Mikrofeiertag) |
| **System** | Weltereignisse |
| **Subsystem** | Frühlingsballonfest |
| **Priorität** | **P3** |
| **Reproduktionsschritte** | 1. Ereignis aktivieren. 2. Lin Wolkenläufer in der Violetten Feste ansprechen. 3. Wolkenläufer-Express (MoP-Variante) benutzen. |
| **Expected Result** | Beide NPCs reagieren mit ihrer Skript-KI. |
| **Actual Result** | **Nicht beobachtet.** Statisch belegt: `//RegisterCreatureAI(npc_lin_cloudwalker_vh);` und `//RegisterCreatureAI(npc_cloudwalker_express_mop);`, beide Typen im selben Modul definiert und lebend. |
| **Root Cause** | Registrierung abgeschaltet, kein Grund vermerkt. |
| **Affected Files** | `src/server/scripts/Legion/MicroHolidays/SpringBalloonFestival.cpp` (AddSC-Rumpf) |
| **Affected Database Tables** | `world.creature_template.ScriptName` |
| **Suggested Fix** | Beide Registrierungen aktiviert. |
| **Regression Risk** | **niedrig** |
| **Status** | **behoben** (Commit `306babf`). |

### LEG-005 — Zwei OutdoorPvP-Skripte nicht gebunden (Silithus, Tausend Nadeln)

| Feld | Inhalt |
|---|---|
| **Expansion** | Legion (Mikrofeiertage) |
| **System** | Außenwelt-PvP |
| **Subsystem** | „Ruf des Skarabäus", „Bootsparty der Tausend Nadeln" |
| **Priorität** | **P3** |
| **Reproduktionsschritte** | 1. Ereignis 78 bzw. 307 aktivieren. 2. Zone Silithus (1377) bzw. Tausend Nadeln (400) betreten. |
| **Expected Result** | Weltzustände 12952/12953 werden gesendet; Silithyst-Wertung läuft. Im Wasser der Tausend Nadeln greift Zauber 234458 mit Bosswisper. |
| **Actual Result** | **Nicht beobachtet.** Statisch belegt: `//new OutdoorPvP_Silithus();` und `//new OutdoorPvP_ThousandNeedles();`; beide `OutdoorPvPScript`-Ableitungen und die zugehörigen `OutdoorPvP`-Klassen vollständig implementiert. |
| **Root Cause** | Registrierung abgeschaltet. Kein Zonenkonflikt: kein anderes OutdoorPvP-Skript registriert Zone 1377/5695 oder 400 (geprüft gegen `OutdoorPvP/*.cpp` und die übrigen Legion-Skripte). |
| **Affected Files** | `src/server/scripts/Legion/MicroHolidays/CallOfTheScarab.cpp`, `.../ThousandBoatBash.cpp` |
| **Affected Database Tables** | `world.game_event` (78, 307), `character_currency` (1324/1325) |
| **Suggested Fix** | Beide Registrierungen aktiviert. |
| **Regression Risk** | **mittel** — siehe LEG-007, `OutdoorPvPSilithus::SetupOutdoorPvP` führt beim Start Löschanweisungen aus. |
| **Status** | **behoben** (Commit `306babf`); Folgebefund LEG-007 bleibt offen. |

### LEG-006 — Vier Dateien nicht UTF-8 kodiert

| Feld | Inhalt |
|---|---|
| **Expansion** | Legion, Classic, Cataclysm, Custom |
| **System** | Quelltextpflege |
| **Subsystem** | Zeichenkodierung |
| **Priorität** | **P4** (eine Teilmenge P3, siehe unten) |
| **Reproduktionsschritte** | `python3 -c "open(F,'rb').read().decode('utf-8')"` über alle `.cpp`/`.h`. |
| **Expected Result** | Alle Quelldateien dekodieren als UTF-8. |
| **Actual Result** | **Beobachtet.** 4 Treffer: `Legion/invasion_point_argus.cpp` (cp1251), `EasternKingdoms/Deadmines/boss_glubtok.cpp` (cp1251), `EasternKingdoms/ShadowfangKeep/boss_commander_springvale.cpp` (cp1251), `Custom/CustomTalkMenu.cpp` (**GBK**, 5 Zeichenketten — darunter zwei, die als Spielertext ausgegeben werden). |
| **Root Cause** | Beiträge aus russischsprachigen (cp1251) bzw. chinesischsprachigen (GBK) Zweigen ohne Konvertierung übernommen. Bei `CustomTalkMenu.cpp` betrifft das keine Kommentare, sondern **ausgegebene Zeichenketten** — der Client bekommt ungültiges UTF-8 (deshalb dort P3, nicht P4). |
| **Affected Files** | die vier genannten |
| **Affected Database Tables** | keine |
| **Suggested Fix** | Alle nach UTF-8 konvertiert; Kommentare und Spielertexte ins Deutsche übersetzt („Ehre erforderlich", „Nicht genug Ehrenpunkte!", „Funktionsedelstein", „Erweiterte Funktionen", Arena-Hinweistext). |
| **Regression Risk** | **niedrig** — bytegenaue Ersetzung, verifiziert; Build danach fehlerfrei. |
| **Status** | **behoben** (Commit `306babf`). |

---

## 3. Offene Befunde

### LEG-007 — `OutdoorPvPSilithus` löscht bei jedem Serverstart Spielerdaten

| Feld | Inhalt |
|---|---|
| **Expansion** | Legion (Mikrofeiertag) |
| **System** | Außenwelt-PvP |
| **Subsystem** | „Ruf des Skarabäus" |
| **Priorität** | **P2** (Datenverlust, eng begrenzt) |
| **Reproduktionsschritte** | 1. Ereignis 78 deaktiviert lassen. 2. Weltserver starten. 3. `SELECT * FROM character_currency WHERE currency IN (1324,1325);` |
| **Expected Result** | Ein Zurücksetzen der Feiertagswährung findet **beim Ende des Ereignisses** statt, nicht bei jedem Start. |
| **Actual Result** | **Nicht beobachtet** (DEP-001). Statisch belegt, `CallOfTheScarab.cpp:35–41`: `SetupOutdoorPvP()` führt bei jedem Start, sofern Ereignis 78 nicht aktiv ist, `DELETE FROM character_currency WHERE currency in (1325,1324)` sowie zwei `DELETE` auf `character_queststatus` / `character_queststatus_rewarded` für die Quests 45785/45787 aus — **serverweit, über alle Charaktere**. |
| **Root Cause** | Der Zurücksetzvorgang hängt an der Einrichtungsfunktion statt am Ereignisende (`HandleGameEventEnd`). Solange das Ereignis aus ist, läuft er bei jedem Neustart erneut. |
| **Affected Files** | `src/server/scripts/Legion/MicroHolidays/CallOfTheScarab.cpp:35–41` |
| **Affected Database Tables** | `characters.character_currency` (1324, 1325), `characters.character_queststatus`, `characters.character_queststatus_rewarded` (45785, 45787) |
| **Suggested Fix** | Zurücksetzen nach `HandleGameEventEnd(78)` verlagern, oder über einen Weltzustand („zuletzt zurückgesetzter Ereignisdurchlauf") absichern, sodass es je Durchlauf höchstens einmal läuft. |
| **Regression Risk** | **niedrig**, wenn der Vorgang verlagert wird — der Umfang der Löschung bleibt gleich. |
| **Status** | **offen.** Ich habe die Registrierung aktiviert (LEG-005), aber die Löschlogik **nicht** angefasst — sie ist die erklärte Absicht des ursprünglichen Autors, und die Vorgabe „keinen funktionierenden Code ohne Abhängigkeitsanalyse ändern" gilt. Vor dem Produktivbetrieb sollte das entschieden werden. |

### LEG-008 — Artefakterwerbs-Szenarien nur für 7 von 12 Klassen

| Feld | Inhalt |
|---|---|
| **Expansion** | Legion |
| **System** | Artefaktwaffen |
| **Subsystem** | Erwerbsszenarien |
| **Priorität** | **P2** |
| **Reproduktionsschritte** | 1. Todesritter/Druide/Jäger/Magier/Hexenmeister auf Stufe 98+ bringen. 2. Klassenhallen-Einführungsquest annehmen. 3. Zum Artefaktszenario reisen. |
| **Expected Result** | Jede der 36 Spezialisierungen hat ein eigenes Erwerbsszenario. |
| **Actual Result** | **Nicht beobachtet.** Statisch belegt: `Scenario/Artifacts/` enthält Szenarien für Dämonenjäger (**nur Rachsucht**), Mönch, Paladin, Priester (**nur Tuure**), Schurke (3), Schamane (**nur Sharas'dal**), Krieger (2 + Einführung). Für **Todesritter, Druide, Jäger, Magier, Hexenmeister** existiert kein einziges. Insgesamt 24 Dateien / 12 gebundene Instanzkarten gegenüber 36 benötigten Ketten. |
| **Root Cause** | Unvollständige Portierung des Originalprojekts. Es fehlen sowohl die Szenarioskripte als auch die zugehörigen `InstanceMapScript`-Bindungen. |
| **Affected Files** | `src/server/scripts/Scenario/Artifacts/**` (fehlende Unterbäume), `src/server/scripts/ScriptLoader.cpp` |
| **Affected Database Tables** | `world.instance_template.script`, `world.scenarios`, `world.scenario_step`, `world.quest_template` (Artefaktquestketten), `world.creature_template` |
| **Suggested Fix** | Je fehlender Spezialisierung ein `instance_<artefakt>.cpp` (Karte aus `world.instance_template`) plus Szenarioskript nach dem Muster von `Scenario/Artifacts/Rogue/Kingslayers/`. Ohne Weltdatenbank (DEP-001) sind die Kartennummern und NPC-Einträge nicht überprüfbar; sie zu erfinden wäre wertlos. |
| **Regression Risk** | **niedrig** — reine Ergänzung. |
| **Status** | **offen — blockiert durch DEP-001.** |

### LEG-009 — Keine C++-Skripte für die Klassenhallen-Kampagnen

| Feld | Inhalt |
|---|---|
| **Expansion** | Legion |
| **System** | Klassenhallen |
| **Subsystem** | Kampagne, Hallen-NPCs |
| **Priorität** | **P3** |
| **Reproduktionsschritte** | 1. Klassenhalle betreten. 2. Kampagnenquestkette starten. 3. Anhänger rekrutieren, Ausbau der Halle beginnen. |
| **Expected Result** | Kampagnen-Zwischensequenzen, Hallenaufwertungen und Anhängerrekrutierung laufen ab. |
| **Actual Result** | **Nicht beobachtet.** Statisch belegt: Suche nach `classhall`/`OrderHall`/`class_hall` in `src/server/scripts` liefert nur `Commands/cs_npc.cpp` und `Spells/spell_generic.cpp` — kein einziges Klassenhallenskript. Der Kernserver-Teil ist dagegen vorhanden (`Garrison/`, 6862 Zeilen, `GARRISON_TYPE_CLASS_ORDER`, `StartClassHallUpgrade`, Anhänger- und Missionslogik inklusive fest verdrahteter Anhänger-IDs in `Garrison.cpp:1481–1507`). |
| **Root Cause** | Klassenhallen sind in Legion überwiegend datengetrieben (`garr_*`-DB2 + Quests/Conversations der Weltdatenbank). Ob hier tatsächlich Skripte fehlen oder nur die Daten, lässt sich ohne Weltdatenbank nicht entscheiden. |
| **Affected Files** | — (Neuanlage unter `src/server/scripts/Legion/ClassHalls/`) |
| **Affected Database Tables** | `world.quest_template`, `world.conversation_*`, `world.creature_text`, `world.scenarios`; DB2 `GarrTalent`, `GarrTalentTree`, `GarrFollower`, `GarrBuilding` |
| **Suggested Fix** | Nach Import der Weltdatenbank prüfen, welche Kampagnenschritte ohne Skript hängenbleiben, und gezielt nachrüsten. Vorher keine sinnvolle Aussage möglich. |
| **Regression Risk** | — |
| **Status** | **offen — nicht entscheidbar ohne DEP-001.** |

### LEG-010 — `isClassHallMap` kennt nur 3 von 12 Klassenhallenkarten (toter Code)

| Feld | Inhalt |
|---|---|
| **Expansion** | Legion |
| **System** | Klassenhallen |
| **Subsystem** | Kartenprüfung |
| **Priorität** | **P4** |
| **Reproduktionsschritte** | — (statisch) |
| **Expected Result** | Eine Hilfsfunktion mit diesem Namen erkennt alle Klassenhallenkarten. |
| **Actual Result** | **Beobachtet.** `Garrison/GarrisonGlobal.h:50` gibt `ID == 1513 \|\| ID == 1479 \|\| ID == 1107` zurück. Legion hat 12 Klassenhallen. **Die Funktion wird nirgends aufgerufen** — projektweite Suche liefert nur die Definition. |
| **Root Cause** | Rest einer früheren Fassung; die tatsächliche Typbestimmung läuft über `getGarrisoneTypeBySite()` (`Garrison.cpp:60`) anhand der Site-ID, nicht über die Karte. |
| **Affected Files** | `src/server/game/Garrison/GarrisonGlobal.h:50–53` |
| **Affected Database Tables** | keine |
| **Suggested Fix** | Entweder entfernen oder um die restlichen 9 Karten ergänzen. Aktuell keine Laufzeitwirkung — aber eine Falle für den nächsten, der sie benutzt. |
| **Regression Risk** | **keiner** beim Entfernen (nicht referenziert). |
| **Status** | **offen — bewusst nicht angefasst.** Ohne Weltdatenbank kann ich die 9 fehlenden Kartennummern nicht belegen, und geraten wäre schlechter als gar nichts. |

### LEG-011 — Todesgrubenminen: Glubtoks Flammenwand fehlt

| Feld | Inhalt |
|---|---|
| **Expansion** | Cataclysm |
| **System** | Dungeons |
| **Subsystem** | Todesminen (heroisch) — Glubtok |
| **Priorität** | **P3** |
| **Reproduktionsschritte** | 1. Todesminen heroisch betreten. 2. Glubtok unter 50 % Gesundheit bringen (Phasenwechsel Feuer/Frost). |
| **Expected Result** | Die Flammenwände (NPC 48975/48976/49039/49041/49042) rotieren durch den Raum und zwingen zum Ausweichen. |
| **Actual Result** | **Nicht beobachtet.** Statisch belegt: `boss_glubtok.cpp:4` trägt den Vermerk „todo: Flammenwand implementieren"; die NPC-Einträge sind als Enum vorhanden (Zeilen 45–49), werden im Skript aber nirgends beschworen oder bewegt. |
| **Root Cause** | Vom ursprünglichen Autor bewusst ausgelassen. |
| **Affected Files** | `src/server/scripts/EasternKingdoms/Deadmines/boss_glubtok.cpp` |
| **Affected Database Tables** | `world.creature_template` (48975, 48976, 49039, 49041, 49042), `world.creature` (Spawns), `world.waypoint_data` |
| **Suggested Fix** | Beschwörung und Wegpunktbewegung der fünf Flammenwand-NPCs in der zweiten Phase. Positionen und Wege sind ohne Weltdatenbank nicht ermittelbar. |
| **Regression Risk** | **niedrig** — der übrige Kampf bleibt unberührt. |
| **Status** | **offen — blockiert durch DEP-001.** Der Kommentar wurde von cp1251 nach UTF-8 übersetzt, der Befund bleibt bestehen. |

### LEG-012 — Burg Schattenfang: `EVENT_FORSAKEN_ABILITY` ohne Wirkung

| Feld | Inhalt |
|---|---|
| **Expansion** | Cataclysm |
| **System** | Dungeons |
| **Subsystem** | Burg Schattenfang — Kommandant Springvale / Gequälter Offizier |
| **Priorität** | **P3** |
| **Reproduktionsschritte** | 1. Burg Schattenfang betreten. 2. Kommandant Springvale pullen. 3. 30 Sekunden warten. |
| **Expected Result** | Der Gequälte Offizier wirkt alle 10–30 Sekunden seine Verlassenen-Fähigkeit. |
| **Actual Result** | **Nicht beobachtet.** Statisch belegt: `boss_commander_springvale.cpp:313–315` — das Ereignis wird in `Reset()` (Zeile 292) alle 10–30 s eingeplant, der `case`-Zweig enthält jedoch nur `//todo` und `break;`. Der Timer läuft, es passiert nichts. |
| **Root Cause** | Die Zauber-ID wurde nie ermittelt. |
| **Affected Files** | `src/server/scripts/EasternKingdoms/ShadowfangKeep/boss_commander_springvale.cpp:313–315` |
| **Affected Database Tables** | `world.creature_template` (Gequälter Offizier), DB2 `Spell` |
| **Suggested Fix** | Zauber-ID aus dem Schlachtzugsjournal bzw. `spell_dbc` bestimmen und `DoCastVictim` ergänzen. Ohne DB2-Daten (DEP-001) nicht verifizierbar; eine geratene ID wäre schlechter als der leere Zweig. |
| **Regression Risk** | **niedrig** |
| **Status** | **offen — blockiert durch DEP-001.** Kommentar nach UTF-8 übersetzt. |

### LEG-013 — Iskar und Socrethar (Höllenfeuerzitadelle) nur als Gerüst

| Feld | Inhalt |
|---|---|
| **Expansion** | Draenor |
| **System** | Schlachtzüge |
| **Subsystem** | Höllenfeuerzitadelle — Verbannter Iskar, Socrethar der Ewige |
| **Priorität** | **P3** |
| **Reproduktionsschritte** | 1. Höllenfeuerzitadelle betreten. 2. Iskar bzw. Socrethar pullen. |
| **Expected Result** | Der Kampf verläuft mit den Fähigkeiten des jeweiligen Bosses (Iskar: Auge von Anzu, Phantomsicht; Socrethar: Schattengeschmiedeter Verteidiger). |
| **Actual Result** | **Nicht beobachtet.** Statisch belegt: beide KIs enthalten Kampfrahmen (Aggro, Kill, Tod, Bosszustand), aber keine Fähigkeitenliste. Der Dateikopf sagt das ausdrücklich. |
| **Root Cause** | Die Abfrage des Schlachtzugsjournals lieferte für diese beiden Bosse keine Fähigkeitenliste. Ich habe bewusst kein Verhalten erfunden. |
| **Affected Files** | `src/server/scripts/Draenor/HellfireCitadel/boss_hfc_upper.cpp` |
| **Affected Database Tables** | `world.creature_template` (95067, 91769), DB2 `JournalEncounter`, `JournalEncounterSection` |
| **Suggested Fix** | Fähigkeiten aus dem DB2-Journal des 7.3.5-Clients ableiten, sobald `ClientData` vorliegt. |
| **Regression Risk** | **niedrig** — der Encounter-Rahmen (Bossleiste, Sperre, Fortschritt) funktioniert bereits, die Ergänzung fügt nur Zauber hinzu. |
| **Status** | **offen — dokumentiert, blockiert durch DEP-001.** |

### LEG-014 — Zeitbedarf offener `//todo`/`FIXME`-Stellen

| Feld | Inhalt |
|---|---|
| **Expansion** | alle |
| **System** | Quelltextpflege |
| **Subsystem** | offene Vermerke |
| **Priorität** | **P4** (Bestandsaufnahme) |
| **Reproduktionsschritte** | `grep -rniE "//\s*(todo\|fixme\|hack)" --include=*.cpp --include=*.h src/server/scripts` |
| **Expected Result** | — |
| **Actual Result** | **Beobachtet.** 154 Stellen projektweit, davon 27 in Legion. Nennenswert: `mardum.cpp:1740` („Bedrohung wird bei den NPC-Dämonenjägern durch den Fraktionswechsel nicht korrekt zurückgesetzt"), `mardum.cpp:1860/1881`, `mardum.cpp:1966` („DBC-Werte statt fest verdrahteter benutzen"), `boss_high_botanist_telarn.cpp:816/1027/1771` (drei Zeitangaben ausdrücklich als ungeprüft markiert). |
| **Root Cause** | Ursprüngliche Autoren. |
| **Affected Files** | siehe Suche |
| **Affected Database Tables** | verschieden |
| **Suggested Fix** | Kein Sammeleingriff. Die Botaniker-Tel'arn-Zeiten und der Mardum-Bedrohungsfehler sind Kandidaten für die nächste Runde, sobald ein Testrealm läuft. |
| **Regression Risk** | — |
| **Status** | **offen — als Bestandsaufnahme geführt.** |

---

## 4. Regressionslage der bisherigen Sitzungsarbeit (Classic → Draenor)

Die Kette wurde nicht bespielt (DEP-001), sondern mit denselben statischen Prüfungen erneut über das **gesamte** Projekt gefahren, mit denen die Befunde je Erweiterung ursprünglich gefunden wurden:

| Prüfung | Classic → Draenor | Bewertung |
|---|---|---|
| Kompilierbarkeit aller Ziele | fehlerfrei | keine Regression |
| `AddSC`-Definitionen ohne Aufruf | 0 | keine Regression |
| `AddSC`-Aufrufe ohne Definition | 0 echte (`AddSC_alterac_mountains` ist in `ScriptLoader.cpp:990/2338` selbst auskommentiert und passt zur ebenfalls auskommentierten Definition) | keine Regression |
| Leere Quelldateien | 0 (die vier zuvor leeren Dateien der Sitzung sind gefüllt) | behoben geblieben |
| `.cpp` ohne Objektdatei | 0 | keine Regression |
| Doppelte Skriptnamen | 1 → behoben (LEG-001) | **Regression durch meine eigene WoD-Arbeit, jetzt beseitigt** |
| Nicht-UTF-8-Dateien | 4 → 0 | behoben |
| Instanzkartenbindungen | alle geprüften Karten gebunden, inkl. der in dieser Sitzung angelegten (34, 389, 429, 1208) und der WoD-Raids | keine Regression |

Der in dieser Sitzung eingeführte Kernumbau `SCR_MAP_BGN_INSTANCE` (`Scripting/ScriptMgr.cpp`) bleibt der Punkt mit der größten Reichweite: er betrifft **jede** Instanz im Spiel. Rückwärtskompatibel entworfen (Karten ohne `instance_template.script` verhalten sich wie zuvor), aber **auf einem echten Realm nicht verifiziert** — das gehört an den Anfang des ersten Testlaufs nach dem DB-Import.

---

## 5. Priorisierte Reihenfolge

| Rang | ID | Priorität | Aufwand |
|---|---|---|---|
| 1 | DEP-001 | P0 | extern — Weltdatenbank + `ClientData` beschaffen |
| 2 | LEG-007 | P2 | klein — Löschvorgang ans Ereignisende hängen |
| 3 | LEG-008 | P2 | groß — 5 Klassen, ~10 Szenarien |
| 4 | LEG-011, LEG-012, LEG-013 | P3 | mittel — je Boss ein Mechanikblock |
| 5 | LEG-009 | P3 | erst nach DB-Import entscheidbar |
| 6 | LEG-010, LEG-014 | P4 | Aufräumen |

**Behoben in dieser Runde:** LEG-001 (P1), LEG-002 (P2), LEG-003 (P3), LEG-004 (P3), LEG-005 (P3), LEG-006 (P4/P3).
