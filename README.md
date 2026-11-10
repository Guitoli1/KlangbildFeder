# Klangbild Feder

Federhall mit Röhrentreiber im Klangbild-Vintage-Look – AU, VST3 und Standalone für macOS (Logic Pro).

Jede Feder ist als Kette gestreckter Allpässe nachgebildet: Dadurch entsteht das typische
Zwitschern und „Tropfen“ echter Federhallspiralen. Im Sichtfenster schwingen die Federn mit dem Hall.

## Tanks

| Tank | Charakter |
|---|---|
| **Kurz · 2 Federn** | Wie im Combo-Verstärker – knapp, direkt |
| **Lang · 3 Federn** | Klassischer Hallkasten neben dem Amp – voll, zwitschernd |
| **Groß · Studio** | Lange Federn, weicher und dichter – gut für Gesang und Orgel |

## Regler

| Regler | Aufgabe |
|---|---|
| **Dwell** | Wie stark der Röhrentreiber die Federn anfährt – mehr Biss und Sättigung |
| **Mischung** | Hallanteil |
| **Länge** | Nachhallzeit 0,5 – 6 s |
| **Ton** | Helligkeit des Halls |
| **Zwitschern** | Wie stark die Federn „tropfen“ |
| **Vorverzögerung** | 0 – 80 ms Abstand zwischen Ton und Hall |
| **Bassfilter** | Hält den Hall unten sauber (60 – 600 Hz) |
| **Ausgang** | Mit Direktsignal oder nur Hall (für einen Send/Aux) |
| **Stoß** | Wie ein Tritt gegen den Verstärker: die Federn schlagen aneinander |

## Presets

Standard · Blues-Combo · Soul-Gitarre · Surf – nass & tropfend · Gesang – Vintage · Orgel – Raum ·
Dub – tiefe Feder · Snare – Feder · Send – 100 % Hall

## Installation

Über den [Klangbild Installer](https://github.com/Guitoli1/KlangbildInstaller):

```
curl -fsSL https://raw.githubusercontent.com/Guitoli1/KlangbildInstaller/main/install.sh | bash
```

## Build

JUCE 8 / CMake. GitHub Actions baut auf macOS 15 ein Universal Binary (arm64 + x86_64), signiert ad-hoc und prüft mit `auval`.
