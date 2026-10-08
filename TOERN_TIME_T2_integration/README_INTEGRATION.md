# TŒRN TIME T2: Integration

Basis: Soundpauli/toern Main, Commit `924fe4d36fc667a36b7fae56b2c58d634dbd5ae2`.
Dieser Stand enthält den bereits bestätigten Sampler-Interpolationsfix. Der Patch ergänzt darauf TIME T2; der Interpolationsfix bleibt erhalten.

## Inhalt

- `TOERN_TIME_T2.patch`: alle sechs Projektänderungen gegen den Basis-Commit.
- `project-files/`: dieselben sechs Dateien als vollständige Endfassungen, mit relativen Projektpfaden.
- `arduino-libraries/`: vollständig vorbereitete Bibliotheken für Arduino IDE: TeensyVariablePlayback 1.0.16 und TeensyAudioSampler 1.0.7, einschließlich der bisherigen TŒRN-Anpassungen.
- `TOERN_v3.01f_T2_TIME_816MHz.hex`: die bereits gelieferte T2-Testfirmware.
- `reference/platformio.ini`: unveränderte Build-Konfiguration als Referenz, nicht Teil des Patches.
- `source-checksums.json`: Basis-Commit und SHA-256 der sechs Projektdateien.

## PlatformIO

Im eigenen TŒRN-Projekt zuerst den Arbeitsstand sichern. Anschließend im Projektverzeichnis:

```sh
git apply --check /pfad/TOERN_TIME_T2.patch
git apply /pfad/TOERN_TIME_T2.patch
pio run -t clean
pio run -e teensy41
```

Das Pre-Build-Skript übernimmt die Reader-Änderungen und beide WSOLA-Header automatisch in die gepinnte TeensyVariablePlayback-Bibliothek. Es ergänzt dort auch die benötigten Methoden im Player. `arduino-libraries/` wird bei PlatformIO nicht zusätzlich installiert.

Der Patch ist für den genannten Main-Stand gedacht, nicht für bereits eingebauten T1-/T2-Code. Bei einer neueren oder individuell veränderten Basis und einem fehlgeschlagenen `--check` müssen die betroffenen Stellen zusammengeführt werden. Vollständige Quelldateien nicht ungeprüft über neuere eigene Änderungen kopieren.

## Arduino IDE

1. Die sechs Dateien aus `project-files/` anhand ihrer relativen Pfade in das eigene TŒRN-Projekt übernehmen bzw. mit eigenen Änderungen zusammenführen. `toern_wsola.cpp` gehört neben `toern.ino`; die Header gehören nach `src/`.
2. Die bisher installierten TeensyVariablePlayback- und TeensyAudioSampler-Bibliotheken sichern und aus den aktiven Arduino-Bibliotheksverzeichnissen entfernen. Sicherungskopien außerhalb dieser Verzeichnisse aufbewahren, damit Arduino sie nicht als weitere Bibliotheken auswählt.
3. Die beiden vollständigen Ordner unter `arduino-libraries/` in das Arduino-Sketchbook unter `libraries/` kopieren. Projekt-Header allein reichen bei Arduino IDE nicht: der tatsächlich verwendete Bibliotheks-Reader und `playresmp.h` müssen ebenfalls aktualisiert sein. Im Paket ist das bereits erledigt.
4. Teensy 4.1, 816 MHz, USB MIDI + Audio + Serial und die Optimierung Smallest Code (`-Os`) wählen. Vollständig neu bauen. In der ausführlichen Build-Ausgabe prüfen, dass Arduino die gerade installierten Bibliotheksordner verwendet.

Andere Abhängigkeiten und Board-Einstellungen des Hauptprojekts bleiben erforderlich. Die beiliegende HEX wurde mit PlatformIO Teensy 6.0.0, Teensyduino-Core 1.62 und GCC 15.2.1 gebaut. Andere Toolchain-Versionen können andere Speicherwerte ergeben.

## Bedienung

RDO-Filterseite, Encoder 4: vollständiger Name `TIME`, Kurzlabel `T`.

| Einstellung | Verhalten |
|---|---|
| −10 % bis −0 % | WSOLA an, 90 % bis 100 % der Originallänge |
| OFF in der Mitte | Bisheriger Sampler: Pitch und Länge gekoppelt |
| +0 % bis +10 % | WSOLA an, 100 % bis 110 % der Originallänge |

