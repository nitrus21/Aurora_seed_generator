# Tests de collecte d’entropie — AURORA 1.7.6

Sous Windows avec Visual Studio Build Tools (compilateur C++ et SDK Windows) :

```powershell
.\tests\native\run.cmd
```

Les tests exécutent le véritable `include/entropy.h` avec des entrées matérielles simulées et des fonctions SHA-256/HMAC fournies par Windows CNG. Les fixtures sont déterministes et ne doivent jamais servir à générer un portefeuille réel. Les exécutables restent dans `tmp/entropy-tests/`.

Vérifications : conservation des coordonnées, de la pression, des timings et du RNG ; contribution effective de la lumière ; exclusion mutuelle ADC/RNG ; seuil de 320 échantillons et refus de finaliser à 160 ; absence d’influence de l’aperçu sur le mélange ; effacement de sa clé ; annulation et recommencement.

La compilation du véritable firmware utilise ses dépendances épinglées :

```powershell
pio run -e esp32-2432S028R
```

Pour contrôler les binaires de la version finale inclus dans le dépôt, sans compiler ni flasher :

```powershell
.\tests\release\verify.ps1
```

Ce contrôle vérifie la cohérence des versions et du manifeste, les octets de chaque composant aux offsets prévus dans l’image fusionnée, toutes les empreintes SHA-256 et celles affichées dans le Web Flasher et le README.

## État de validation du 14 septembre 2026

- Tests natifs de collecte et contrôle des binaires : réussis.
- Écran LVGL avec matériel simulé : compteur 320, seuils de couleur, aperçu, pause verte, annulation et recommencement vérifiés.
- Carte ESP32-2432S028R du projet : binaires finaux 1.7.6 flashés, données écrites vérifiées, puis **E00 en 2 120 ms** au redémarrage, sans défaut de démarrage observé.

À vérifier manuellement avant tout usage réel : fluidité tactile, réaction de la photorésistance lorsqu’on la couvre puis l’éclaire, bandeau défilant, rouge avant 50 %, orange jusqu’à 99 %, vert pendant une seconde à 320 échantillons, retour pendant la collecte et pendant l’état vert, puis parcours complet d’un portefeuille de test sans fonds. Les tests natifs et E00 ne mesurent pas l’entropie physique et ne constituent pas un audit de sécurité.
