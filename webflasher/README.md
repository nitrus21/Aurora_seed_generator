# AURORA · Installer, choisir, démarrer

### Votre carte devient un générateur Bitcoin hors ligne

[**Ouvrir le Web Flasher**](https://nitrus21.github.io/Aurora_seed_generator/) ·
[Découvrir AURORA](../README.md) · [Comparer les versions](CHANGELOG.md)

| Carte exacte | Versions proposées |
| --- | --- |
| **ESP32-2432S028R — Cheap Yellow Display (CYD)** | **1.9.7**, dernière version ; **1.7.5**, ancienne version conservée. |
| **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3** | **2.0.3**, images distinctes pour silicium **1.x** et **3.x**. |

La branche ESP32-2432S028R est terminée en 1.9.7.
Depuis la v2.0.0, les nouvelles fonctionnalités se développent sur la
Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3.

## Installation en quatre étapes

1. Ouvrez le Web Flasher dans Chrome ou Edge sur ordinateur.
2. Choisissez la carte exacte, la version et, sur P4, la révision silicium.
3. Branchez un câble USB de données, cliquez sur **Installer** et choisissez le port.
4. Attendez la confirmation, vérifiez le démarrage, puis déconnectez les données USB.

Le site ne demande aucune seed, clé privée ni mot de passe.
HTTPS est requis ; `localhost` convient aux essais locaux. N’ouvrez pas
directement le fichier HTML. Une installation peut effacer la flash :
ne l’utilisez pas comme sauvegarde d’un portefeuille.

## Choisir la bonne image

Toutes les images `.factory.bin` s’installent à **0x0000**.

| Élément | ESP32-2432S028R : DIO 40 MHz, 4 Mo | Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 : DIO 80 MHz, 32 Mo |
| --- | --- | --- |
| Bootloader | `0x1000` | `0x2000` |
| Partitions | `0x8000` | `0x8000` |
| `boot_app0` | `0xE000` | — |
| Application seule | `0x10000` | `0x10000` |

L’image complète CYD inclut la table de partitions requise.
Les en-têtes P4 limitent chaque image à la famille silicium prévue :
**ne remplacez pas rev1 par rev3, ou inversement**. Le modèle Waveshare `-C`
n’est pas la cible. L’image 3.x est contrôlée par logiciel mais n’a pas été
testée sur carte 3.x ; les applications CYD et P4 1.3 ont été flashées et démarrées.

## Vérifier le téléchargement

Les [empreintes SHA-256](firmware/SHA256SUMS.txt) couvrent les binaires du site.
Comparez le fichier téléchargé à une référence obtenue par un canal de confiance :

```powershell
Get-FileHash -Algorithm SHA256 .\aurora-1.9.7-esp32-2432s028r.factory.bin
```

Sous Linux : `sha256sum fichier.factory.bin`. Sous macOS :
`shasum -a 256 fichier.factory.bin`. Un hash identique ne certifie pas le firmware.

## Après l’installation

Utilisez une alimentation autonome sans données USB. Commencez par un portefeuille
de test sans fonds et suivez le [parcours guidé](../README.md#votre-premier-portefeuille-en-6-étapes).
Préférez la 1.9.7 à l’ancienne 1.7.5 sur ESP32-2432S028R.

> [!WARNING]
> Projet expérimental, sans garantie d’effacement physique à la coupure.
> L’export Electrum privé est **en clair sur microSD** et n’est pas supprimé au verrouillage.

## Maintenance du site

- `tests/release/verify.ps1` : versions, images, partitions, SHA-256 et sélection de la cible.
- `tools/prepare_releases.py` : paquets locaux à liste de fichiers autorisés ; aucun secret ni rapport.
- Le [workflow Pages](../.github/workflows/deploy-webflasher-pages.yml) publie uniquement
  `webflasher/`, après validation, lors d’un push sur `main`.
- Releases : `v1.7.5`, `v1.9.7`, `v2.0.3` ; préparation locale sans publication automatique.
- ESP Web Tools est épinglé à `10.4.0`, actions GitHub par SHA. Le module et ses
  dépendances chargés depuis `unpkg.com` restent une dépendance de confiance.
