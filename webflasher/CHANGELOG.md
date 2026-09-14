# Notes de version AURORA

## 1.7.6 — 14 septembre 2026

- Ajout de la luminosité mesurée sur GPIO34 au mélange aléatoire, en conservant les coordonnées, la pression, les timings et le RNG matériel.
- Alternance des lectures de luminosité et de la source interne du RNG pour éviter leur utilisation simultanée du SAR-ADC.
- Collecte portée de 160 à 320 échantillons, sans imposer de durée minimale ni modifier le choix du nombre de mots.
- Aperçu hexadécimal défilant, séparé du secret final par un HMAC avec une clé d’affichage indépendante et temporaire.
- Jauge rouge avant 50 %, orange de 50 à 99 %, verte à 100 %, avec affichage du compteur et maintien du vert pendant une seconde.
- Effacement de l’aperçu et de sa clé en fin de collecte ou lors d’une annulation.
- Tests natifs couvrant les sources du mélange, le nouveau seuil, l’exclusion ADC/RNG, l’aperçu et l’annulation.
- Binaires applicatif et fusionné du Web Flasher, manifeste et empreintes SHA-256 alignés sur la version 1.7.6.

Les formats BIP39, les choix de 12 à 24 mots, les dérivations Bitcoin, les passphrases et les exports sont inchangés. Le compteur indique une progression de collecte, pas une mesure d’entropie certifiée ; 320 échantillons ne signifient pas 320 bits de sécurité.

Validation : tests natifs de collecte, tests de l’écran LVGL avec matériel simulé, compilation ESP32 et contrôle des binaires réussis. Le 14 septembre 2026, les binaires finaux 1.7.6 ont été flashés sur l’ESP32-2432S028R du projet avec vérification des données écrites ; l’autotest a renvoyé **E00 en 2 120 ms** au redémarrage, sans défaut de démarrage observé. Le contrôle manuel des gestes, de la luminosité et du parcours complet reste à réaliser. Le projet reste expérimental et non audité.

## 1.7.5

Version précédente du firmware et première version incluse dans le Web Flasher français. Son image fusionnée est conservée dans `firmware/` pour référence.
