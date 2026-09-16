# AURORA — Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3

### Plus d’espace pour vos mots. Plus de possibilités, toujours hors ligne.

**Version 2.0.3 · Écran capacitif 4,3″ · Portrait 480 × 800**

![AURORA](../../assets/splash_320x240.png)

Douze mots par page, de grands QR et une interface tactile aérée : AURORA
sur Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 met la création, la restauration
et la sauvegarde de vos portefeuilles Bitcoin à portée de main.

[Installer](https://nitrus21.github.io/Aurora_seed_generator/) ·
[Parcours guidé](../../README.md#votre-premier-portefeuille-en-6-étapes) ·
[Fonctions et nouveautés 2.0.3](../../webflasher/releases/2.0.3.md)

## L’expérience P4

- BIP39 de 12 à 24 mots, passphrase optionnelle et quatre types d’adresses Bitcoin.
- Titres à 20 px, sous-titres à 18 px, saisies agrandies et claviers sombres.
- Informations publiques et QR en accès direct ; consultations privées par mot de passe.
- Fichiers `.aurora` chiffrés, interopérables avec l’ESP32-2432S028R actuel.
- Collecte tactile et microphones, caméra OV5647 facultative.
- Récupération Umbrel/LND AEZEED vers une clé maître BIP32, sans restauration des canaux Lightning.

Carte exacte : **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3**, sans suffixe `-C`.
Deux images distinctes sont proposées : **silicium 1.x** et **silicium 3.x**.
La branche ESP32-2432S028R se termine en **1.9.7**, avec **1.7.5** conservée.
Depuis la v2.0.0, les nouvelles fonctionnalités se développent sur cette carte P4.

L'ouverture des fichiers et chaque consultation privée utilisent le mot de passe
du fichier. Les exports Aurora Wallet sont au format V1 et les fichiers V1/V2
sont lisibles. Voir le [parcours d’utilisation](../../README.md#ouvrir-un-fichier-aurora-wallet).

## De l’installation à la première sauvegarde

1. Sélectionnez votre révision silicium dans le Web Flasher et installez l’image complète.
2. Vérifiez le démarrage ; déconnectez les données USB avant de manipuler des secrets.
3. Créez ou restaurez un portefeuille, notez et vérifiez vos mots.
4. Sauvegardez sur microSD FAT32 au format `.aurora`, puis testez sa réouverture.
5. Verrouillez l’appareil après utilisation et conservez vos sauvegardes à l’abri.

L’application 2.0.3 pour silicium 1.3 a été flashée, relue et démarrée avec
confirmation du nettoyage initial et de l’interface. L’image 3.x est compilée
et contrôlée par logiciel ; **elle n’a pas été testée sur une carte 3.x**.
Ces contrôles ne valent pas recette complète du portefeuille ni certification.

## Un seul noyau, deux firmwares

Le dossier racine reste le seul dépôt de travail. Il n'y a pas de copie à synchroniser : ce projet P4 compile directement les fichiers communs du dossier `src/` et les en-têtes de `include/`.

| Élément | Code commun / différences |
| --- | --- |
| BIP39, BIP32, BIP44/49/84/86, autotests | `src/wallet.cpp`, même uBitcoin épinglé et durci |
| Fichiers chiffrés `.aurora`, exports Electrum | `src/sd_export.cpp`, lecture V1/V2 ; nouvelles écritures V1 sans PIN |
| Lecture des vérificateurs V2 | `src/pin_security.cpp`, validation de compatibilité des anciens fichiers |
| Effacement des allocations LVGL | `src/secure_lvgl_memory.c` |
| Mélange aléatoire et aperçu HMAC indépendant | `include/entropy.h` |
| Parcours et actions utilisateur | `src/ui.cpp` |
| ESP32-2432S028R | Arduino, TFT_eSPI, XPT2046, LDR GPIO34, microSD SPI |
| Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 | ESP-IDF, BSP Waveshare, LVGL 9, GT911, ES7210, OV5647 facultative, microSD SDMMC |

Les écrans P4 sont rendus avec des widgets natifs dans une surface **480 × 800 portrait** et des polices adaptées à cette résolution. L'écran d'entropie possède une disposition portrait dédiée. Les QR conservent leur forme carrée ; la caméra conserve son rapport d'aspect.

Le menu **Choisissez une action** du P4 place le logo Bitcoin en haut au centre,
entièrement sous le séparateur d'en-tête, au-dessus de quatre boutons centrés de
**384 × 64 pixels** (80 % de la largeur),
et propose **RÉCUPÉRER UMBREL / LND**. Ce parcours déchiffre AEZEED avec
les paramètres scrypt officiels dans une allocation PSRAM temporaire d'environ
16 Mio, puis expose le `xprv` maître BIP32 dans un QR temporaire.
Le retour ou 15 secondes d'affichage ferment et effacent cette session, sans décompte.
Il ne restaure pas les canaux Lightning.
Tous les écrans P4 vérifient aussi que leurs informations et contrôles restent
sous le séparateur sans le masquer ni le couper.
Les boutons d'action standards du P4, dont
**PRÉCÉDENT** et **SUIVANT**, utilisent eux aussi une hauteur uniforme de
64 pixels.

## Capteurs et collecte

- ESP32-2432S028R : coordonnées, pression résistive, temps, photorésistance et RNG matériel.
- Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 : coordonnées capacitives et temps, RNG matériel, deux microphones via ES7210.
- OV5647 : détection au début de chaque collecte. Absence normale sur la carte sans caméra ; connecter une caméra compatible **appareil éteint**, puis recommencer la collecte. Ce n'est pas une prise en charge du branchement à chaud.
- Audio : blocs PCM 16 bits, deux canaux, 16 kHz ; indicateur de niveau sonore.
- Caméra : capture RGB565 via CSI/ISP, SHA-256 des pixels acquis et aperçu local à fréquence limitée. Aucun faux échantillon si une lecture échoue.
- Le mélange identifie chaque source et sa séquence. Les blocs et images sont complémentaires : **320 échantillons tactiles restent nécessaires**, sans modification des choix BIP39. La double saisie de passphrase vient ensuite, après arrêt confirmé des capteurs.
- Rouge < 50 %, orange de 50 à 99 %, vert à 100 %. Ce sont des seuils de collecte, **pas une mesure certifiée de bits d'entropie**. Les capteurs ambiants peuvent être prévisibles ou manipulés ; ils ne remplacent pas le RNG matériel.

## Vie privée et arrêt

Les pilotes sont démarrés uniquement sur l'écran de collecte. Aucun audio/image n'est écrit sur microSD, dans les journaux ou envoyé sur un réseau. Avant tout changement d'écran, l'interface demande l'arrêt des tâches et attend leur confirmation ; les tampons sont effacés. Une fermeture non confirmée bloque le passage aux secrets et demande un redémarrage. L'amplificateur reste éteint.

Le coprocesseur radio ESP32-C6 est maintenu en reset actif bas sur GPIO54 selon le profil P4 SDIO utilisé par Waveshare. Aucun pilote réseau n'est lancé. Ce brochage et l'absence d'activité radio doivent être vérifiés sur la révision exacte de la carte ; le logiciel ne peut garantir une isolation physique avant son démarrage.

## Compatibilité microSD

Sur **P4 2.0.3**, la création/restauration initiale fonctionne sans carte.
La microSD FAT32 doit être détectée et sa racine lisible à l'entrée de la
sauvegarde/export, puis durant sa préparation et avant l'écriture. Sans carte,
l'écran **microSD requise** bloque l'export, avec **RÉESSAYER** et **FERMER**.
Une carte vide lisible est acceptée, sans formatage ni fichier de test.

L'ouverture et chaque consultation privée d'un fichier requièrent sa présence
sur la microSD et une nouvelle saisie du mot de passe. La relecture vérifie
l'empreinte SHA-256 du fichier et son authentification AES-GCM. Entre deux
consultations, seuls les renseignements publics, le nom et l'empreinte restent
en mémoire. Le mot de passe et la clé de déchiffrement sont effacés après utilisation.
Les secrets calculés et les saisies sont temporaires et effacés en sortie.
Uniquement en consultation authentifiée d'un fichier `.aurora`, les mots ont
un décompte de 3 minutes partagé entre les pages ; une clé privée
a 1 minute, hors déchiffrement. **RETOUR** ou l'expiration ferme toute la session.
La passphrase reste limitée à 15 secondes, traitement compris ; la préparation
d'export à 120 secondes. L'inactivité ferme les autres écrans après 120 secondes,
mais n'interrompt pas les vues chronométrées avant leur propre échéance.
Création et restauration manuelle : aucun décompte, inactivité limitée à 120 secondes.

| Firmware | Lecture | Nouvelle écriture |
| --- | --- | --- |
| Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 — 2.0.3 | V1 et V2, mot de passe | V1, 1 120 octets |
| ESP32-2432S028R — 1.9.7 | V1 et V2, mot de passe par consultation | V1 sans PIN, 1 120 octets |

FAT32, mêmes noms et suffixes sur les deux appareils. L'en-tête de 46 octets,
PBKDF2-HMAC-SHA-256 à 500 000 itérations à l'écriture dans 2.0.3, AES-256-GCM, le sel de
16 octets, le nonce de 12 octets et le tag de 16 octets restent inchangés.
V2 ajoute uniquement 80 octets chiffrés pour le vérificateur PIN.
Aucun fichier existant n'est converti automatiquement.

Le démarrage et le verrouillage nettoient les tampons possédés par l'application,
sans supprimer les sauvegardes de la microSD. Une coupure brutale ne permet pas
d'exécuter un effacement ; ce nettoyage ne garantit pas l'absence de rémanence
physique. L'export Electrum volontairement en clair reste disponible et doit
être traité comme une sauvegarde privée non chiffrée.

Le mot de passe `.aurora` et son KDF ne protègent **pas** ce fichier Electrum.
Ni le verrouillage ni le démarrage ne suppriment les sauvegardes microSD.
Le nettoyage ne couvre pas chaque zone RTC, registre, cache ou mémoire de
périphérique ; une mesure sur des tampons ciblés n’est pas une preuve de purge
physique complète.

Un fichier créé sur l'un est destiné à être ouvert sur l'autre avec le même mot de passe. Même seed + même passphrase BIP39 + même dérivation = même portefeuille. La compatibilité cryptographique n'élimine pas les essais croisés de lecture/écriture sur les vrais lecteurs SD. Aucun formatage automatique, aucun écrasement volontaire d'un fichier existant. Ne jamais retirer une carte pendant une écriture.

## Compilation

Les versions résolues, les correctifs locaux et la procédure de revue des avis
de sécurité sont décrits dans [DEPENDENCIES.md](../../DEPENDENCIES.md).
Conserver `dependencies.lock` avec les sources ; ne pas le supprimer pour
contourner une incompatibilité de compilation ou de correctif.

Depuis la racine du dépôt, avec PlatformIO Core 6.1.19 ou ultérieur et accès aux registres de dépendances :

```powershell
# P4 révision 3.x, PSRAM 250 MHz
.\targets\waveshare_p4\build.ps1

# P4 révision 1.3, PSRAM 200 MHz
.\targets\waveshare_p4\build.ps1 -Target waveshare-p4-rev1
```

Le P4 utilise ESP-IDF 5.5.5, BSP Waveshare 1.0.1 et LVGL 9.5.0. Son environnement est distinct du SDK Arduino du CYD. Les sources partagées ne sont ni recopiées ni exportées. Les premiers téléchargements d'outils sont volumineux.

Compiler les profils P4 successivement : ils partagent les dépendances gérées. Le script empêche deux compilations P4 simultanées afin qu'une installation de dépendances ne perturbe pas l'autre. Lancer également la compilation CYD séparément : les plateformes peuvent remplacer certains outils partagés de PlatformIO. Cela n'empêche pas le développement et l'utilisation des deux appareils en parallèle.

ESP-IDF refuse les espaces dans les chemins de compilation. Le script Windows utilise le **nom court 8.3 du même dossier** : ce n'est ni un second répertoire ni une copie. Les fichiers restent tous dans le projet. Sur un système avec un chemin sans espaces, on peut utiliser directement `pio run -d targets/waveshare_p4 -e waveshare-p4` (ou `waveshare-p4-rev1`).

Ne pas flasher tant que la révision du P4 et le port série ne sont pas identifiés. Aucun flash P4 automatique ni mise à jour du Web Flasher n'est effectué par ces commandes. Ne pas charger une image CYD sur P4, ni une image P4 sur CYD. Le profil de partition P4 vise un flash de 32 Mo.

Les deux familles de silicium sont incompatibles : le profil `waveshare-p4` est limité aux révisions 3.0–3.99, et `waveshare-p4-rev1` aux révisions 1.0–1.99 (dont 1.3). Les en-têtes de l'application **et du bootloader** portent ces limites. Les paramètres de compilation sont contrôlés ; une ancienne configuration incohérente bloque le build au lieu de produire une image mal identifiée. Après modification des profils, supprimer uniquement le fichier généré `sdkconfig.waveshare-p4` ou `sdkconfig.waveshare-p4-rev1` concerné pour repartir de ses valeurs par défaut.

## Tests sur ordinateur

```powershell
cmd /c tests\native\run.cmd
python tests/crypto/run.py
python tests/ui/run.py
python tests/ui/run.py --cyd
python tests/crypto/verify_p4_images.py --profile waveshare-p4
powershell -File tests/release/verify.ps1 -ReleasedArtifactsOnly
```

Les tests natifs nécessitent les outils C++ Visual Studio et Python ; ceux de l'interface et du chiffrement utilisent les dépendances téléchargées par le build P4. Contrôler l'image immédiatement après chaque compilation avec le profil correspondant (`waveshare-p4` ou `waveshare-p4-rev1`) : changer de profil peut recréer le répertoire de build et supprimer l'autre image. Ils couvrent le mélange, V1/V2, les écrans 480 × 800, le verrou d'arrêt des capteurs, les vecteurs cryptographiques, le rejet des fichiers altérés, les limites de silicium et les octets de l'image factory. Les cartes et capteurs sont simulés dans les tests PC : ils ne remplacent pas la recette ci-dessous.

Lancer les tests UI **après** le build P4 et les tests crypto, sans compilation P4 simultanée : la configuration ESP-IDF peut remplacer les sources LVGL gérées. Le verrou de fichier empêche cette concurrence. Les tests P4 couvrent le mot de passe par catégorie privée, la liaison au fichier, les erreurs de relecture, la double passphrase, les expirations et l'effacement avant libération. Les tests de vérificateurs PIN concernent uniquement la compatibilité des anciens fichiers V2. Les tests de mémoire et de temporaires sont détaillés dans [tests/ui/README.md](../../tests/ui/README.md).

## Recette matérielle obligatoire

1. Vérifier la révision silicium, la mémoire, le reset C6 et l'absence d'initialisation radio.
2. Démarrer sans caméra : E00, 480 × 800, tactile dans les quatre coins, aucune pression/LDR annoncée.
3. Parcourir tous les écrans : clavier, suggestions, 12 à 24 mots, QR public/privé et restauration ; aucune coupure ni chevauchement.
4. Vérifier microphones, puis refaire la collecte avec OV5647 : image, compteur réel, variations sonores, absence de données après sortie.
5. Tester annulation, redémarrage de collecte, source muette/bloquée et erreurs I2C/CSI ; aucun accès aux secrets si l'arrêt échoue.
6. Échanger un portefeuille **de test sans fonds** dans les deux sens entre CYD et P4 ; comparer adresse, dérivation et exports.
7. Sur P4, créer/restaurer sans SD jusqu'au portefeuille, sans PIN ; la sauvegarde doit exiger une carte FAT32 lisible. Ouvrir des fichiers V1/V2 avec leur mot de passe, puis vérifier qu'une nouvelle consultation privée le redemande et relit le même fichier. Tester absence/retrait de carte hors écriture, substitution de fichier, carte pleine, fichier existant, mauvais mot de passe, annulation et fichier altéré : aucune révélation après échec, aucun formatage ni perte d'une sauvegarde préexistante. L'ESP32-2432S028R 1.9.7 redemande également le mot de passe pour les consultations privées. Ne pas retirer pendant une écriture.
8. Tester le décompte des mots à 180 s, commun aux pages, et celui des clés à 60 s, hors déchiffrement. Vérifier la fermeture complète par RETOUR et à expiration, la passphrase à 15 s, l'export et l'inactivité des autres écrans à 120 s. Contrôler l'effacement au verrouillage, le nettoyage au démarrage et la stabilité mémoire avec uniquement des données publiques de test.
9. Avec une seed AEZEED de test sans fonds, ouvrir **RÉCUPÉRER UMBREL / LND**, vérifier le résultat avec et sans passphrase, puis importer le XPRV dans Sparrow. Comparer les premières adresses des comptes BIP49, BIP84 et BIP86. Confirmer aussi qu'une mauvaise passphrase est rejetée, que le QR privé expire après 15 s sans décompte et qu'aucun canal Lightning n'est présenté comme récupéré.

## Références matérielles et pilotes

- [Waveshare : carte et exemples ESP-IDF](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3)
- [BSP Waveshare](https://github.com/waveshareteam/Waveshare-ESP32-components/tree/master/bsp/esp32_p4_wifi6_touch_lcd_4_3)
- [Espressif : pilotes vidéo](https://github.com/espressif/esp-video-components/tree/master/esp_video)
- [Espressif : codecs audio](https://github.com/espressif/esp-adf/tree/master/components/esp_codec_dev)
- [Espressif : configuration SDIO et reset du coprocesseur](https://github.com/espressif/esp-hosted-mcu/blob/main/host/mcu/eh_host_mcu_transport/Kconfig.host.sdio)
