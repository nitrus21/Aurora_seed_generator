# AURORA Seed Generator

AURORA est un générateur et lecteur de portefeuilles Bitcoin hors ligne. L'interface est en français ; les phrases de récupération utilisent la liste anglaise BIP39.

## Appareils et versions

| Appareil | Écran | Version du logiciel | Versions disponibles dans le Web Flasher |
| --- | --- | --- | --- |
| **ESP32-2432S028R — Cheap Yellow Display (CYD)** | 2,8 pouces, 320 × 240, tactile résistif XPT2046 | **1.9.5**, dernière version | **1.9.5** et **1.7.5** |
| **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 (P4)** | 4,3 pouces, 480 × 800 portrait, tactile capacitif GT911 | **2.0.0** | **2.0.0**, images distinctes pour silicium **1.x** et **3.x** |

Les abréviations **CYD** et **P4** désignent ces deux modèles dans la suite du guide.
Le CYD **1.9.5** reprend les tailles de police et les couleurs du clavier de la **1.7.5**
et conserve les fonctionnalités/protections décrites ci-dessous. Voir le
[détail de l’affichage CYD](targets/cyd/README.md).
La 1.9.5 est la dernière version CYD ; la 1.7.5 reste disponible.
À partir de la v2.0.0, AURORA évolue sur le P4 pour davantage de fonctionnalités.
Les [notes de version](webflasher/CHANGELOG.md) présentent uniquement les nouveautés retenues depuis la précédente version officielle.

