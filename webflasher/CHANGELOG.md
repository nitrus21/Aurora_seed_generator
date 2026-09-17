# AURORA · Versions distribuées

| Version | Carte | Rôle |
| --- | --- | --- |
| [1.7.5](releases/1.7.5.md) | ESP32-2432S028R — Cheap Yellow Display | Version historique conservée. |
| [1.9.9](releases/1.9.9.md) | ESP32-2432S028R — Cheap Yellow Display | Version finale de cette carte. |
| [2.0.3](releases/2.0.3.md) | Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 | Version P4, images silicium 1.x et 3.x. |

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

## Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 : 2.0.3

- Interface portrait 480 × 800, douze mots par page et grands QR.
- Images distinctes pour silicium P4 1.x et 3.x.
- Collecte tactile et audio, caméra OV5647 facultative.
- KDF PBKDF2-HMAC-SHA-256 à 500 000 itérations, accéléré sur P4.
- Consultations privées avec mot de passe, décomptes de 3 minutes et 1 minute.
- Récupération AEZEED Umbrel/LND vers une clé maître BIP32.

Les fichiers `.aurora` V1/V2 restent lisibles. L’export Electrum privé contient
une clé étendue en clair sur microSD et doit être conservé comme un secret.
