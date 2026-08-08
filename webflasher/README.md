# AURORA Web Flasher

Installeur Web en français pour **AURORA Seed Generator 1.7.5**, destiné exclusivement à l’ESP32-2432S028R.

## Utilisation

Le Web Flasher doit être servi depuis une adresse HTTPS, par exemple avec GitHub Pages. Ouvrez ensuite `index.html` avec Chrome ou Microsoft Edge sur ordinateur, branchez l’ESP32 avec un câble USB de données et utilisez le bouton **Installer AURORA v1.7.5**.

Une ouverture directe de `index.html` depuis l’Explorateur Windows ne permet pas d’utiliser Web Serial.

## Disposition réelle de la flash

| Élément | Offset |
|---|---:|
| `bootloader.bin` | `0x1000` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xE000` |
| `firmware.bin` | `0x10000` |

ESP Web Tools utilise l’image fusionnée `firmware/aurora-1.7.5-esp32-2432s028r.factory.bin` à l’offset `0x0000`. Elle est préparée en mode DIO, à 40 MHz, pour une flash de 4 Mo.

Les empreintes SHA-256 sont disponibles dans `firmware/SHA256SUMS.txt`.

## Sécurité

- Le flashage se déroule localement entre le navigateur et le port USB.
- Le site ne demande et ne reçoit aucune seed ni clé privée.
- Vérifiez l’empreinte SHA-256 avant publication ou distribution.
- Ne débranchez jamais l’appareil pendant l’écriture.
- Une nouvelle installation peut effacer les données déjà présentes sur la flash.
