# AURORA Web Flasher

Installeur Web en français pour les deux familles AURORA : **ESP32-2432S028R / CYD** et **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3**. Le CYD propose la version finale **1.9.2** ainsi que l'ancienne **1.7.5**. Le P4 propose séparément les images finales pour les révisions silicium 1.x et 3.x.

## Utilisation

Ouvrez le [Web Flasher AURORA sur GitHub Pages](https://nitrus21.github.io/Aurora_seed_generator/) avec Chrome ou Microsoft Edge sur ordinateur. Sélectionnez d'abord la carte et la version exacte, branchez-la avec un câble USB de données, puis utilisez le bouton **Installer AURORA**. Une copie auto-hébergée doit également être servie en HTTPS.

Une ouverture directe de `index.html` depuis l’Explorateur Windows ne permet pas d’utiliser Web Serial.

## Images proposées

| Choix | État | Image |
|---|---|---|
| CYD 1.9.2 | finale, recommandée | `aurora-1.9.2-esp32-2432s028r.factory.bin` |
| CYD 1.7.5 | historique | `aurora-1.7.5-esp32-2432s028r.factory.bin` |
| P4 révision 1.x | 1.9.2 finale | `aurora-1.9.2-esp32-p4-rev1.factory.bin` |
| P4 révision 3.x | 1.9.2 finale | `aurora-1.9.2-esp32-p4-rev3.factory.bin` |

La version CYD 1.7.6 reste archivée dans `firmware/`, mais les choix demandés dans l'interface sont 1.9.2 et 1.7.5. Les images P4 1.x et 3.x ne sont pas interchangeables ; leurs en-têtes limitent aussi les révisions de silicium acceptées.

## Disposition réelle de la flash CYD

| Élément | Offset |
|---|---:|
| `bootloader.bin` | `0x1000` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xE000` |
| `firmware.bin` | `0x10000` |

ESP Web Tools utilise l’image fusionnée choisie à l’offset `0x0000`. Les images CYD sont préparées en mode DIO, à 40 MHz, pour une flash de 4 Mo.

Pour le P4, le bootloader est à `0x2000`, les partitions à `0x8000` et l'application à `0x10000` dans une image fusionnée prévue pour une flash de 32 Mo.

Les empreintes SHA-256 sont disponibles dans [firmware/SHA256SUMS.txt](firmware/SHA256SUMS.txt). Depuis la racine du dépôt, `./tests/release/verify.ps1` vérifie le manifeste, les versions, les octets aux offsets prévus et les empreintes affichées.

## Publication GitHub Pages

Le workflow [Deploy AURORA Web Flasher to GitHub Pages](../.github/workflows/deploy-webflasher-pages.yml) publie le dossier `webflasher/` après un push sur `main` qui modifie ce dossier ou le workflow. Il peut aussi être lancé manuellement depuis GitHub Actions. Un commit local seul ne déclenche aucun déploiement.

Le composant ESP Web Tools est verrouillé sur la version exacte `10.4.0` et la
page applique une politique CSP restrictive. Les GitHub Actions sont épinglées
par SHA de commit. Le chargement du composant depuis `unpkg.com` reste une
dépendance réseau de confiance ; une distribution à menace renforcée doit
l'auto-héberger et vérifier son contenu.

Après le déploiement, vérifiez les quatre choix, le changement de manifeste, de cible, de version, d'avertissement et d'empreinte. Chaque image téléchargée doit correspondre à `SHA256SUMS.txt`. Les notes de version sont incluses localement ; elles ne dépendent pas de la création d’une GitHub Release.

## Validation de l’appareil

Le 15 septembre 2026, les composants binaires finaux CYD 1.9.2 ont été flashés par port série sur l’ESP32-2432S028R du projet avec vérification des données écrites. Le redémarrage a produit **E00 en 2 145 ms**. L'application P4 1.x finale a aussi été écrite par port série sur un ESP32-P4 révision 1.3 de 32 Mo et vérifiée ; le démarrage a confirmé **AURORA 1.9.2**, le GT911, la PSRAM 32 Mo à 200 MHz et **E00 en 1 243 ms**, sans panic ni redémarrage pendant 35 secondes. Ces contrôles ne constituent pas encore un essai d’installation depuis le navigateur ; l'image P4 3.x reste à essayer sur un matériel 3.x réel.

## Sécurité

- Le flashage se déroule localement entre le navigateur et le port USB.
- Le site ne demande et ne reçoit aucune seed ni clé privée.
- Vérifiez l’empreinte SHA-256 avant publication ou distribution.
- Vérifiez la famille de carte et, pour le P4, la révision silicium exacte.
- Ne débranchez jamais l’appareil pendant l’écriture.
- Une nouvelle installation peut effacer les données déjà présentes sur la flash.
- Après installation et vérification du démarrage, déconnectez les données USB et utilisez une alimentation autonome avant de générer des secrets.
