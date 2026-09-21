# Protocole INT-A1 — marge de pile P4 rev1.3

## Objet et limites

Le profil temporaire `waveshare-p4-rev1-stack-health` exécute l'interface
AURORA normale et mesure le minimum de pile libre (`high-water mark`) des deux
tâches qui portent le code applicatif : `app_main` et le worker `lvgl`. Dans le
port FreeRTOS d'ESP-IDF, ces valeurs sont exprimées en octets.

Le verdict PASS utilise un seuil d'ingénierie conservateur de 25 % de la pile
configurée : 6 144 octets pour `app_main` (24 576 octets) et 2 048 octets pour
`lvgl` (8 192 octets). Ce seuil n'est pas une norme ni une certification. Le
canari FreeRTOS reste activé indépendamment et provoquerait un arrêt si un
débordement était détecté.

## Invariants de confidentialité

- utiliser exclusivement le portefeuille public de test prévu pour la campagne ;
- aucune seed réelle, passphrase réelle ou clé privée réelle ;
- aucune valeur de pile ni donnée applicative n'est écrite sur microSD ;
- aucun journal applicatif, fichier de diagnostic ou export série n'est créé ;
- seuls les minima agrégés et le verdict sont affichés dans « À propos » ;
- aucune eFuse et aucune autre partition que l'application ne doit être écrite.

## Campagne physique autorisée séparément

1. Identifier le port et la révision sans écriture, sauvegarder puis hacher la
   partition application complète.
2. Avec autorisation explicite, écrire uniquement l'application diagnostique
   rev1 et vérifier l'écriture. Ne jamais utiliser `erase-all`.
3. Démarrer à froid puis exécuter, avec des données publiques, les parcours les
   plus lourds : création 24 mots avec passphrase de test, export `.aurora`,
   ouverture et mot de passe erroné, affichage mots/passphrase/clé privée,
   restauration BIP39, dérivations BIP44/49/84/86, récupération Umbrel/LND,
   navigation QR, verrouillage et absence de microSD.
4. Revenir à l'accueil, ouvrir « À propos », photographier les deux minima, le
   nombre d'échantillons et le verdict. Un échec ou une tâche LVGL introuvable
   interdit de conclure.
5. Restaurer exactement l'application AURORA 2.0.11, vérifier son SHA-256 sur
   toute la partition et confirmer visuellement l'écran normal.

Le résultat qualifie uniquement le P4 rev1.3 et les parcours réellement
exercés. Il ne qualifie ni le P4 rev3 ni un futur firmware modifié.
