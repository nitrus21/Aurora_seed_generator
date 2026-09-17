# Test natif du codec chiffré commun

Prérequis : Python 3, outils C++ Visual Studio et SDK ESP-IDF installé par la compilation P4.

```powershell
python tests/crypto/run.py
# Exception CYD explicitement autorisée : politique 120000 et lecteur CYD
python tests/crypto/run.py --cyd
```

Le test compile **`src/sd_export.cpp`, `src/pin_security.cpp` et Mbed TLS réel** avec une carte simulée uniquement en RAM. Il vérifie les vecteurs PBKDF2-HMAC-SHA-256/AES-256-GCM et BIP39 SHA-512 inchangés, l'équivalence des API, la lecture d’une fixture V1 de 1 120 octets, l'aller-retour V2 de 1 200 octets, le rejet d'un mauvais mot de passe, des altérations/troncatures, l'effacement après échec, le non-écrasement et le nettoyage d'une écriture dont la synchronisation échoue. Les tests de l’ancien vérificateur PIN sont conservés uniquement pour la compatibilité V2 : zéros initiaux, bornes 4–8, caractères refusés, sels indépendants et blocage après trois erreurs cumulées même séparées par un succès. Ils ne décrivent pas un parcours PIN actuel sur l’une ou l’autre carte.

Les parcours actuels sont couverts par l’écriture **V1 sans PIN**, de taille
inchangée, puis la relecture authentifiée. Le KDF de fichier conserve PBKDF2-HMAC-SHA-256
mais utilise 500 000 itérations sur P4 et 120 000 sur CYD 1.9.9 pour les
nouvelles sauvegardes. Les deux profils vérifient les fichiers V1/V2 à 10 000,
120 000 et 500 000, les bornes, les vecteurs différentiels, les en-têtes réellement
écrits et le rejet d'une modification non authentifiée du compteur.
Le mode `--cyd` reste un test hôte avec carte RAM et Mbed TLS natif ; il n'est
pas une mesure du SDK ou de la durée sur appareil.
`readAuroraWalletFileChecked` vérifie l'empreinte SHA-256 attendue avant
d'autoriser la lecture ; une substitution, même chiffrée avec le même mot de
passe, est refusée. `writeAuroraWalletFileVerified` relit et authentifie la
sauvegarde avant de rendre son empreinte publique. Aucune de ces API ne fournit
une clé de déchiffrement réutilisable à la session UI.

Le contrôle préalable SD est également testé : carte vide acceptée sans écriture,
absence de carte ou racine illisible refusées, montage/démontage équilibrés,
aucun fichier créé lors d'un export sans carte.

Ce n'est pas un test des contrôleurs SD physiques ni des accélérateurs cryptographiques ESP32. Les fixtures sont fictives, sans fonds ; aucune carte réelle n'est ouverte. Les échanges entre les deux lecteurs et l'autotest E00 restent à vérifier sur les appareils.

## Régressions d'effacement P4

```powershell
python tests/crypto/run_memory_hardening.py
```

Le test reconstruit dans `tmp/` le commit uBitcoin épinglé, applique V1 puis le
complément P4 et vérifie leur idempotence. Un patch incomplet est rejeté sans
modifier d'autres fichiers. Les sources SDK des overlays Mbed TLS sont également
validées par empreintes intégrales ; le SDK partagé n'est jamais modifié.

Les fonctions réelles de sérialisation privée sont testées sur succès, sortie
tronquée et échec Base58 simulé. Les cinq erreurs possibles de fin HMAC et les
contextes invalides doivent tous effacer leur tampon temporaire. Les algorithmes
Mbed TLS complets, dont PBKDF2/AES-GCM, restent vérifiés par `run.py`, qui compile
désormais les overlays P4 de `md.c` et `platform_util.c`.

Avec les vraies sources SHA/HMAC uBitcoin et MSVC `/O2`, des fibres isolées
permettent d'inspecter la pile **du seul processus de test** après retour :
le témoin non corrigé doit contenir les calendriers SHA-256/SHA-512 connus ; les
versions corrigées, déroulées ou non, ne doivent plus les contenir. Des vecteurs
SHA et HMAC publics vérifient que les résultats cryptographiques restent inchangés.

La dépendance P4 doit avoir été téléchargée par une configuration du projet.
Au besoin, `AURORA_TEST_UBITCOIN_LIB` désigne un autre répertoire `src` avec le
même commit disponible dans son dépôt Git ; il est utilisé **en lecture seule**.
Tous les essais et modifications de dépendances se font dans des copies `tmp/`.

Par exemple, si seul le profil P4 3.x a été compilé :

```powershell
$env:AURORA_TEST_UBITCOIN_LIB = (Resolve-Path 'targets/waveshare_p4/.pio/build/waveshare-p4/_deps/ubitcoin-src/src').Path
python tests/crypto/run_memory_hardening.py
```

Ces vérifications ne constituent pas une extraction physique de RAM P4, un test
de rémanence après coupure, ni une preuve couvrant tous les spills du compilateur.
Le binaire embarqué et les réinitialisations AES/SHA doivent aussi être validés
sur une carte d'essai avec des données publiques.
