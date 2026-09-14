# AURORA Seed Generator

Firmware Bitcoin hors ligne, avec un noyau commun et deux cibles matérielles : **ESP32-2432S028(R)** (320 × 240) et portage **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3** (480 × 800 portrait, version sans caméra fournie). Toute l’interface est en français ; seuls les mots de la phrase de récupération utilisent la liste anglaise officielle BIP39.

> Cette branche développe **1.9.0-dev** : confirmation de passphrase, PIN par fichier et verrouillage de session, sans changer les dérivations Bitcoin ni le chiffrement AES-256-GCM/PBKDF2 existant. Les nouveaux parcours doivent être validés sur les deux appareils. Les binaires du Web Flasher restent ceux de la version CYD **1.7.6**. [Sécurité et compatibilité V1/V2](SECURITY.md) · [Architecture et validation P4](targets/waveshare_p4/README.md).

![Fond de l’écran de démarrage AURORA](assets/splash_320x240.png)

Dernière version publiée et binaires inclus : **1.7.6**
Notes de version : [luminosité, aperçu de collecte et 320 échantillons](webflasher/CHANGELOG.md)
Installation Web : [AURORA Web Flasher](https://nitrus21.github.io/Aurora_seed_generator/)
Environnement : **PlatformIO + Arduino**
Cible : **ESP32-2432S028R / Cheap Yellow Display**
Réseaux : **Wi-Fi et Bluetooth désactivés**

> [!CAUTION]
> AURORA est un projet expérimental à auditer avant tout usage avec de vrais fonds. Un ESP32 généraliste n’est pas un élément sécurisé et n’offre pas la résistance physique d’un portefeuille matériel certifié. Commencez avec des montants de test, comparez toujours les adresses avec un logiciel reconnu et ne photographiez jamais une seed ou une clé privée.

## Sommaire

- [À quoi sert AURORA ?](#à-quoi-sert-aurora-)
- [Ce qu’AURORA ne fait pas](#ce-quaurora-ne-fait-pas)
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

AURORA transforme un ESP32-2432S028R en générateur et lecteur de portefeuille Bitcoin hors ligne. Il permet de :

- créer une phrase BIP39 anglaise de 12, 15, 18, 21 ou 24 mots ;
- ajouter une passphrase BIP39 optionnelle ;
- mélanger le générateur matériel de l’ESP32 avec des mouvements tactiles et la luminosité ;
- dériver une première adresse Bitcoin Mainnet selon BIP44, BIP49, BIP84 ou BIP86 ;
- vérifier que la phrase a bien été recopiée ;
- afficher l’adresse, la clé publique étendue du compte et leurs QR codes ;
- révéler volontairement la clé privée WIF, sur un écran rouge d’avertissement ;
- restaurer une seed existante avec autocomplétion BIP39 ;
- ouvrir une sauvegarde `.aurora` chiffrée depuis une carte microSD ;
- exporter un portefeuille vers un fichier Aurora Wallet chiffré ou un fichier Electrum privé ;
- effacer les tampons sensibles de la session avant de revenir à l’accueil.

## Ce qu’AURORA ne fait pas

AURORA :

- ne se connecte pas à Internet ;
- ne consulte pas le solde d’une adresse ;
- ne construit et ne signe pas de transaction ;
- ne diffuse aucune transaction ;
- ne remplace pas une sauvegarde physique durable de la seed ;
- ne protège pas contre une personne ayant un accès physique prolongé à l’appareil ;
- n’active pas automatiquement Secure Boot ni le chiffrement du flash ;
- ne garantit pas qu’un portefeuille tiers interprétera une WIF avec le même type de script.

La méthode recommandée consiste à utiliser AURORA hors ligne pour créer ou examiner les secrets, puis à utiliser uniquement une clé publique étendue ou un descripteur watch-only sur l’ordinateur connecté.

## Fonctionnalités

- Liste anglaise officielle BIP39 de 2 048 mots et contrôle du checksum.
- Toutes les combinaisons **12/15/18/21/24 mots × Legacy/Nested SegWit/Native SegWit/Taproot**.
- Passphrase BIP39 ASCII optionnelle, de 0 à 63 caractères imprimables, saisie deux fois après la collecte.
- PIN de 4 à 8 chiffres par fichier Aurora V2 ; trois erreurs ferment la session, sans effacer la carte.
- Entropie tactile : coordonnées, pression et timings de 320 échantillons.
- Luminosité : lecture de la photorésistance intégrée sur GPIO34 à chaque échantillon.
- RNG matériel ESP32 activé explicitement autour de `esp_random()`.
- Mélange final RNG + tactile + luminosité par SHA-256 avant création BIP39.
- Aperçu hexadécimal défilant et jauge de collecte rouge, orange puis verte.
- Dérivations Bitcoin Mainnet BIP44, BIP49, BIP84 et BIP86.
- Affichage de huit mots maximum par page.
- Vérification de trois positions différentes tirées aléatoirement.
- Suggestions BIP39 pendant la restauration et la vérification de sauvegarde.
- Clé publique étendue de compte : `xpub`, `ypub` ou `zpub` selon le type.
- QR de l’adresse, de la clé publique étendue et de la WIF privée brute.
- Export microSD FAT32.
- Conteneur Aurora Wallet authentifié par AES-256-GCM.
- Autotest cryptographique bloquant au démarrage.
- Effacement anti-optimisation des principaux buffers applicatifs et graphiques.
- Aucune écriture volontaire de seed dans NVS, SPIFFS ou LittleFS.

La cryptographie Bitcoin repose principalement sur [uBitcoin](https://github.com/micro-bitcoin/uBitcoin), épinglé au commit `877542fdc16319dd92a7d2a679ea9dacce474bd2`, sur trezor-crypto inclus par cette bibliothèque et sur mbedTLS fourni par l’environnement ESP32. Le script `tools/patch_ubitcoin.py` applique les corrections et renforcements RAM attendus ; la compilation s’arrête si la dépendance ne correspond plus aux motifs contrôlés.

## Parcours de l’application

![Schéma des parcours AURORA](assets/aurora_workflow.svg)

Ce schéma décrit la version publiée 1.7.6. En développement : configuration → collecte → double passphrase → portefeuille ; la sauvegarde ajoute un PIN et l'ouverture n'affiche plus les secrets automatiquement. Voir le [parcours sécurisé actuel](SECURITY.md).

L’accueil présente trois choix :

1. **NOUVEAU PORTEFEUILLE** : création complète avec RNG matériel, entropie tactile et luminosité.
2. **OUVRIR AURORA WALLET** : lecture d’un fichier `.aurora` chiffré présent à la racine de la microSD.
3. **RESTAURER UNE SEED** : saisie manuelle d’une phrase existante, avec autocomplétion.

Le logo blanc utilisé sur cette page est également conservé dans le projet :

![Logo Bitcoin blanc de l’accueil](assets/bitcoin_logo_112x160.png)

## Matériel nécessaire

- une carte **ESP32-2432S028R** avec écran ILI9341 2,8 pouces et dalle XPT2046 ;
- un câble USB capable de transférer les données, pas uniquement de charger ;
- un ordinateur Windows, macOS ou Linux ;
- Visual Studio Code + PlatformIO, ou PlatformIO Core en ligne de commande ;
- facultatif : une carte microSD formatée en FAT32 pour les sauvegardes ;
- idéalement : un ordinateur hors ligne ou une machine dédiée pour la génération finale.

Selon la révision de la carte, Windows peut demander le pilote du convertisseur USB-série, généralement CH340 ou CP210x. Vérifiez le composant présent sur votre propre carte avant d’installer un pilote.

## Installation depuis le Web Flasher

Ouvrez [AURORA Web Flasher](https://nitrus21.github.io/Aurora_seed_generator/) dans Chrome ou Microsoft Edge sur ordinateur, branchez l’ESP32-2432S028R avec un câble USB de données, puis choisissez **Installer AURORA v1.7.6** et le port correspondant à votre appareil.

L’installeur utilise l’image complète `aurora-1.7.6-esp32-2432s028r.factory.bin` avec son bootloader et ses partitions. Une nouvelle installation peut effacer les données présentes en flash. Attendez la confirmation de fin, vérifiez **1.7.6** au démarrage et l’autotest **E00**, puis débranchez les données USB avant toute génération de secrets.

Les [instructions du Web Flasher](webflasher/README.md) et les [empreintes des binaires](webflasher/firmware/SHA256SUMS.txt) sont conservées dans le dépôt. La compilation locale reste possible avec les étapes ci-dessous.

## Installation rapide avec Visual Studio Code

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

Placez-vous dans le dossier contenant `platformio.ini`.

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

### Empreinte de la version finale 1.7.6 compilée

Fichier distribué : `webflasher/firmware/firmware.bin` (copie du build PlatformIO)
Taille : **1 483 328 octets**
SHA-256 :

```text
9CFF7030555D60AD3A5E18589D76B961BBBE8431C32C34B8374B5E1A31C33033
```

Cette empreinte concerne l’application seule, pas l’image fusionnée du Web Flasher. Les empreintes de tous les binaires sont dans [SHA256SUMS.txt](webflasher/firmware/SHA256SUMS.txt). Sous PowerShell, `./tests/release/verify.ps1` contrôle les versions, le manifeste, le contenu de l’image fusionnée et les empreintes, sans flasher l’appareil.

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
2. vérifiez que vous utilisez bien le binaire de la version 1.7.6 correspondant à cette empreinte ;
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

Si le code n’est pas `E00`, ne créez pas de portefeuille et notez le code exact.

### Validation de la version 1.7.6

Le 14 septembre 2026, les binaires 1.7.6 inclus dans ce dépôt ont été programmés sur la carte ESP32-2432S028R du projet, avec vérification des données écrites. Au redémarrage, l’autotest a renvoyé **E00 en 2 120 ms**, sans défaut de démarrage observé.

Les tests natifs, le contrôle des binaires et les tests de l’écran LVGL avec matériel simulé ont réussi. Les gestes réels, la réponse de la photorésistance et le parcours complet d’un portefeuille de test restent à vérifier manuellement. E00 ne certifie pas l’entropie physique et ne constitue pas un audit de sécurité.

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
- la pression tactile ;
- le compteur en microsecondes ;
- une valeur provenant du RNG matériel ESP32 ;
- la lecture lumineuse brute sur 12 bits de la photorésistance intégrée (GPIO34).

Après 320 échantillons, le mélange est condensé par SHA-256. La durée de collecte et un tirage matériel supplémentaire restent incorporés à la fin. Lors de la création BIP39, 32 nouveaux octets du RNG matériel sont mélangés avec ce résultat, puis condensés une seconde fois, comme auparavant.

La barre suit le nombre d’échantillons : **rouge de 0 à 49 %**, **orange de 50 à 99 %**, puis **verte à 100 %**. Le vert reste visible une seconde avant l'arrêt des capteurs et la double saisie de passphrase. La jauge n'est pas une mesure certifiée de bits d'entropie. Sur P4, micro et caméra facultative complètent la collecte ; la pression et la luminosité sont propres au CYD.

Le bandeau « Aperçu du mélange » fait défiler quatre groupes hexadécimaux, actualisés pendant les gestes (au plus environ dix fois par seconde, avec une dernière actualisation à 100 %). Chaque groupe est un HMAC-SHA-256 tronqué de l’échantillon, avec une clé d’affichage aléatoire indépendante et temporaire. Ni les valeurs brutes du RNG, ni l’état du mélange secret, ni l’entropie finale BIP39 ne sont affichés. Cette clé d’affichage est effacée en fin de collecte ou en cas de retour ; le bandeau est effacé en quittant l’écran.

#### 3/7 — Passphrase BIP39

Saisissez la même passphrase dans les deux champs de la même page, ou laissez les deux vides. La comparaison est exacte ; la confirmation n'est jamais préremplie. Limite inchangée : ASCII imprimable, 63 caractères.

Une passphrase différente crée un autre portefeuille ; l'oublier rend les fonds associés irrécupérables. La saisir après l'entropie ne change pas la dérivation BIP39. Elle est distincte du mot de passe de chiffrement `.aurora` et du PIN d'accès aux secrets.

#### 4/7 — Phrase de récupération

- Recopiez les mots dans l’ordre exact.
- AURORA affiche au maximum huit mots par page.
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

### Ouvrir un Aurora Wallet

1. Formatez une microSD en FAT32.
2. Copiez les fichiers `.aurora` à la racine de la carte.
3. Insérez la carte avant de choisir **OUVRIR AURORA WALLET**.
4. Sélectionnez un fichier dans la liste déroulante.
5. Utilisez **ACTUALISER** si la carte a été insérée après l’ouverture de la page.
6. Saisissez le mot de passe du fichier.
7. Patientez pendant PBKDF2 et AES-GCM, environ 15 secondes sur la carte testée.

Après déchiffrement, AURORA ne fait pas confiance aux valeurs enregistrées. Il valide la phrase BIP39, recalcule le portefeuille depuis les mots et la passphrase, puis compare l’adresse, le chemin, les clés étendues, la WIF et le descripteur. Une différence, un mauvais mot de passe ou un fichier modifié provoque un refus.

L'ouverture affiche seulement les données publiques. Le PIN est requis pour révéler les mots, la passphrase ou le QR privé et pour exporter des secrets. Trois erreurs cumulées ferment la session ; les fichiers restent sur la carte. Les anciens V1 demandent un PIN temporaire, à rendre permanent en réexportant sous un nouveau nom. [Règles et limites](SECURITY.md).

### Restaurer une seed

1. Choisissez le nombre de mots.
2. Saisissez chaque mot anglais séparément.
3. Touchez une des trois suggestions pour éviter les fautes.
4. AURORA refuse la phrase si le checksum BIP39 est invalide.
5. Saisissez et confirmez la passphrase BIP39 éventuelle, puis créez un PIN de session.
6. Sélectionnez le type de dérivation dans la liste.
7. Comparez l’adresse et utilisez les QR.
8. Utilisez **EXPORTER** pour sauvegarder le portefeuille restauré.

La restauration manuelle ne sauvegarde rien automatiquement.

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

Le conteneur binaire `.aurora` est écrit en **V2** ; la lecture **V1** reste disponible. Les deux versions utilisent sans changement :

- AES-256-GCM ;
- une clé AES de 256 bits ;
- un tag d’authentification de 128 bits ;
- PBKDF2-HMAC-SHA-256 avec 120 000 itérations ;
- un sel aléatoire de 128 bits ;
- un nonce aléatoire de 96 bits ;
- les paramètres d’en-tête comme données authentifiées ;
- un mot de passe ASCII de 12 à 63 caractères, saisi deux fois à la création.

Le contenu chiffré comprend les mots BIP39, la passphrase éventuelle, le type d’adresse, le chemin, l’adresse, la clé publique étendue, la clé privée étendue, la WIF, le descripteur et la version du firmware.

V2 ajoute un vérificateur PIN chiffré et passe de 1 120 à 1 200 octets. Le PIN ne modifie aucune clé et ne remplace pas le mot de passe. Un ancien firmware ne lit pas V2 ; les deux cartes doivent utiliser le nouveau firmware. [Format exact et migration](SECURITY.md#format-binaire-v2-et-migration).

AES-256 ne rend pas un mot de passe faible équivalent à une clé aléatoire de 256 bits. Utilisez une phrase de passe longue, unique et conservée séparément. Il n’existe ni porte dérobée ni récupération en cas de perte.

Après lecture ou écriture, AURORA écrase le mot de passe du fichier, la clé AES, le texte clair et les principaux temporaires. Le fichier reste néanmoins une sauvegarde complète : mot de passe compromis = secrets compromis.

## Brochage et configuration matérielle

### Brochage par défaut

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
pio run
```

Le script recadre en 4:3, redimensionne en 320 × 240 et régénère `src/assets/splash_img.c` au format RGB565. Le bouton, le titre et la version restent dessinés par LVGL.

### Logo de l’accueil

Le logo blanc est `assets/bitcoin_logo_112x160.png`. Pour le régénérer après modification de la source :

```text
python tools/make_bitcoin_logo_asset.py
pio run
```

Ne modifiez pas directement les grands tableaux C générés si l’image PNG source peut être mise à jour proprement.

## Sécurité et limites

### Réseaux

Au démarrage, le firmware coupe le Wi-Fi et le Bluetooth. Il n’efface pas les identifiants éventuellement présents en NVS afin d’éviter une écriture flash supplémentaire, mais il ne les utilise pas.

### RNG

Sur l’ESP32 original, `esp_random()` n’est considéré comme une source matérielle complète que lorsqu’une source d’entropie est active. AURORA active explicitement la source interne SAR-ADC avec `bootloader_random_enable()`, collecte les valeurs, puis la désactive.

La lecture de luminosité et cette source SAR-ADC sont alternées : désactivation de la source interne, configuration et lecture ponctuelle de l’ADC, puis réactivation avant tout appel au RNG. Elles ne fonctionnent pas simultanément, conformément aux [contraintes Espressif sur le RNG et l’ADC](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/random.html). Le Wi-Fi et le Bluetooth restent désactivés.

### Mémoire

Les secrets doivent nécessairement exister en RAM pendant la dérivation et l’affichage. AURORA écrase explicitement ses buffers, les textes LVGL sensibles, les contextes cryptographiques principaux et plusieurs temporaires uBitcoin. Cela ne garantit pas l’effacement après un crash, une coupure brutale, une attaque DMA ou une analyse physique.

### Flash

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
| microSD absente | Reformatez en FAT32, réinsérez avant l’ouverture de la page et utilisez **ACTUALISER** |
| Fichier déjà existant | Choisissez un autre nom ; AURORA refuse volontairement l’écrasement |
| Mauvais mot de passe `.aurora` | Vérifiez casse, espaces et caractères ; le fichier ne possède aucune procédure de récupération |
| BlueWallet indique `Non-base58 character` | Le QR privé doit commencer par `K` ou `L` et ne contenir que la WIF brute ; utilisez une version au moins égale à 1.7.5 et comparez ensuite l’adresse |
| Échec de sécurité E01–E60 | Ne générez rien ; notez le code, recompilez avec les dépendances épinglées et contrôlez le matériel |

## Dépendances épinglées

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
  aurora_workflow.svg          Schéma utilisé dans ce README
include/
  board_config.h               Brochage, rotation et calibration
  version.h                    Version affichée sur le splash
  secure_memory.h              Effacement anti-optimisation
  hardware_rng.h               Activation de la source RNG ESP32
  entropy.h                    Mélange tactile + luminosité, SHA-256 et aperçu HMAC
  wallet.h                     Interface du moteur Bitcoin
  sd_export.h                  Types d’export et lecture Aurora Wallet
  ui.h                         État et parcours LVGL
src/
  main.cpp                     Initialisation écran, tactile et radios
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
  release/verify.ps1          Cohérence de version, image fusionnée et SHA-256
webflasher/
  index.html                  Installeur français publié sur GitHub Pages
  manifest.json               Version et image installées
  firmware/                   Binaires 1.7.6, archive 1.7.5 et SHA256SUMS.txt
  CHANGELOG.md                Notes de version et état de validation
platformio.ini                 Cible, dépendances et broches TFT
README.md                      Ce guide
```

## Licence

Code AURORA : MIT. Les bibliothèques et outils tiers conservent leurs licences respectives.
