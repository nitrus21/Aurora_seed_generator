# AURORA — ESP32-P4

Version P4 en développement : **2.0.0**, branche `codex/waveshare-p4-480x800`. Le CYD reste figé en **1.9.2**. La microSD requise à la sauvegarde, les protections PIN et la confirmation de passphrase sont décrites dans [SECURITY.md](../../SECURITY.md), sans modification des dérivations du portefeuille ou du fichier.
Carte visée : **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3**, sans suffixe `-C`.
Ne pas confondre une compilation et un démarrage réussis avec une certification indépendante : les limites physiques et risques résiduels documentés restent applicables.

Validation matérielle finale : AURORA 1.9.2 a été écrite sur un P4 révision
1.3 de 32 Mo ; le démarrage série a confirmé l'application 1.9.2, la PSRAM
32 Mo à 200 MHz, le GT911 et l'autotest E00 en 1 238 ms, sans panic ni
redémarrage pendant 35 secondes.

## Un seul noyau, deux firmwares

Le dossier racine reste le seul dépôt de travail. Il n'y a pas de copie à synchroniser : ce projet P4 compile directement les fichiers communs du dossier `src/` et les en-têtes de `include/`.

| Élément | Code commun / différences |
| --- | --- |
| BIP39, BIP32, BIP44/49/84/86, autotests | `src/wallet.cpp`, même uBitcoin épinglé et durci |
| Fichiers chiffrés `.aurora`, exports Electrum | `src/sd_export.cpp`, lecture V1 et écriture/lecture V2 communes |
| PIN et effacement des allocations LVGL | `src/pin_security.cpp`, `src/secure_lvgl_memory.c` |
| Mélange aléatoire et aperçu HMAC indépendant | `include/entropy.h` |
| Parcours et actions utilisateur | `src/ui.cpp` |
| CYD | Arduino, TFT_eSPI, XPT2046, LDR GPIO34, microSD SPI |
| P4 | ESP-IDF, BSP Waveshare, LVGL 9, GT911, ES7210, OV5647 facultative, microSD SDMMC |

Les écrans P4 sont rendus avec des widgets natifs dans une surface **480 × 800 portrait** et des polices agrandies, pas dans un framebuffer 320 × 240 étiré. L'écran d'entropie possède une disposition portrait dédiée. Les QR conservent leur forme carrée ; la caméra conserve son rapport d'aspect.

Le menu **Choisissez une action** du P4 place le logo Bitcoin en haut au centre,
entièrement sous le séparateur d'en-tête, au-dessus de quatre boutons centrés de
**384 × 64 pixels** (80 % de la largeur),
et propose **RÉCUPÉRER UMBREL / LND**. Ce parcours déchiffre AEZEED avec
les paramètres scrypt officiels dans une allocation PSRAM temporaire d'environ
16 Mio, puis expose le `xprv` maître BIP32 derrière le PIN de session. Il ne
restaure pas les canaux Lightning.
Ce changement de disposition ne concerne pas le CYD.
Tous les écrans P4 vérifient aussi que leurs informations et contrôles restent
sous le séparateur sans le masquer ni le couper.
Les quatre boutons actuels conservent leur position verticale afin de réserver
une cinquième rangée de 64 pixels. Les boutons d'action standards du P4, dont
**PRÉCÉDENT** et **SUIVANT**, utilisent eux aussi une hauteur uniforme de
64 pixels.

## Capteurs et collecte