1-%-Schritte. −0 % und +0 % sind bewusst getrennte aktive Nullstellungen. Beide erhalten die Samplelänge trotz Pitch. Nach Neustart steht TIME auf OFF. Die Einstellung gilt pro Sample-Stimme und wird in dieser Testversion nicht in EEPROM/Patterns gespeichert.

Die Prozentwerte beziehen sich auf die Länge, nicht auf BPM. Für eine Tempoanpassung gilt: `Längenänderung [%] = 100 × (Original-BPM / Ziel-BPM − 1)`. Beispiel 115 → 120 BPM: etwa −4,17 %, mit diesem Regler näherungsweise −4 %. TIME ändert weder Sequencer-BPM noch MIDI-Clock und bietet keine automatische Beat-Synchronisation.

## Umfang und Grenzen

- Mono-Samples, One-Shot, Reverse und Trim. Pitch-Bereich 0,5× bis 2×, entsprechend −12 bis +12 Halbtönen.
- Loops, Dual-Head-Loop-Crossfades, Stereo und Pitch außerhalb dieses Bereichs verwenden weiterhin den normalen Sampler, auch wenn TIME im UI aktiv ist.
- Die vorhandenen ADSR-, MIDI-Note-Off-, Filter- und Lautstärkepfade wirken weiter. Eine Hüllkurve kann einen Ton vor dem Sampleende beenden.
- WSOLA arbeitet direkt mit den bestehenden RAM-Samples. Keine SD-Streaming-Erweiterung und kein neuer Anti-Aliasing-Filter.
- Änderungen innerhalb des aktiven TIME-Bereichs passen die verbleibende Laufzeit an, ohne die Aufnahme neu zu starten. Umschalten auf OFF bzw. über OFF hinweg wechselt den Wiedergabepfad; dabei ist kein nahtloser Übergang garantiert.
- Keine neue Firmware-Diagnose oder Audio-IRQ-Serial-Ausgabe.
- Die maximale Zahl gleichzeitig aktiver WSOLA-Stimmen und die Klangqualität bei verschiedenen Aufnahmen müssen auf dem Gerät beurteilt werden.

## Verifikation

T2 wurde erfolgreich vollständig gebaut: 816 MHz, `-Os`, USB MIDI + Audio + Serial. RAM1: 97.888 Byte frei für lokale Variablen; RAM2: 466.976 Byte statisch frei vor dynamischen Allokationen.

Softwaretests mit AddressSanitizer, UndefinedBehaviorSanitizer und Float-Cast-Prüfung: 1.260 bestehende Sampler-Randfälle; 99 ursprüngliche WSOLA-Randfälle; weitere 770 Kombinationen für die 22 aktiven TIME-Positionen; Reverse/Trim/Retrigger; bitgenaue neutrale Wiedergabe; klassische Fallback-Pfade; laufender Wechsel von 0 % auf +10 % ohne Wiedergabe des Sampleanfangs.

Numerische Sinustests: Ein Zwei-Sekunden-Sample ergab bei −10 % exakt 1,8 s, bei ±0 % exakt 2,0 s und bei +10 % exakt 2,2 s, unabhängig von der getesteten Tonhöhe. Die Ausgabelängen werden auf ganze Audiosamples gerundet. Diese Softwaretests ersetzen keine Messung der CPU-Last oder Klangbeurteilung auf dem Teensy.

## Für Cursor

> Integriere TOERN_TIME_T2.patch in meinen aktuellen TŒRN-Code. Basis des Patches ist Main 924fe4d. Prüfe zuerst git apply --check; bei Abweichungen führe nur die sechs betroffenen Dateien mit meinen neueren Änderungen zusammen. Behalte den vorhandenen Sampler-Interpolationsfix, 816 MHz, -Os und USB MIDI+Audio+Serial. Übernimm keine Firmware-Diagnose. Bei Arduino IDE müssen zusätzlich die beiden vorbereiteten Bibliotheken installiert werden; bei PlatformIO übernimmt das Pre-Build-Skript die Bibliotheksanpassungen. Baue danach vollständig neu und melde eventuelle Konflikte oder Build-Fehler.
