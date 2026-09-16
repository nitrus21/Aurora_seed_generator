# AURORA · Les versions distribuées

Trois versions, deux références de cartes. Les notes détaillent **toutes les
fonctions présentes**, puis les ajouts depuis la version de comparaison.

| Version | Carte | Rôle |
| --- | --- | --- |
| [1.7.5](releases/1.7.5.md) | ESP32-2432S028R — Cheap Yellow Display | Ancienne version conservée, binaire inchangé. |
| [1.9.7](releases/1.9.7.md) | ESP32-2432S028R — Cheap Yellow Display | Dernière version de cette branche. |
| [2.0.3](releases/2.0.3.md) | Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 | Branche active ; images silicium 1.x / 3.x. |

## ESP32-2432S028R : 1.7.5 → 1.9.7

- Collecte : 320 échantillons au lieu de 160, lumière et aperçu indépendant.
- Passphrase confirmée deux fois après collecte.
- Mot de passe redemandé par consultation privée du fichier.
- Nettoyage des temporaires, contrôles des erreurs et protection contre les dumps de crash.
- KDF des nouveaux fichiers : 500 000 itérations au lieu de 120 000.
- Mots du fichier : 3 minutes ; clé privée : 1 minute ; RETOUR ferme la session.
- Polices et couleurs du clavier de la 1.7.5 conservées.
- Douze mots par page au lieu de huit.

Depuis la précédente version publiée, **1.9.5** : source RNG des appels
cryptographiques contrôlée, KDF renforcé et décomptes des consultations privées.

## Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 : 2.0.0 → 2.0.3

- Source RNG contrôlée pour les appels cryptographiques.
- Tampons de lecture/écriture SD nettoyés explicitement.
- KDF à 500 000 itérations, accéléré par blocs sans conserver les clés.
- Décomptes 3 minutes / 1 minute pour les consultations privées du fichier uniquement.
- Suivi des dépendances et tests de non-régression étendus.

Le P4 conserve son écran portrait, ses 12 mots par page, ses grands QR,
la collecte audio/vidéo facultative et la récupération AEZEED vers BIP32.
Depuis la 2.0.0, les nouvelles fonctionnalités se développent sur cette carte.
La branche ESP32-2432S028R s’arrête en 1.9.7.

Les anciens fichiers restent lisibles, sans renforcement automatique.
**Projet expérimental ; limites physiques et export Electrum en clair :
[précautions](https://github.com/nitrus21/Aurora_seed_generator/blob/main/SECURITY.md).**