- CYD : coordonnées, pression résistive, temps, photorésistance et RNG matériel conservés.
- P4 : coordonnées capacitives et temps, RNG matériel, deux microphones ES7210 ; aucune pression ni photorésistance inventées.
- OV5647 : détection au début de chaque collecte. Absence normale sur la carte sans caméra ; connecter une caméra compatible **appareil éteint**, puis recommencer la collecte. Ce n'est pas une prise en charge du branchement à chaud.
- Audio : blocs PCM 16 bits, deux canaux, 16 kHz ; indicateur de niveau sonore.
- Caméra : capture RGB565 via CSI/ISP, SHA-256 des pixels acquis et aperçu local à fréquence limitée. Aucun faux échantillon si une lecture échoue.
- Le mélange identifie chaque source et sa séquence. Les blocs et images sont complémentaires : **320 échantillons tactiles restent nécessaires**, sans modification des choix BIP39. La double saisie de passphrase vient ensuite, après arrêt confirmé des capteurs.
- Rouge < 50 %, orange de 50 à 99 %, vert à 100 %. Ce sont des seuils de collecte, **pas une mesure certifiée de bits d'entropie**. Les capteurs ambiants peuvent être prévisibles ou manipulés ; ils ne remplacent pas le RNG matériel.

## Vie privée et arrêt

Les pilotes sont démarrés uniquement sur l'écran de collecte. Aucun audio/image n'est écrit sur microSD, dans les journaux ou envoyé sur un réseau. Avant tout changement d'écran, l'interface demande l'arrêt des tâches et attend leur confirmation ; les tampons sont effacés. Une fermeture non confirmée bloque le passage aux secrets et demande un redémarrage. L'amplificateur reste éteint.

Le coprocesseur radio ESP32-C6 est maintenu en reset actif bas sur GPIO54 selon le profil P4 SDIO utilisé par Waveshare. Aucun pilote réseau n'est lancé. Ce brochage et l'absence d'activité radio doivent être vérifiés sur la révision exacte de la carte ; le logiciel ne peut garantir une isolation physique avant son démarrage.

## Compatibilité microSD

Sur **CYD et P4**, la création/restauration et le PIN de session fonctionnent
sans carte. La microSD FAT32 doit être détectée et sa racine lisible à l'entrée
de la sauvegarde/export, puis durant sa préparation et avant l'écriture. Sinon
l'écran **microSD requise** bloque l'export, avec **RÉESSAYER** et **FERMER**
seulement. Une carte vide lisible est acceptée, sans formatage ni fichier de
test. L'ouverture d'un fichier conserve les erreurs de carte dans le lecteur.
Pas de surveillance continue de chaque frappe ni de changement des règles PIN.

