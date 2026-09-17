# AURORA · ESP32-2432S028R

### L’expérience Bitcoin hors ligne au format compact

**Cheap Yellow Display (CYD) · Écran résistif 2,8″ · Version finale 1.9.9**

![AURORA](../../assets/splash_320x240.png)

Créez une phrase de récupération, vérifiez vos mots et conservez une sauvegarde
chiffrée sur microSD depuis une interface française compacte.

[Installer](https://nitrus21.github.io/Aurora_seed_generator/) ·
[Parcours guidé](../../README.md#votre-premier-portefeuille-en-6-étapes) ·
[Notes 1.9.9](../../webflasher/releases/1.9.9.md)

## Fonctions principales

- Création ou restauration BIP39 de 12, 15, 18, 21 ou 24 mots anglais.
- Passphrase BIP39 optionnelle et vérification de trois mots.
- Legacy, Nested SegWit, Native SegWit et Taproot sur Bitcoin Mainnet.
- Adresse, clé publique étendue et clé privée en texte ou QR.
- Collecte associant RNG matériel, tactile, timings et photorésistance.
- Sauvegarde `.aurora` chiffrée sur microSD FAT32.
- Lecture V1/V2 et nouvelles sauvegardes V1 sans PIN.
- Export Electrum privé sur microSD.

## Une interface adaptée au 320 × 240

La 1.9.9 conserve les polices compactes et les couleurs de clavier de la 1.7.5.
Les mots sont présentés par groupes de douze, en deux colonnes.

| Élément | Taille |
| --- | --- |
| Titres courants | 14 px |
| Titre du formulaire de mot de passe | 12 px |
| Boutons | 12 px |
| Mots / numéros | 16 px / 12 px |
| Saisie BIP39 | 14 px |
| Informations courantes | 10 px |

Dans un portefeuille ouvert depuis un fichier :

- **CLÉ PUBLIQUE** utilise un bouton vert.
- **CLÉ PRIVÉE**, **MOTS** et une passphrase disponible utilisent des boutons rouges.
- Sans passphrase BIP39, son bouton reste visible, désactivé et grisé.

## Sauvegarde et consultation

Les fichiers `.aurora` utilisent AES-256-GCM et PBKDF2-HMAC-SHA-256.
Les nouvelles sauvegardes 1.9.9 utilisent 120 000 itérations. Les fichiers V1/V2
existants restent lisibles sans conversion automatique.

Après l’ouverture, seules les informations publiques et l’identité du fichier
restent disponibles entre deux consultations. Le mot de passe est redemandé pour
les mots, la passphrase, la clé privée et la préparation d’un export.

- Mots du fichier : **3 minutes**, décompte commun à toutes les pages.
- Clé privée du fichier : **1 minute**, hors déchiffrement.
- Passphrase : **15 secondes**, traitement compris.
- Préparation d’export : **120 secondes**.
- **RETOUR** efface la consultation privée et retrouve le portefeuille public.
- **VERROUILLER** ou l’expiration ferme entièrement la session.

La microSD est nécessaire pour ouvrir, sauvegarder ou exporter un fichier, mais
pas pour créer ou restaurer initialement une phrase BIP39.

## Installation

Sélectionnez **ESP32-2432S028R — 1.9.9** dans le Web Flasher. L’image complète
`aurora-1.9.9-esp32-2432s028r.factory.bin` s’installe à **0x0000**.
La version historique 1.7.5 reste disponible séparément.

Déconnectez les données USB après l’installation et utilisez une alimentation
autonome avant toute manipulation de secrets. Vérifiez l’ouverture de chaque
sauvegarde avant de compter sur elle.

La branche ESP32-2432S028R est terminée en 1.9.9. Les futures fonctionnalités
d’AURORA sont développées pour la Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3.

> [!WARNING]
> AURORA est un projet expérimental. L’export Electrum privé contient une clé
> étendue en clair sur microSD. Le verrouillage ne supprime pas les fichiers de la carte.