[Utilisation du mot de passe sur Waveshare ESP32-P4](#p4-200--mot-de-passe-uniquement) · [Compilation Waveshare ESP32-P4](targets/waveshare_p4/README.md) · [Web Flasher](https://nitrus21.github.io/Aurora_seed_generator/)

![Fond de l’écran de démarrage AURORA](assets/splash_320x240.png)

Environnement : **PlatformIO + ESP-IDF** pour Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 ; **PlatformIO + Arduino** pour ESP32-2432S028R.
Utilisation : **hors ligne**.

> [!CAUTION]
> AURORA est un projet expérimental à auditer avant tout usage avec de vrais fonds. Un ESP32 généraliste n’est pas un élément sécurisé et n’offre pas la résistance physique d’un portefeuille matériel certifié. Commencez avec des montants de test, comparez toujours les adresses avec un logiciel reconnu et ne photographiez jamais une seed ou une clé privée.

## Sommaire

- [À quoi sert AURORA ?](#à-quoi-sert-aurora-)
- [Fonctionnalités](#fonctionnalités)
- [Parcours de l’application](#parcours-de-lapplication)
- [Matériel nécessaire](#matériel-nécessaire)
- [Installation depuis le Web Flasher](#installation-depuis-le-web-flasher)
- [Installation rapide avec Visual Studio Code](#installation-rapide-avec-visual-studio-code)
- [Compilation et flashage en ligne de commande](#compilation-et-flashage-en-ligne-de-commande)
- [Vérification SHA-256 du firmware](#vérification-sha-256-du-firmware)
- [Premier démarrage et autotest E00](#premier-démarrage-et-autotest-e00)
- [Utilisation détaillée](#utilisation-détaillée)
- [Types d’adresses et chemins](#types-dadresses-et-chemins)
- [Exports sur carte microSD](#exports-sur-carte-microsd)
- [Format chiffré Aurora Wallet](#format-chiffré-aurora-wallet)
- [Brochage et configuration matérielle](#brochage-et-configuration-matérielle)
- [Personnaliser les images](#personnaliser-les-images)
- [Sécurité et limites](#sécurité-et-limites)
- [Dépannage](#dépannage)
- [Arborescence du projet](#arborescence-du-projet)

## À quoi sert AURORA ?

Les deux appareils permettent de générer, restaurer et consulter un portefeuille Bitcoin hors ligne. AURORA permet de :

- créer une phrase BIP39 anglaise de 12, 15, 18, 21 ou 24 mots ;
- ajouter une passphrase BIP39 optionnelle ;
- mélanger le générateur matériel de l’ESP32 avec des mouvements tactiles et les capteurs propres à la carte ;
- dériver une première adresse Bitcoin Mainnet selon BIP44, BIP49, BIP84 ou BIP86 ;
- vérifier que la phrase a bien été recopiée ;
- afficher l’adresse, la clé publique étendue du compte et leurs QR codes ;
- révéler volontairement la clé privée WIF, sur un écran rouge d’avertissement ;
- restaurer une seed existante avec autocomplétion BIP39 ;
- sur P4, déchiffrer une seed Umbrel/LND AEZEED et produire son `xprv` maître BIP32 pour Sparrow ;
- ouvrir une sauvegarde `.aurora` chiffrée depuis une carte microSD ;
- exporter un portefeuille vers un fichier Aurora Wallet chiffré ou un fichier Electrum privé ;
- effacer les tampons sensibles de la session avant de revenir à l’accueil.

## Fonctionnalités

| Fonction | ESP32-2432S028R — 1.9.5 | Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 — 2.0.0 |
| --- | --- | --- |
| Création et restauration BIP39 | 12, 15, 18, 21 ou 24 mots | 12, 15, 18, 21 ou 24 mots |
| Adresses Bitcoin | Legacy, Nested SegWit, Native SegWit, Taproot | Legacy, Nested SegWit, Native SegWit, Taproot |
| Sources de collecte | RNG matériel, tactile résistif, temps, photorésistance | RNG matériel, tactile capacitif, temps, microphones ; caméra OV5647 facultative |
| Ouverture d'un fichier Aurora Wallet | Mot de passe du fichier | Mot de passe du fichier |
| Consultation privée après ouverture | Nouvelle saisie du mot de passe du fichier | Nouvelle saisie du mot de passe du fichier |
| Fichiers chiffrés lus | V1 et V2 | V1 et V2 |
| Fichiers chiffrés créés | V1 | V1 |
| Exports | Aurora Wallet chiffré et Electrum privé | Aurora Wallet chiffré et Electrum privé |

**Fonction supplémentaire de la Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 :**
récupération Umbrel/LND AEZEED vers une clé maître BIP32, avec affichage QR temporaire.

- Liste anglaise officielle BIP39 de 2 048 mots et contrôle du checksum.
- Toutes les combinaisons **12/15/18/21/24 mots × Legacy/Nested SegWit/Native SegWit/Taproot**.
- Passphrase BIP39 ASCII optionnelle, de 0 à 63 caractères imprimables, saisie deux fois après la collecte.
- Entropie tactile : coordonnées et timings de 320 échantillons ; pression résistive sur CYD uniquement.
- P4 : microphones et caméra facultative ; CYD : photorésistance intégrée sur GPIO34.
- RNG matériel ESP32 activé explicitement autour de `esp_random()`.
- Mélange final du RNG, du tactile et des sources disponibles par SHA-256 avant création BIP39.
- Aperçu hexadécimal défilant et jauge de collecte rouge, orange puis verte.
- Dérivations Bitcoin Mainnet BIP44, BIP49, BIP84 et BIP86.
- P4 : affichage de douze mots maximum par page.
- Vérification de trois positions différentes tirées aléatoirement.
- Suggestions BIP39 pendant la restauration et la vérification de sauvegarde.
- Récupération Umbrel/LND AEZEED sur P4 : 24 mots, checksum CRC32C, scrypt et authentification AEZ v5.
- Clé publique étendue de compte : `xpub`, `ypub` ou `zpub` selon le type.
- QR de l’adresse, de la clé publique étendue et de la WIF privée brute.
- Export microSD FAT32.
- Conteneur Aurora Wallet authentifié par AES-256-GCM.
- Autotest cryptographique bloquant au démarrage.
- Effacement anti-optimisation des principaux buffers applicatifs et graphiques.
- Aucune écriture volontaire de seed dans NVS, SPIFFS ou LittleFS.

La cryptographie Bitcoin repose principalement sur [uBitcoin](https://github.com/micro-bitcoin/uBitcoin), épinglé au commit `877542fdc16319dd92a7d2a679ea9dacce474bd2`, sur trezor-crypto inclus par cette bibliothèque et sur mbedTLS fourni par l’environnement ESP32. Le script `tools/patch_ubitcoin.py` applique les corrections et renforcements RAM attendus ; la compilation s’arrête si la dépendance ne correspond plus aux motifs contrôlés.

## Parcours de l’application

La création suit les étapes suivantes : configuration du portefeuille, collecte
d'entropie, double saisie de la passphrase éventuelle, affichage et vérification
des mots, puis informations et sauvegarde sur microSD.

L’accueil présente trois choix sur CYD et un quatrième sur P4 :

1. **NOUVEAU PORTEFEUILLE** : création complète avec RNG matériel, entropie tactile et capteurs disponibles.
2. **OUVRIR AURORA WALLET** : lecture d’un fichier `.aurora` chiffré présent à la racine de la microSD.
3. **RESTAURER UNE SEED** : saisie manuelle d’une phrase existante, avec autocomplétion.
4. **RÉCUPÉRER UMBREL / LND** (P4) : conversion hors ligne d’une seed AEZEED en clé maître BIP32 pour Sparrow.

Sur **CYD 1.9.5 et P4 2.0.0**, la création et la restauration initiales fonctionnent sans
microSD. La carte devient obligatoire à la sauvegarde/export, à l'ouverture
d'un fichier et à chaque consultation privée de ce fichier. Des contrôles sont
refaits pendant la préparation et avant l'écriture. Sans carte, l'export est
bloqué ; **FERMER** efface la session. Une carte FAT32 vide lisible suffit pour
préparer une sauvegarde, sans formatage ni création de fichier de test.
Une interruption de l'export efface les identifiants saisis.
Ne jamais retirer la carte pendant une écriture.

Sur **P4 uniquement**, le menu d'action place le logo Bitcoin en haut au centre,
puis quatre boutons centrés de **384 × 64 pixels**. La disposition à trois
boutons du menu ESP32-2432S028R donne accès à la création, à l'ouverture et à la restauration BIP39.

Le logo blanc utilisé sur cette page est également conservé dans le projet :

![Logo Bitcoin blanc de l’accueil](assets/bitcoin_logo_112x160.png)

## Matériel nécessaire

- pour le développement actif : une **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3**, avec un firmware correspondant à sa révision silicium 1.x ou 3.x ;
- pour la version ESP32-2432S028R : une **ESP32-2432S028R** avec écran ILI9341 2,8 pouces et dalle XPT2046 ;
- un câble USB capable de transférer les données, pas uniquement de charger ;
- un ordinateur Windows, macOS ou Linux ;
- Visual Studio Code + PlatformIO, ou PlatformIO Core en ligne de commande ;
- une carte microSD formatée en FAT32 pour enregistrer ou ouvrir des fichiers et pour chaque consultation privée d'un fichier ouvert ; création/restauration initiales sans carte possibles ;
- idéalement : un ordinateur hors ligne ou une machine dédiée pour la génération finale.

Selon la révision de la carte, Windows peut demander le pilote du convertisseur USB-série, généralement CH340 ou CP210x. Vérifiez le composant présent sur votre propre carte avant d’installer un pilote.

## Installation depuis le Web Flasher

Ouvrez [AURORA Web Flasher](https://nitrus21.github.io/Aurora_seed_generator/) dans Chrome ou Microsoft Edge sur ordinateur. Choisissez **CYD 1.9.5**, **CYD 1.7.5**, **P4 2.0.0 silicium 1.x** ou **P4 2.0.0 silicium 3.x**, puis branchez uniquement la carte correspondante et sélectionnez son port série.

L’installeur utilise une image complète avec son bootloader et ses partitions. Une nouvelle installation peut effacer les données présentes en flash. Les deux images P4 correspondent à des révisions de silicium différentes et ne doivent jamais être interverties. Attendez la confirmation de fin, vérifiez la version au démarrage et l’autotest **E00**, puis débranchez les données USB avant toute génération de secrets.

Les [instructions du Web Flasher](webflasher/README.md) et les [empreintes des binaires](webflasher/firmware/SHA256SUMS.txt) sont conservées dans le dépôt. La compilation locale reste possible avec les étapes ci-dessous.

## Installation rapide avec Visual Studio Code

Les étapes ci-dessous décrivent l'**ESP32-2432S028R**, dont la version finale
est 1.9.5. Pour le **P4 2.0.0**, ouvrir le même dépôt et utiliser le script
[de compilation P4](targets/waveshare_p4/README.md#compilation) dans le terminal.
Le bouton Build du projet racine compile le CYD, pas le P4.

### 1. Installer les outils

1. Installez [Visual Studio Code](https://code.visualstudio.com/).
2. Ouvrez l’onglet **Extensions**.
3. Recherchez et installez **PlatformIO IDE**.
4. Redémarrez Visual Studio Code si l’extension le demande.

### 2. Ouvrir le projet

1. Téléchargez ou copiez le dossier complet AURORA.
2. Dans Visual Studio Code, choisissez **Fichier > Ouvrir un dossier**.
3. Sélectionnez le dossier qui contient `platformio.ini`, `src/`, `include/` et `assets/`.
4. Attendez la fin de l’installation automatique de la plateforme et des bibliothèques.

Ne copiez pas uniquement `src/main.cpp` dans un projet Arduino vide : le firmware dépend de la configuration PlatformIO, des polices, des images, des scripts de durcissement et des versions épinglées.

### 3. Brancher l’ESP32

1. Branchez l’ESP32 avec un câble USB de données.
2. Dans PlatformIO, ouvrez **Devices** pour identifier le port série.
3. Sous Windows, le port ressemble à `COM3` ou `COM5`.
4. Sous Linux, il ressemble souvent à `/dev/ttyUSB0` ou `/dev/ttyACM0`.
5. Sous macOS, il ressemble souvent à `/dev/cu.usbserial-...`.

### 4. Compiler

Cliquez sur l’icône **✓ Build** de PlatformIO. La fin du journal doit contenir :

```text
========================= [SUCCESS] =========================
```

L’avertissement TFT_eSPI indiquant que `TOUCH_CS` n’est pas défini est normal : AURORA utilise directement la bibliothèque XPT2046 avec les broches de `include/board_config.h`.

### 5. Flasher

Cliquez sur **→ Upload**. PlatformIO compile si nécessaire, détecte le port et programme les quatre zones nécessaires : bootloader, partitions, boot application et firmware.

Un flashage réussi se termine par :

```text
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
========================= [SUCCESS] =========================
```

### 6. Vérifier le démarrage

Ouvrez **Serial Monitor** à `115200` bauds. Le résultat attendu est :

```text
AURORA autotest : E00
AURORA autotest durée : environ 2 secondes
```

Fermez ensuite le moniteur série avant toute nouvelle commande de flashage, car un port déjà ouvert peut empêcher PlatformIO d’utiliser la carte.

## Compilation et flashage en ligne de commande

Depuis la racine du dépôt, compiler le **P4 actif** sous PowerShell avec le
profil correspondant à la carte :

```powershell
# P4 révision 3.x
.\targets\waveshare_p4\build.ps1 -Target waveshare-p4
# Ou P4 révision 1.x, dont 1.3
.\targets\waveshare_p4\build.ps1 -Target waveshare-p4-rev1
```

Ces commandes ne flashent rien. Les profils doivent être compilés séparément ;
voir les [prérequis et contrôles P4](targets/waveshare_p4/README.md#compilation).
Les commandes ci-dessous concernent l'**ESP32-2432S028R**.
La compilation produit 1.9.5 : sa table de partitions doit être installée
avec le programme. Un chargement du seul `firmware.bin` sur une ancienne table
est refusé au démarrage. Aucun fichier microSD n’est effacé par ce contrôle.

### Vérifier PlatformIO

```text
pio --version
pio device list
```

### Compiler

```text
pio run -e esp32-2432S028R
```

Le binaire principal est créé ici :

```text
.pio/build/esp32-2432S028R/firmware.bin
```

### Flasher en indiquant le port

Windows :

```text
pio run -e esp32-2432S028R --target upload --upload-port COM3
```

Linux :

```text
pio run -e esp32-2432S028R --target upload --upload-port /dev/ttyUSB0
```

macOS :

```text
pio run -e esp32-2432S028R --target upload --upload-port /dev/cu.usbserial-XXXX
```

### Lire l’autotest

```text
pio device monitor --port COM3 --baud 115200
```

Quittez le moniteur avec `Ctrl+C`.

### Installation depuis un seul `firmware.bin`

La méthode recommandée reste **PlatformIO Upload**, surtout sur une carte neuve. Le fichier `firmware.bin` seul correspond uniquement à la partition applicative située à l’adresse `0x10000`. Une carte totalement vierge a également besoin du bootloader, de la table de partitions et de `boot_app0`.

Sur une carte déjà initialisée avec exactement le même environnement AURORA, un utilisateur avancé peut mettre à jour uniquement l’application à `0x10000`. Ne changez jamais cette adresse sans vérifier `platformio.ini` et la table de partitions. PlatformIO évite ces erreurs et effectue automatiquement les vérifications de hash pendant l’écriture.

## Vérification SHA-256 du firmware

SHA-256 permet de vérifier que le fichier n’a pas changé entre sa création, son téléchargement et son flashage. Il ne prouve l’authenticité que si la valeur de référence a été obtenue par un canal de confiance.

### Empreinte de l’application CYD 1.9.5

Fichier distribué : `webflasher/firmware/firmware.bin` (copie du build PlatformIO)
Taille : **1 489 104 octets**
SHA-256 :

```text
4C3A30209EA8240F697A959695D609BBE0CB9114B5425D3DC5FFD697E9820E2E
```

Cette empreinte concerne l’application seule, pas l’image fusionnée du Web Flasher. Les empreintes de tous les binaires sont dans [SHA256SUMS.txt](webflasher/firmware/SHA256SUMS.txt). Sous PowerShell, `./tests/release/verify.ps1 -ReleasedArtifactsOnly` contrôle les versions, le manifeste, le contenu de l’image fusionnée et les empreintes, sans flasher l’appareil.

### Windows PowerShell

```powershell
Get-FileHash -Algorithm SHA256 .\webflasher\firmware\firmware.bin
```

Pour obtenir uniquement la valeur :

```powershell
(Get-FileHash -Algorithm SHA256 .\webflasher\firmware\firmware.bin).Hash
```

### Linux

```bash
sha256sum webflasher/firmware/firmware.bin
```

### macOS

```bash
shasum -a 256 webflasher/firmware/firmware.bin
```

Ces commandes contrôlent le binaire distribué. Pour contrôler une compilation locale, utilisez le chemin `.pio/build/esp32-2432S028R/firmware.bin` ; un autre environnement de compilation ou horodatage peut produire une empreinte différente.

La casse des lettres n’a pas d’importance, mais les 64 caractères hexadécimaux doivent être identiques pour le même binaire distribué. Si l’empreinte diffère :

1. ne flashez pas le fichier ;
2. vérifiez que vous utilisez bien le binaire de la cible et de la version correspondant à cette empreinte ;
3. retéléchargez ou recompilez depuis les sources attendues ;
4. contrôlez `platformio.ini` et la liste des dépendances ;
5. générez et archivez une nouvelle empreinte si vous avez volontairement modifié le code.

Une recompilation après modification du code, des images, des options ou des dépendances produit normalement un autre SHA-256. Archivez ensemble le binaire, son SHA-256, la version du code et la sortie de :

```text
pio pkg list -e esp32-2432S028R
```

## Premier démarrage et autotest E00

Au démarrage, AURORA affiche le splash et lance en arrière-plan un autotest bloquant. Le bouton **CONTINUER** devient utilisable après la fin du contrôle.

`E00` signifie que tous les contrôles intégrés ont réussi. Toute autre valeur affiche **ÉCHEC DE SÉCURITÉ** et empêche la génération.

| Code | Contrôle concerné |
|---:|---|
| E00 | Tous les tests ont réussi |
| E01 | Clé maître BIP32 |
| E02 | Seed BIP39 connue |
| E03 | Checksum de descripteur BIP380 |
| E04 | Recherche et autocomplétion BIP39 |
| E05 | Encodage/décodage WIF et correspondance d’adresse |
| E10 à E14 | Construction Legacy/BIP44 |
| E20 à E24 | Construction Nested SegWit/BIP49 |
| E30 à E34 | Construction Native SegWit/BIP84 |
| E40 à E44 | Construction Taproot/BIP86 |
| E51 à E55 | Génération des phrases de 12 à 24 mots |
| E60 | PBKDF2-HMAC-SHA-256 et AES-256-GCM Aurora Wallet |
| E61 | Dérivation de la racine BIP32 depuis l'entropie AEZEED |

Si le code n’est pas `E00`, ne créez pas de portefeuille et notez le code exact.

L'autotest contrôle les résultats cryptographiques intégrés. Il ne mesure pas
l'entropie physique et ne constitue pas une certification de sécurité.

## Utilisation détaillée

### Nouveau portefeuille : pages 1 à 7

#### 1/7 — Configuration du portefeuille

- Choisissez 12, 15, 18, 21 ou 24 mots.
- Choisissez Legacy, Nested SegWit, Native SegWit ou Taproot.
- Le réseau est toujours **Bitcoin Mainnet**.

Le nombre de mots détermine la quantité d’entropie BIP39, pas le format de l’adresse. Toutes les combinaisons proposées sont valides.

#### 2/7 — Collecte d’entropie

Tracez des mouvements irréguliers dans le cadre jusqu’à 100 %. Pour chaque échantillon, AURORA conserve les sources précédentes et mélange :

- les coordonnées X/Y ;
- la pression tactile (CYD uniquement) ;
- le compteur en microsecondes ;
- une valeur provenant du RNG matériel ESP32 ;
- la lecture lumineuse brute sur 12 bits de la photorésistance intégrée GPIO34 (CYD uniquement) ;
- les blocs microphone et caméra facultative disponibles (P4 uniquement, avec leur propre séquence).

Après 320 échantillons, le mélange est condensé par SHA-256. La durée de collecte et un tirage matériel supplémentaire restent incorporés à la fin. Lors de la création BIP39, 32 nouveaux octets du RNG matériel sont mélangés avec ce résultat, puis condensés une seconde fois, comme auparavant.

La barre suit le nombre d’échantillons : **rouge de 0 à 49 %**, **orange de 50 à 99 %**, puis **verte à 100 %**. Le vert reste visible une seconde avant l'arrêt des capteurs et la double saisie de passphrase. La jauge n'est pas une mesure certifiée de bits d'entropie. Sur P4, micro et caméra facultative complètent la collecte ; la pression et la luminosité sont propres au CYD.

Le bandeau « Aperçu du mélange » fait défiler quatre groupes hexadécimaux, actualisés pendant les gestes (au plus environ dix fois par seconde, avec une dernière actualisation à 100 %). Chaque groupe est un HMAC-SHA-256 tronqué de l’échantillon, avec une clé d’affichage aléatoire indépendante et temporaire. Ni les valeurs brutes du RNG, ni l’état du mélange secret, ni l’entropie finale BIP39 ne sont affichés. Cette clé d’affichage est effacée en fin de collecte ou en cas de retour ; le bandeau est effacé en quittant l’écran.

#### 3/7 — Passphrase BIP39

Saisissez la même passphrase dans les deux champs de la même page, ou laissez les deux vides. La comparaison est exacte ; la confirmation n'est jamais préremplie. Limite inchangée : ASCII imprimable, 63 caractères.

Une passphrase différente crée un autre portefeuille ; l'oublier rend les fonds associés irrécupérables. Elle est distincte du mot de passe de chiffrement `.aurora`.

#### 4/7 — Phrase de récupération

- Recopiez les mots dans l’ordre exact.
- Le P4 affiche au maximum douze mots par page.
- Utilisez **SUIVANT** et **PRÉCÉDENT** pour parcourir les pages.
- Ne photographiez jamais l’écran.
- Ne stockez jamais la phrase dans un service cloud ou une messagerie.

#### 5/7 — Vérification de sauvegarde

AURORA choisit trois positions différentes avec le RNG matériel. Tapez les premières lettres ; les suggestions BIP39 apparaissent au-dessus du clavier. Toucher une proposition recopie exactement le mot et passe au champ suivant. Un préfixe n’est complété automatiquement que s’il ne correspond plus qu’à un seul mot BIP39.

Les positions sont retirées aléatoirement à chaque nouveau passage sur l’écran.

#### 6/7 — Informations et QR codes

L’écran affiche :

- la première adresse de réception ;
- le chemin complet `m/purpose'/0'/0'/0/0` ;
- la clé publique étendue du compte ;
- les QR de l’adresse et de la clé publique étendue.

La clé privée n’apparaît qu’après une action volontaire sur **CLÉ PRIVÉE**. Le QR privé contient une WIF Base58Check brute commençant normalement par `K` ou `L`. Il ne doit contenir ni `wpkh(`, ni parenthèses, ni `#checksum`.

> [!IMPORTANT]
> Une WIF ne contient pas le type d’adresse. Après un import dans BlueWallet ou un autre logiciel, comparez l’adresse obtenue avec celle affichée par AURORA avant de recevoir des fonds. Pour conserver sans ambiguïté le chemin et le type de script, préférez une restauration BIP39 complète ou une importation de clé étendue/descripteur adaptée.

#### 7/7 — Sauvegarde et effacement

Vous pouvez exporter sur microSD, puis utiliser **EFFACER**. Cette action écrase les principaux buffers de la session et revient à l’accueil. Couper brutalement l’alimentation ne remplace pas l’action **EFFACER**.

### P4 2.0.0 : mot de passe uniquement

Le même parcours s’applique à l’**ESP32-2432S028R 1.9.5**.

Sur la **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3**, l'accès aux fichiers et aux
consultations privées utilise le mot de passe du fichier. Après ouverture,
la session conserve uniquement les informations publiques, le nom du fichier
et son empreinte SHA-256.
Cette empreinte n’est pas une clé et ne permet pas de déchiffrer le fichier.

Pour consulter les mots, la passphrase ou la clé privée, ou exporter les secrets
d’un fichier ouvert, il faut ressaisir son mot de passe. La microSD est relue,
l’empreinte et l’authentification AES-GCM sont vérifiées, puis le portefeuille est
recalculé et comparé. Le mot de passe et la clé de fichier sont effacés après
utilisation ; les données privées de consultation sont effacées au retour,
à l’annulation, à l’erreur ou à l’expiration. Changer de catégorie privée demande
une nouvelle authentification. Une préparation d’export autorisée conserve les
données nécessaires jusqu’à sa fin, avec une limite de 120 secondes.

Les fichiers créés utilisent le **format V1** : AES-256-GCM et
PBKDF2-HMAC-SHA-256 à 120 000 itérations. Les formats V1 et V2 sont lisibles
avec leur mot de passe sur les deux appareils.

Avant une première sauvegarde, une création/restauration doit encore conserver
temporairement ses secrets pour terminer la vérification et l’export. Ce n’est
pas une session de fichier rouvert : sans sauvegarde, les effacer impose de
recommencer. Le verrouillage et 120 secondes d’inactivité effacent ce parcours.
Le résultat AEZEED reste temporaire jusqu’à consultation ; quitter son QR ou
atteindre 15 secondes d’affichage ferme et efface la session.

Il n’y a aucune conservation volontaire de mot de passe ou de clé privée dans
la flash interne. Cela ne signifie pas absence de secrets pendant les calculs
en RAM, ni garantie d’effacement physique lors d’une coupure. L’export Electrum
en clair reste une exception explicite choisie par l’utilisateur.

### Ouvrir un Aurora Wallet

1. Formatez une microSD en FAT32.
2. Copiez les fichiers `.aurora` à la racine de la carte.
3. Insérez la carte avant de choisir **OUVRIR AURORA WALLET**.
4. Sélectionnez un fichier dans la liste déroulante.
5. Utilisez **ACTUALISER** si la carte a été insérée après l’ouverture de la page.
6. Saisissez le mot de passe du fichier.
7. Patientez pendant PBKDF2, AES-GCM et la vérification du portefeuille ; la durée dépend de la carte et du firmware.

Après déchiffrement, AURORA ne fait pas confiance aux valeurs enregistrées. Il valide la phrase BIP39, recalcule le portefeuille depuis les mots et la passphrase, puis compare l’adresse, le chemin, les clés étendues, la WIF et le descripteur. Une différence, un mauvais mot de passe ou un fichier modifié provoque un refus.

L’ouverture affiche seulement les données publiques. Le mot de passe du fichier est redemandé pour révéler les mots, la passphrase, le QR privé ou exporter des secrets. Les fichiers restent sur la carte. [Règles et limites](SECURITY.md).

### Restaurer une seed

1. Choisissez le nombre de mots.
2. Saisissez chaque mot anglais séparément.
3. Touchez une des trois suggestions pour éviter les fautes.
4. AURORA refuse la phrase si le checksum BIP39 est invalide.
5. Saisissez et confirmez la passphrase BIP39 éventuelle. Aucun PIN n’est demandé.
6. Sélectionnez le type de dérivation dans la liste.
7. Comparez l’adresse et utilisez les QR.
8. Utilisez **EXPORTER** pour sauvegarder le portefeuille restauré.

La restauration manuelle ne sauvegarde rien automatiquement.

### Récupérer les fonds on-chain d’Umbrel/LND dans Sparrow — P4

Le choix **RÉCUPÉRER UMBREL / LND** accepte les 24 mots AEZEED de LND. Ces mots
utilisent le vocabulaire anglais BIP39, mais leur encodage n'est pas une phrase
BIP39. AURORA vérifie la version et le checksum CRC32C, applique les paramètres
scrypt officiels de LND (`N=32768`, `r=8`, `p=1`), authentifie AEZ v5, puis
dérive le `xprv` maître BIP32 directement depuis les 16 octets d'entropie LND.
Une passphrase AEZEED vide emploie la valeur par défaut définie par LND.

Le P4 utilise un compromis temps/mémoire exact : un état ROMix sur deux est
conservé et les états intermédiaires sont recalculés. Le résultat reste
strictement identique au scrypt LND ; aucun paramètre cryptographique n'est
réduit. Environ 16 Mio de PSRAM temporaire sont nécessaires et sont écrasés
avant libération.

Sur Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 2.0.0, le `xprv` apparaît sur
l’écran privé ; quitter cet écran ou atteindre 15 secondes ferme et efface la session. Dans Sparrow, créez un portefeuille, ouvrez
**Software Wallet**, puis importez-le comme **Master Private Key (BIP32)**.
Recherchez les comptes usuels `m/49'/0'/0'`, `m/84'/0'/0'` et `m/86'/0'/0'`.

> [!WARNING]
> Cette procédure ne récupère que les fonds Bitcoin **on-chain** dérivés de la
> seed. Elle ne restaure ni l'état ni les fonds des canaux Lightning. Conservez
> et restaurez séparément le `channel.backup` / Static Channel Backup avec LND.
> Le `xprv` donne le pouvoir de dépenser tous les fonds dérivés : ne le scannez
> que sur une machine de confiance et déplacez ensuite les fonds vers une
> nouvelle seed.

## Types d’adresses et chemins

| Choix AURORA | Standard | Première adresse | Clé étendue affichée | Adresse typique |
|---|---|---|---|---|
| Legacy | BIP44 / P2PKH | `m/44'/0'/0'/0/0` | `xpub` du compte `m/44'/0'/0'` | commence par `1` |
| Nested SegWit | BIP49 / P2SH-P2WPKH | `m/49'/0'/0'/0/0` | `ypub` du compte `m/49'/0'/0'` | commence par `3` |
| Native SegWit | BIP84 / P2WPKH | `m/84'/0'/0'/0/0` | `zpub` du compte `m/84'/0'/0'` | commence par `bc1q` |
| Taproot | BIP86 / P2TR | `m/86'/0'/0'/0/0` | `xpub` du compte `m/86'/0'/0'` | commence par `bc1p` |

Le descripteur watch-only utilise la clé publique étendue BIP32 standard et l’origine de clé complète, même lorsque l’interface présente un `ypub` ou `zpub` SLIP-132.

## Exports sur carte microSD

La microSD doit être en FAT32. Les noms acceptent les lettres, chiffres, `_` et `-`, avec 24 caractères maximum. AURORA ajoute l’extension et refuse d’écraser un fichier existant.

| Choix | Nom obtenu | Contenu | Risque principal |
|---|---|---|---|
| Aurora Wallet chiffré | `mon_nom.aurora` | seed, passphrase éventuelle, adresse, chemin, xpub, xprv, WIF et descripteur | le mot de passe permet de tout déchiffrer |
| Electrum privé non chiffré | `mon_nom-electrum.json` | `xpub` et `xprv` du compte dans une structure Electrum | toute personne possédant le fichier peut dépenser les fonds |

L’export Electrum est désactivé pour Taproot/BIP86. Les exports Sparrow natif, Specter et Bitcoin Core ne sont pas proposés.

Le JSON Electrum contient notamment :

```json
{
  "keystore": {
    "xpub": "clé publique étendue du compte",
    "xprv": "clé privée étendue du compte",
    "type": "bip32",
    "pw_hash_version": 1
  },
  "wallet_type": "standard",
  "use_encryption": false,
  "seed_type": "bip39"
}
```

Ce fichier est volontairement non chiffré. Ne l’utilisez pas pour une démonstration publique et ne le laissez pas sur la carte après import.

## Format chiffré Aurora Wallet

Le **CYD 1.9.5** et le **P4 2.0.0** écrivent V1 et lisent V1/V2.
Les deux formats utilisent
les paramètres suivants :

- AES-256-GCM ;
- une clé AES de 256 bits ;
- un tag d’authentification de 128 bits ;
- PBKDF2-HMAC-SHA-256 avec 120 000 itérations ;
- un sel aléatoire de 128 bits ;
- un nonce aléatoire de 96 bits ;
- les paramètres d’en-tête comme données authentifiées ;
- un mot de passe ASCII de 12 à 63 caractères, saisi deux fois à la création.

Le contenu chiffré comprend les mots BIP39, la passphrase éventuelle, le type d’adresse, le chemin, l’adresse, la clé publique étendue, la clé privée étendue, la WIF, le descripteur et la version du firmware.

V1 fait 1 120 octets ; V2 fait 1 200 octets et contient en plus le
vérificateur PIN chiffré historique, non utilisé pour les consultations actuelles.
La Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 authentifie les consultations
avec le mot de passe du fichier. Les fichiers existants sont préservés.

AES-256 ne rend pas un mot de passe faible équivalent à une clé aléatoire de 256 bits. Utilisez une phrase de passe longue, unique et conservée séparément. Il n’existe ni porte dérobée ni récupération en cas de perte.

Après lecture ou écriture, AURORA écrase le mot de passe du fichier, la clé AES, le texte clair et les principaux temporaires. Le fichier reste néanmoins une sauvegarde complète : mot de passe compromis = secrets compromis.

## Brochage et configuration matérielle

### Brochage ESP32-2432S028R

Cette table ne s'applique pas au P4. Son BSP, son écran 480 × 800 et ses
capteurs sont décrits dans le [README P4](targets/waveshare_p4/README.md).

| Fonction | GPIO |
|---|---:|
| TFT MOSI | 13 |
| TFT MISO | 12 |
| TFT SCLK | 14 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT RST | -1 |
| Rétroéclairage | 21 |
| Photorésistance (LDR, ADC1) | 34 |
| Touch MOSI | 32 |
| Touch MISO | 39 |
| Touch CLK | 25 |
| Touch CS | 33 |
| Touch IRQ | 36 |
| microSD MOSI | 23 |
| microSD MISO | 19 |
| microSD CLK | 18 |
| microSD CS | 5 |

Les paramètres TFT se trouvent dans `platformio.ini`. Le tactile, la photorésistance, la microSD, la rotation et la calibration se trouvent dans `include/board_config.h`.

Configuration actuelle :

```text
Écran logique : 320 × 240
Rotation TFT : 1
Touch swap XY : 1
Touch inversion X : 0
Touch inversion Y : 1
Fréquence TFT : 40 MHz
Fréquence microSD : 10 MHz
```

Certains clones sans suffixe `R` utilisent un câblage ou une orientation différents. Modifiez uniquement les définitions concernées, recompilez, puis vérifiez toutes les zones tactiles avant de générer une seed.

## Personnaliser les images

### Splash

L’image source utilisée au démarrage est `assets/splash_320x240.png`. Pour la remplacer :

```text
python tools/make_splash_asset.py chemin/vers/nouvelle_image.png
```

Le script recadre en 4:3, redimensionne en 320 × 240 et régénère `src/assets/splash_img.c` au format RGB565. Le bouton, le titre et la version restent dessinés par LVGL.

### Logo de l’accueil

Le logo blanc est `assets/bitcoin_logo_112x160.png`. Pour le régénérer après modification de la source :

```text
python tools/make_bitcoin_logo_asset.py
```

Après génération, recompiler le profil P4 concerné avec `targets/waveshare_p4/build.ps1`.
Ne modifiez pas directement les grands tableaux C générés si l’image PNG source peut être mise à jour proprement.

## Sécurité et limites

### Réseaux

Sur CYD, le firmware coupe le Wi-Fi et le Bluetooth au démarrage. Il n'efface
pas les identifiants éventuellement présents en NVS, mais ne les utilise pas.
Sur P4, aucun pilote réseau n'est lancé et le coprocesseur radio C6 est maintenu
en reset selon le profil matériel ; voir les limites dans le [README P4](targets/waveshare_p4/README.md#vie-privée-et-arrêt).

### RNG

Sur l’ESP32 original, `esp_random()` n’est considéré comme une source matérielle complète que lorsqu’une source d’entropie est active. AURORA active explicitement la source interne SAR-ADC avec `bootloader_random_enable()`, collecte les valeurs, puis la désactive.

La lecture de luminosité et cette source SAR-ADC sont alternées : désactivation de la source interne, configuration et lecture ponctuelle de l’ADC, puis réactivation avant tout appel au RNG. Elles ne fonctionnent pas simultanément, conformément aux [contraintes Espressif sur le RNG et l’ADC](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/random.html). Le Wi-Fi et le Bluetooth restent désactivés.

### Mémoire

Les secrets doivent nécessairement exister en RAM pendant la dérivation et l’affichage. AURORA écrase explicitement ses buffers, les textes LVGL sensibles, les contextes cryptographiques principaux et plusieurs temporaires uBitcoin. Cela ne garantit pas l’effacement après un crash, une coupure brutale, une attaque DMA ou une analyse physique.

### Flash

Sur **CYD 1.9.5**, le binaire exclut les routines de sauvegarde de crash et
utilise un redémarrage silencieux sur panique. Avant l’accueil, la zone réservée
`aurora_scrub` est contrôlée, effacée si nécessaire et relue ; les allocations
libres obtenues par le nettoyage sont écrasées. Une erreur bloque l’utilisation.
Ce contrôle n’efface ni les sauvegardes microSD ni les allocations système actives.

La seed et les clés privées ne sont pas volontairement persistées dans le flash. Secure Boot et Flash Encryption ne sont pas activés automatiquement, car leur provisioning peut écrire des eFuses irréversibles. Sans ces protections, une personne ayant accès au matériel peut remplacer le firmware par une version malveillante.

### Écran et QR codes

Tout secret affiché peut être photographié ou observé. Le QR de clé privée doit être considéré comme aussi sensible que la seed. Une clé WIF seule est ambiguë sur le type de script ; vérifiez toujours l’adresse résultante.

### Carte microSD

- Le fichier `.aurora` est chiffré mais contient une sauvegarde complète.
- Le fichier Electrum contient une clé privée étendue en clair.
- Une suppression normale sur ordinateur n’efface pas nécessairement physiquement les cellules de la carte.
- Utilisez une carte dédiée et conservez-la hors ligne.

### Cérémonie recommandée avant de recevoir des fonds

1. Compiler depuis une machine de confiance.
2. Vérifier le SHA-256 du binaire.
3. Contrôler `E00` au démarrage.
4. Générer un portefeuille de test sans fonds.
5. Restaurer la même seed sur un portefeuille reconnu et hors ligne.
6. Comparer l’adresse Legacy, Nested SegWit, Native SegWit ou Taproot choisie.
7. Effacer la session AURORA.
8. Refaire la procédure avec la seed finale sur une machine isolée.
9. Envoyer d’abord un très petit montant et vérifier sa récupération.

## Dépannage

| Problème | Vérifications et solution |
|---|---|
| Aucun port série | Essayez un câble de données, un autre port USB et le pilote adapté au convertisseur USB-série |
| `Failed to connect` | Fermez le moniteur série, rebranchez la carte, recommencez l’upload ; utilisez le mode BOOT uniquement si votre révision le nécessite |
| Écran noir | Vérifiez l’alimentation, le rétroéclairage GPIO21, les broches TFT et le pilote `ILI9341_2_DRIVER` |
| Écran tourné ou tronqué | Vérifiez `AURORA_TFT_ROTATION`, `TFT_WIDTH` et `TFT_HEIGHT` |
| Toucher inversé ou décalé | Ajustez `AURORA_TOUCH_SWAP_XY`, `AURORA_TOUCH_INVERT_X/Y` et les valeurs MIN/MAX |
| Collecte bloquée avant 100 % | Bougez le doigt dans le cadre avec une pression suffisante jusqu’à 320 échantillons ; une lumière stable ne bloque pas la collecte |
| Rien après 100 % | L’état vert reste visible une seconde avant la génération ; si le blocage persiste, redémarrez et vérifiez que l’autotest affiche E00 |
| Suggestions incorrectes | Vérifiez que le mot est anglais et appartient à BIP39 ; la version doit être au moins 1.7.5 |
| microSD absente ou illisible | À la sauvegarde, insérez une carte FAT32 lisible et utilisez **RÉESSAYER** ; à l'ouverture, utilisez **ACTUALISER** dans la liste de fichiers. Sauvegardez les données existantes avant tout éventuel formatage sur ordinateur ; AURORA ne formate jamais la carte |
| Fichier déjà existant | Choisissez un autre nom ; AURORA refuse volontairement l’écrasement |
| Mauvais mot de passe `.aurora` | Vérifiez casse, espaces et caractères ; le fichier ne possède aucune procédure de récupération |
| BlueWallet indique `Non-base58 character` | Le QR privé doit commencer par `K` ou `L` et ne contenir que la WIF brute ; utilisez une version au moins égale à 1.7.5 et comparez ensuite l’adresse |
| Échec de sécurité E01–E60 | Ne générez rien ; notez le code, recompilez avec les dépendances épinglées et contrôlez le matériel |

## Dépendances épinglées

Le **P4** utilise ESP-IDF 5.5.5, LVGL 9.5.0 et le BSP Waveshare 1.0.1,
avec le même commit uBitcoin durci. Voir sa [configuration dédiée](targets/waveshare_p4/README.md#compilation).
La table suivante concerne l'**ESP32-2432S028R** :

| Composant | Version ou révision |
|---|---|
| Plateforme PlatformIO Espressif32 | `6.9.0` |
| LVGL | `8.4.0` |
| TFT_eSPI | `2.5.43` |
| XPT2046_Touchscreen | commit `f956c5d8ce3bf39169c7378416b89e7cfe70a034` |
| uBitcoin | commit `877542fdc16319dd92a7d2a679ea9dacce474bd2` |

Ne mettez pas ces dépendances à jour sans relancer les vecteurs de test, examiner les changements cryptographiques et vérifier que `tools/patch_ubitcoin.py` s’applique toujours exactement.

## Arborescence du projet

```text
assets/
  splash_320x240.png           Image du splash
  bitcoin_logo_112x160.png     Logo blanc de l’accueil
include/
  board_config.h               Brochage, rotation et calibration
  version.h                    Version affichée sur le splash
  secure_memory.h              Effacement anti-optimisation
  hardware_rng.h               Activation de la source RNG ESP32
  entropy.h                    Mélange RNG/tactile/capteurs, SHA-256 et aperçu HMAC
  wallet.h                     Interface du moteur Bitcoin
  sd_export.h                  Types d’export et lecture Aurora Wallet
  ui.h                         État et parcours LVGL
src/
  main.cpp                     Initialisation CYD : écran, tactile et radios
  wallet.cpp                   BIP39, BIP32, adresses et autotests
  sd_export.cpp                Electrum et conteneur Aurora Wallet
  ui.cpp                       Interface française
  assets/                      Images et polices converties en C
tools/
  patch_ubitcoin.py            Durcissement reproductible de uBitcoin
  make_splash_asset.py         Conversion du splash
  make_bitcoin_logo_asset.py   Conversion du logo
tests/
  native/                     Tests de collecte avec matériel simulé
  crypto/                     Codec V1/V2 et temporaires cryptographiques
  ui/                         Parcours P4/CYD avec matériel simulé, lanceur run.py
  memory/                     Effacement, cache et ordre de démarrage P4
  storage/                    Durée de vie des tampons de fichiers
  release/verify.ps1          Cohérence de version, image fusionnée et SHA-256
targets/waveshare_p4/
  build.ps1                   Compilation P4 selon révision silicium
  main/                       Initialisation P4, pilotes et nettoyage mémoire
webflasher/
  index.html                  Installeur français publié sur GitHub Pages
  manifest.json               Image CYD 1.9.5 installée par défaut
  manifests/                  Choix CYD 1.7.5 et P4 par révision
  firmware/                   Images CYD/P4, archives et SHA256SUMS.txt
  CHANGELOG.md                Versions proposées et nouveautés retenues
platformio.ini                 Cible ESP32-2432S028R, dépendances et broches TFT
README.md                      Ce guide
```

## Licence

Code AURORA : MIT. Les bibliothèques et outils tiers conservent leurs licences respectives ; voir aussi [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