FAT32, mêmes noms et suffixes sur les deux appareils. Lecture V1 conservée ; nouvelles écritures V2 avec PIN par fichier. En-tête de 46 octets, PBKDF2-HMAC-SHA-256 (120 000 itérations à l'écriture), AES-256-GCM et tag de 16 octets restent inchangés. V2 ajoute 80 octets chiffrés pour le vérificateur PIN : fichier total de 1 200 octets contre 1 120 en V1. Aucun champ spécifique au matériel. Les anciens firmwares ne lisent pas V2 ; mettre les deux cartes à jour. [Migration V1/V2](../../SECURITY.md#format-binaire-v2-et-migration).

Un fichier créé sur l'un est destiné à être ouvert sur l'autre avec le même mot de passe. Même seed + même passphrase BIP39 + même dérivation = même portefeuille. La compatibilité cryptographique n'élimine pas les essais croisés de lecture/écriture sur les vrais lecteurs SD. Aucun formatage automatique, aucun écrasement volontaire d'un fichier existant. Ne jamais retirer une carte pendant une écriture.

## Compilation

Depuis la racine du dépôt, avec PlatformIO Core 6.1.19 ou ultérieur et accès aux registres de dépendances :

```powershell
# Appareil d'origine
pio run -e esp32-2432S028R

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
python tests/crypto/verify_p4_images.py
powershell -File tests/release/verify.ps1 -ReleasedArtifactsOnly
```

Les tests natifs nécessitent les outils C++ Visual Studio et Python ; ceux de l'interface et du chiffrement utilisent les dépendances téléchargées par le build P4. Le contrôle des images nécessite une compilation des deux profils. Ils couvrent le mélange, les données du format V1, les écrans 480 × 800, le verrou d'arrêt des capteurs, les vecteurs cryptographiques, le rejet des fichiers altérés, les limites de silicium et les octets de l'image factory. Les cartes et capteurs sont simulés dans les tests PC : ils ne remplacent pas la recette ci-dessous.

Lancer les tests UI **après** le build P4 et les tests crypto, sans compilation P4 simultanée : la configuration ESP-IDF peut remplacer les sources LVGL gérées. Le verrou de fichier empêche cette concurrence. Les tests supplémentaires couvrent V2/PIN, double passphrase, expiration, fermeture après trois erreurs et effacement des allocations LVGL avant libération.

Résultats et limites de la vérification logicielle : [VALIDATION.md](VALIDATION.md).

## Recette matérielle obligatoire

1. Vérifier la révision silicium, la mémoire, le reset C6 et l'absence d'initialisation radio.
2. Démarrer sans caméra : E00, 480 × 800, tactile dans les quatre coins, aucune pression/LDR annoncée.
3. Parcourir tous les écrans : clavier, suggestions, 12 à 24 mots, QR public/privé et restauration ; aucune coupure ni chevauchement.
4. Vérifier microphones, puis refaire la collecte avec OV5647 : image, compteur réel, variations sonores, absence de données après sortie.
5. Tester annulation, redémarrage de collecte, source muette/bloquée et erreurs I2C/CSI ; aucun accès aux secrets si l'arrêt échoue.
6. Échanger un portefeuille **de test sans fonds** dans les deux sens entre CYD et P4 ; comparer adresse, dérivation et exports.
7. Sur les deux cartes, créer/restaurer sans SD jusqu'au portefeuille, y compris passphrase et PIN de session ; le dialogue SD ne doit apparaître qu'à la sauvegarde/export. À l'ouverture d'un fichier sans carte, vérifier l'erreur du lecteur. Réessayer un export sans carte doit rester bloqué ; une carte FAT32 vide doit permettre de poursuivre. Retirer la carte pendant la préparation de l'export puis valider : saisie effacée, aucune écriture ni tentative PIN consommée. Réinsérer et réessayer, puis vérifier FERMER et la conservation du compteur d'erreurs PIN. Tester aussi carte pleine, fichier existant, mauvais mot de passe et fichier altéré ; aucun formatage ni perte d'un fichier préexistant. Ne pas retirer pendant une écriture.
8. Tester double passphrase, PIN avec zéros initiaux, trois erreurs séparées par annulation, expiration à 15 s et inactivité à 120 s. Mesurer la latence PIN et la stabilité mémoire sur plusieurs cycles ; confirmer que l'adresse reste identique à celle d'un logiciel de référence.
9. Avec une seed AEZEED de test sans fonds, ouvrir **RÉCUPÉRER UMBREL / LND**, vérifier le résultat avec et sans passphrase, puis importer le XPRV dans Sparrow. Comparer les premières adresses des comptes BIP49, BIP84 et BIP86. Confirmer aussi qu'une mauvaise passphrase est rejetée, que le QR privé expire après 15 s et qu'aucun canal Lightning n'est présenté comme récupéré.

## Références matérielles et pilotes

- [Waveshare : carte et exemples ESP-IDF](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3)
- [BSP Waveshare](https://github.com/waveshareteam/Waveshare-ESP32-components/tree/master/bsp/esp32_p4_wifi6_touch_lcd_4_3)
- [Espressif : pilotes vidéo](https://github.com/espressif/esp-video-components/tree/master/esp_video)
- [Espressif : codecs audio](https://github.com/espressif/esp-adf/tree/master/components/esp_codec_dev)
- [Espressif : configuration SDIO et reset du coprocesseur](https://github.com/espressif/esp-hosted-mcu/blob/main/host/mcu/eh_host_mcu_transport/Kconfig.host.sdio)
