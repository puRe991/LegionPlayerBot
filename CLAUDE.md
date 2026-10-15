# Hinweise für Claude

## Client-Daten (DataDir)

Fertig extrahierte Client-Daten für 7.3.5 (26972) liegen auf MEGA:

- https://mega.nz/file/OIsBkYSY#zcbbwcV4uwl1xKC7k4Qs7gNEBpYyS7kofEqOiwk5KtY
- Datei: `DataFiles.rar`, 7 004 937 877 Bytes (~7 GB)

Inhalt nach `ClientData/` entpacken (`DataDir = "./ClientData"` in
`worldserver.conf`); erwartet werden `dbc`, `gt`, `cameras`, `maps`, `vmaps`
und `mmaps`, siehe `docs/SETUP.md`.

Download ohne MEGA-Client: Ein Download am Stück bricht mit HTTP 509
(Bandbreiten-Limit) ab, Range-Anfragen in 32-MB-Stücken funktionieren. Die
Download-URL kommt von `POST https://g.api.mega.co.nz/cs` mit
`[{"a":"g","g":1,"ssl":2,"p":"OIsBkYSY"}]`; die Daten sind AES-128-CTR
verschlüsselt, Schlüssel und IV stehen hinter dem `#` im Link.

Die Daten nicht committen: GitHub lehnt Dateien über 100 MB ab.
