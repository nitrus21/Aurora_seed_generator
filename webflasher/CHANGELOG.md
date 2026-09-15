# Notes de version AURORA

## 2.0.0 — P4 uniquement, en développement

- Fin du développement CYD, figé en version 1.9.2.
- Nouvelle identité de version réservée aux profils Waveshare ESP32-P4.
- Les binaires Web Flasher P4 restent en 1.9.2 jusqu'à la validation et la publication des deux révisions matérielles 2.0.0.

## 1.9.2 — CYD et P4 finaux, 15 septembre 2026

- Conservation des trois actions historiques du CYD, sans le déchiffrement AEZEED réservé au P4.
- Création et restauration possibles sans microSD ; la carte est exigée uniquement à l'ouverture ou à la sauvegarde d'un fichier.
- Confirmation de passphrase, PIN de session, PIN par sauvegarde V2 et temporisation des écrans privés.
- Image fusionnée CYD ajoutée au Web Flasher avec maintien du choix historique 1.7.5.
- Ajout des images P4 1.9.2 finales, séparées pour les révisions silicium 1.x et 3.x.
- Claviers et champs de saisie assombris sur CYD et P4 pour réduire l'éblouissement, avec contraste conservé sur les touches de fonction.
- Champs blancs rehaussés avec texte agrandi, sélection de seed 3+2, douze mots par page et QR 70 % de largeur avec valeur agrandie sous le code.
- Hiérarchie typographique P4 uniformisée : titres principaux à 20 px, sous-titres de section à 18 px et consignes inchangées.
- Épinglage exact d'ESP Web Tools 10.4.0, CSP restrictive et GitHub Actions verrouillées par SHA.

Validation CYD : compilation finale réussie ; la compilation matérielle précédente a été vérifiée sur ESP32-D0WD-V3 révision 3.1 avec autotest **E00 en 2 145 ms**, mais le CYD n'était plus connecté pour reflasher le thème sombre. Validation P4 1.x : écriture de l'application finale sur ESP32-P4 révision 1.3, vérification des données, version 1.9.2 et autotest **E00 en 1 244 ms**, sans panic ni redémarrage pendant 35 secondes. Les installations depuis le navigateur et le profil P4 3.x doivent encore être vérifiés sur le matériel correspondant.

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
