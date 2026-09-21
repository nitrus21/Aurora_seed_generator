# AURORA · Versions distribuées

| Version | Carte | Rôle |
| --- | --- | --- |
| [1.7.5](releases/1.7.5.md) | ESP32-2432S028R — Cheap Yellow Display | Version historique conservée. |
| [1.9.9](releases/1.9.9.md) | ESP32-2432S028R — Cheap Yellow Display | Version finale de cette carte. |
| [2.0.3](releases/2.0.3.md) | Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 | Version P4, images silicium 1.x et 3.x. |
| [2.0.5](releases/2.0.5.md) | Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 | Sauvegarde Umbrel et export Sparrow. |
| [2.0.12](releases/2.0.12.md) | Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 | Dernière version P4 : collecte renforcée, seuil microphone adaptatif et qualification rev1.3. |
| [2.0.11](releases/2.0.11.md) | Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 | Durcissement sécurité, builds reproductibles et identité du firmware à l’écran. |
| [2.0.9](releases/2.0.9.md) | Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 | Navigation publique multi-comptes et dérivations verrouillées. |

## ESP32-2432S028R : 1.7.5 → 1.9.9

- Collecte portée à 320 échantillons tactiles, avec timings et lumière.
- Confirmation de la passphrase BIP39 et douze mots par page.
- Lecture des fichiers `.aurora` V1/V2 et nouvelles sauvegardes V1 sans PIN.
- Accès microSD fiabilisé pour la création et l’ouverture des sauvegardes.
- Mot de passe redemandé pour chaque consultation privée du fichier.
- Mots affichés pendant 3 minutes et clé privée pendant 1 minute.
- Retour au portefeuille public après effacement de la consultation privée.
- Bouton public vert, actions privées rouges et passphrase absente grisée.
- Nettoyage renforcé au verrouillage et au démarrage.
- Polices compactes et couleurs du clavier de la 1.7.5 conservées.

La branche ESP32-2432S028R est terminée en **1.9.9**. Les futures fonctionnalités
sont développées pour la Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3.

## Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 : 2.0.12

- Toutes les fonctions et protections de la 2.0.11.
- 512 mouvements tactiles qualifiés, environ 10 secondes d'activité et au moins
  6 zones couvertes sur 12.
- Aucun défilement public lorsque le doigt reste immobile ; validation explicite
  avec **TERMINER** après le minimum.
- Calibration du microphone et seuil adaptatif ; aucun audio brut affiché,
  journalisé ou stocké.
- Reproductibilité Windows/Linux et qualification de l'application canonique
  silicium 1.x sur P4 révision 1.3.

## Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 : 2.0.11

- Toutes les fonctions de la 2.0.9.
- Journaux de production ESP-IDF et second bootloader désactivés, sans eFuse.
- Protection de pile forte et builds reproductibles Windows/Linux.
- Page d’identification : version, profil silicium et SHA-256 de l’application.
- Libellés Legacy/SegWit/Taproot explicites et suppression de l’écran de clé
  publique enfant au profit de la clé étendue de compte.
- Validation physique complète du profil 1.x sur P4 révision 1.3 ; profil 3.x
  contrôlé par logiciel uniquement.

## Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 : 2.0.9

- Toutes les fonctions de la 2.0.5.
- Comptes publics BIP44/49/84/86 et vingt premières adresses après parcours BIP39.
- Comptes publics BIP49/84/86 et vingt premières adresses après récupération Umbrel/LND.
- Dérivation publique depuis les clés étendues de compte, sans clé privée.
- Les fichiers ouverts restent limités au type de dérivation qu’ils contiennent.

## Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 : 2.0.5

- Toutes les fonctions de la 2.0.3.
- QR du `xprv` maître Umbrel avec décompte de 3 minutes.
- Sauvegarde Umbrel `.aurora` protégée par mot de passe et réouverture du `xpub` public.
- Mot de passe exigé à chaque consultation du `xprv`.
- Export Sparrow rouge contenant le `xprv` maître en clair.
- Mots AEZEED et passphrase absents du fichier ; boutons correspondants désactivés.

## Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 : 2.0.3

- Interface portrait 480 × 800, douze mots par page et grands QR.
- Images distinctes pour silicium P4 1.x et 3.x.
- Collecte tactile et audio, caméra OV5647 facultative.
- KDF PBKDF2-HMAC-SHA-256 à 500 000 itérations, accéléré sur P4.
- Consultations privées avec mot de passe, décomptes de 3 minutes et 1 minute.
- Récupération AEZEED Umbrel/LND vers une clé maître BIP32.

Les fichiers `.aurora` V1/V2 restent lisibles. L’export Electrum privé contient
une clé étendue en clair sur microSD et doit être conservé comme un secret.
