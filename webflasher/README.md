# AURORA Web Flasher

Installeur Web en français pour deux modèles :

- **ESP32-2432S028R — Cheap Yellow Display (CYD)** : versions **1.9.2** et **1.7.5**.
- **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3** : version **1.9.2**, avec une image par famille de silicium **1.x** ou **3.x**.

Le code Waveshare ESP32-P4 **2.0.0** en développement est documenté dans le
[guide d'utilisation](../README.md#p4-200--mot-de-passe-uniquement).
Les images **1.9.2** utilisent le mot de passe à l'ouverture, puis le PIN pour
les consultations privées.

## Utilisation

Ouvrez le [Web Flasher AURORA sur GitHub Pages](https://nitrus21.github.io/Aurora_seed_generator/) avec Chrome ou Microsoft Edge sur ordinateur. Sélectionnez d'abord la carte et la version exacte, branchez-la avec un câble USB de données, puis utilisez le bouton **Installer AURORA**. Une copie auto-hébergée doit également être servie en HTTPS.

Une ouverture directe de `index.html` depuis l’Explorateur Windows ne permet pas d’utiliser Web Serial.

## Images proposées

| Choix | État | Image |
|---|---|---|
| ESP32-2432S028R 1.9.2 | image publiée ; correctif 1.9.3 local non publié | `aurora-1.9.2-esp32-2432s028r.factory.bin` |
| ESP32-2432S028R 1.7.5 | disponible | `aurora-1.7.5-esp32-2432s028r.factory.bin` |
| Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 — silicium 1.x | 1.9.2 finale | `aurora-1.9.2-esp32-p4-rev1.factory.bin` |
| Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 — silicium 3.x | 1.9.2 finale | `aurora-1.9.2-esp32-p4-rev3.factory.bin` |

Les images Waveshare ESP32-P4 1.x et 3.x correspondent à des révisions de silicium distinctes ; leurs en-têtes contrôlent les révisions acceptées.

## Disposition réelle de la flash CYD

| Élément | Offset |
|---|---:|
| `bootloader.bin` | `0x1000` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xE000` |
| `firmware.bin` | `0x10000` |

ESP Web Tools utilise l’image fusionnée choisie à l’offset `0x0000`. Les images CYD sont préparées en mode DIO, à 40 MHz, pour une flash de 4 Mo.

Pour le P4, le bootloader est à `0x2000`, les partitions à `0x8000` et l'application à `0x10000` dans une image fusionnée prévue pour une flash de 32 Mo.

Les empreintes SHA-256 sont disponibles dans [firmware/SHA256SUMS.txt](firmware/SHA256SUMS.txt). Depuis la racine du dépôt, `./tests/release/verify.ps1 -ReleasedArtifactsOnly` vérifie le manifeste publié, les versions, les octets aux offsets prévus et les empreintes affichées, indépendamment du code P4 2.0.0 en développement.

## Publication GitHub Pages

Le workflow [Deploy AURORA Web Flasher to GitHub Pages](../.github/workflows/deploy-webflasher-pages.yml) publie le dossier `webflasher/` après un push sur `main` qui modifie ce dossier ou le workflow. Il peut aussi être lancé manuellement depuis GitHub Actions. Un commit local seul ne déclenche aucun déploiement.

Le composant ESP Web Tools est verrouillé sur la version exacte `10.4.0` et la
page applique une politique CSP restrictive. Les GitHub Actions sont épinglées
par SHA de commit. Le chargement du composant depuis `unpkg.com` reste une
dépendance réseau de confiance ; une distribution à menace renforcée doit
l'auto-héberger et vérifier son contenu.

Après le déploiement, vérifiez les quatre choix, le changement de manifeste, de cible, de version, d'avertissement et d'empreinte. Chaque image téléchargée doit correspondre à `SHA256SUMS.txt`. Les notes de version sont incluses localement ; elles ne dépendent pas de la création d’une GitHub Release.

## Sécurité

- Le flashage se déroule localement entre le navigateur et le port USB.
- Le site ne demande et ne reçoit aucune seed ni clé privée.
- Vérifiez l’empreinte SHA-256 avant publication ou distribution.
- Vérifiez la famille de carte et, pour le P4, la révision silicium exacte.
- Ne débranchez jamais l’appareil pendant l’écriture.
- Une nouvelle installation peut effacer les données déjà présentes sur la flash.
- Après installation et vérification du démarrage, déconnectez les données USB et utilisez une alimentation autonome avant de générer des secrets.
