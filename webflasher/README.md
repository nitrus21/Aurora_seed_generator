# AURORA Web Flasher

| Appareil | Versions proposées |
| --- | --- |
| **ESP32-2432S028R — Cheap Yellow Display (CYD)** | **1.9.4**, **1.9.3** et **1.7.5** |
| **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3** | **2.0.0**, image distincte pour silicium **1.x** ou **3.x** |

Le développement du CYD est terminé en 1.9.4.
À partir de la v2.0.0, AURORA évolue sur le P4 pour davantage de fonctionnalités.

## Installation

1. Ouvrez le [Web Flasher](https://nitrus21.github.io/Aurora_seed_generator/) dans Chrome ou Edge sur ordinateur.
2. Choisissez l’appareil, la version et, sur P4, la bonne révision silicium.
3. Branchez un câble USB de données, cliquez sur **Installer**, puis choisissez le port.
4. Attendez la fin de l’écriture. Vérifiez le démarrage, puis déconnectez les données USB.

HTTPS requis (ou `localhost` pour les essais locaux). Une ouverture directe du fichier HTML ne convient pas.

## Images et contrôles

Toutes les images `.factory.bin` s’installent à **0x0000**.

| Élément | CYD : DIO 40 MHz, 4 Mo | P4 : DIO 80 MHz, 32 Mo |
| --- | --- | --- |
| Bootloader | `0x1000` | `0x2000` |
| Partitions | `0x8000` | `0x8000` |
| `boot_app0` | `0xE000` | — |
| Application | `0x10000` | `0x10000` |

CYD 1.9.3 : installer aussi la nouvelle table de partitions, incluse dans l’image fusionnée.
P4 : les en-têtes limitent les images aux révisions silicium 1.x ou 3.x prévues.

- [Empreintes SHA-256](firmware/SHA256SUMS.txt)
- [Notes courtes des versions](CHANGELOG.md)
- Contrôles locaux : `./tests/release/verify.ps1`.
- Paquets locaux après compilation : `python tools/prepare_releases.py` après vérification des images Web Flasher ; sortie dans `tmp/release-candidates/`.

## Publication

Le [workflow GitHub Pages](../.github/workflows/deploy-webflasher-pages.yml) publie uniquement
`webflasher/`, après validation des images, lors d’un push sur `main` ou d’un lancement manuel.
Les paquets GitHub sont séparés : `v1.9.4` pour le CYD et `v2.0.0` pour le P4.
La préparation locale ne publie rien.

## Précautions

- Vérifiez la cible et les empreintes avant installation.
- Le site ne demande aucune seed, clé privée ou mot de passe de portefeuille.
- L’installation peut effacer la flash ; elle ne doit pas servir à sauvegarder un portefeuille.
- Utilisez ensuite une alimentation autonome sans données USB.
- La 1.7.5 ne contient pas les corrections récentes : préférez la 1.9.4 sur CYD.
- ESP Web Tools est épinglé en `10.4.0`, les actions GitHub par SHA ; le composant chargé depuis `unpkg.com` reste une dépendance de confiance.
