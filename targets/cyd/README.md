# AURORA · ESP32-2432S028R

### L’expérience Bitcoin hors ligne au format compact

**Cheap Yellow Display (CYD) · Écran 2,8″ · Version 1.9.7**

![AURORA](../../assets/splash_320x240.png)

Créez une phrase de récupération, vérifiez vos mots et emportez une sauvegarde
chiffrée sur microSD. Le tout depuis l’écran tactile, avec des consignes en français.

[Installer](https://nitrus21.github.io/Aurora_seed_generator/) ·
[Parcours guidé](../../README.md#votre-premier-portefeuille-en-6-étapes) ·
[Notes 1.9.7](../../webflasher/releases/1.9.7.md)

## Ce que vous pouvez faire

- Créer ou restaurer une phrase BIP39 de 12, 15, 18, 21 ou 24 mots.
- Ajouter et confirmer une passphrase BIP39 optionnelle.
- Choisir Legacy, Nested SegWit, Native SegWit ou Taproot.
- Vérifier trois mots recopiés ; consulter l’adresse, la clé publique étendue et leurs QR.
- Ouvrir ou créer un fichier `.aurora` chiffré, avec mot de passe à chaque consultation privée.
- Exporter vers Electrum en connaissance de cause : ce fichier privé est **en clair**.

La collecte associe RNG matériel, gestes tactiles, timings et photorésistance.
Les 320 échantillons indiquent une collecte terminée, pas une mesure certifiée
d’entropie. Umbrel/LND et les capteurs audio/vidéo sont propres à la
Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3, pas à cette carte.

## Une interface adaptée à ses 320 × 240 pixels

La 1.9.7 conserve les polices compactes et les couleurs de clavier de la 1.7.5.
Les mots sont présentés par groupes de douze maximum, en deux colonnes.

| Élément | Taille |
| --- | --- |
| Titres courants | 14 px |
| Titre du formulaire de mot de passe | 12 px |
| Boutons | 12 px |
| Mots / numéros | 16 px / 12 px |
| Saisie BIP39 | 14 px |
| Informations courantes | 10 px |

## Une sauvegarde, puis des consultations ponctuelles

Les nouveaux fichiers `.aurora` utilisent AES-256-GCM et un KDF
PBKDF2-HMAC-SHA-256 à **500 000 itérations**. Les anciennes sauvegardes V1/V2
restent lisibles, sans conversion automatique.

Après ouverture, seules les informations publiques et l’identité du fichier
restent entre deux consultations. Le mot de passe est redemandé pour relire
les données privées ; pas de PIN ni de clé de déchiffrement conservée.

- **Mots depuis un fichier : 3 minutes**, communes à toutes les pages.
- **Clé privée depuis un fichier : 1 minute**, hors déchiffrement.
- **RETOUR**, expiration ou **VERROUILLER** ferment ces vues et nettoient la session.
- Création et restauration manuelle : pas de décompte ; **2 minutes d’inactivité**.

La passphrase consultée est limitée à 15 secondes, calcul compris ; la
préparation d’export à 120 secondes. La microSD est nécessaire pour les fichiers,
pas pour une création/restauration initiale. Ne la retirez jamais pendant une écriture.

## Installer

Sélectionnez **ESP32-2432S028R — 1.9.7** dans le Web Flasher. L’image complète
`.factory.bin` s’installe à **0x0000**, avec sa table de partitions.
N’installez pas l’application seule sur une ancienne table de partitions.
Déconnectez les données USB après installation et utilisez une alimentation autonome.

La **1.7.5** reste téléchargeable, inchangée, mais ne bénéficie pas des corrections
actuelles. **La branche ESP32-2432S028R se termine en 1.9.7** ; les nouvelles
fonctionnalités se développent sur la Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3.

## Compiler

Depuis la racine du dépôt, avec PlatformIO :

```powershell
pio run -e esp32-2432S028R
pio run -e esp32-2432S028R --target upload --upload-port COM3
pio device monitor --port COM3 --baud 115200
```

Remplacez `COM3` par le port identifié. La cible utilise Arduino, TFT_eSPI,
ILI9341, XPT2046 et microSD SPI. Les broches sont définies dans
[`board_config.h`](../../include/board_config.h). Le build racine ne compile pas le P4.
L’autotest de démarrage attendu est **E00**.

## Vérifier

```powershell
python tests/ui/run.py --cyd
python tests/crypto/run.py
python tests/rng/run.py
python tests/cyd_security/run.py
```

Les tests hôte nécessitent les outils C++ Visual Studio et les dépendances
indiquées dans leurs README. L’image 1.9.7 a été flashée et vérifiée par relecture,
avec autotest E00 réussi ; cela ne remplace pas une recette complète des opérations
microSD, des durées réelles de déchiffrement et des coupures sur appareil.

> [!WARNING]
> Projet expérimental, sans élément sécurisé ni garantie de purge physique après
> coupure. Le nettoyage ne supprime pas les sauvegardes microSD. L’export Electrum
> contient des secrets en clair. Lisez les [limites de sécurité](../../SECURITY.md).
