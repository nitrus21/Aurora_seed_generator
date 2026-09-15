# AURORA — ESP32-2432S028R (CYD)

## 1.9.4 — affichage

Tailles de police de la version 1.7.5, avec les fonctionnalités et protections
de la 1.9.3. Le firmware Waveshare ESP32-P4 reste en 2.0.0.

| Élément | Taille |
| --- | --- |
| Titres courants | 14 px |
| Titre « Mot de passe Aurora Wallet » | 12 px |
| Configuration : libellés, nombres et types d’adresse | 10 px |
| Boutons | 12 px |
| Mots de récupération / numéros | 16 px / 12 px |
| Saisie d’un mot BIP39 | 14 px |
| Saisie passphrase / mot de passe d’ouverture | 12 px |
| Deux champs du mot de passe de sauvegarde | 10 px |
| Informations et consignes courantes | 10 px |

Les avertissements spécifiques conservent leurs tailles d’origine. Les nouveaux
écrans utilisent la même hiérarchie. Les actions restent dans la surface 320 × 240.

Conservés : 320 échantillons de collecte, double saisie de passphrase, mot de passe
par consultation privée, lecture V1/V2, écriture V1, export Electrum, délais de
fermeture, nettoyage mémoire et protections contre les sauvegardes de crash.

## Vérification locale

```powershell
python tests/ui/run.py --cyd
python tests/ui/run.py
python tests/crypto/run.py
pio run --environment esp32-2432S028R
```

Les tests CYD contrôlent les polices des widgets réels et les parcours de sécurité.
La compilation contrôle aussi le binaire lié et la partition de nettoyage.
Les captures utilisent uniquement des fixtures publiques sans fonds.

Installer l’image fusionnée à `0x0000` ; elle inclut la table de partitions
requise depuis la 1.9.3. Les sauvegardes microSD ne sont pas effacées.
