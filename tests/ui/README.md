# Tests d'interface — Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 et ESP32-2432S028R

Après une première configuration du projet P4 (pour télécharger LVGL 9.5.0), sous Windows avec Python 3 et les outils C++ Visual Studio :

```powershell
python tests/crypto/run.py
python tests/ui/run.py
python tests/ui/run.py --cyd
```

Le test compile les **vraies sources** de `src/ui.cpp`, de la compatibilité V2, de l’allocateur sécurisé, de l’écran portrait et de LVGL avec un matériel simulé. Il utilise les objets Mbed TLS du test crypto préalable. Le noyau portefeuille, la microSD et les capteurs sont remplacés par des fixtures publiques sans fonds. Ne pas lancer pendant un build P4 : un verrou empêche le remplacement des dépendances gérées pendant le test.

`run.py` est le lanceur des deux profils : P4 par défaut, ESP32-2432S028R avec `--cyd`.

CYD 1.9.9 : assertions sur les tailles de police de la 1.7.5 (configuration,
titres, saisies, mots, boutons, collecte), captures 320 × 240 et contrôle des
boutons d’information après génération comme après ouverture de fichier.
Les couleurs du clavier sont comparées au thème natif LVGL 8 de la 1.7.5,
y compris les états appuyé et sélectionné. Le thème sombre du P4 reste contrôlé.
Ces contrôles s’ajoutent aux tests de sécurité du parcours par mot de passe.

Les mocks microSD fournissent des fixtures publiques et une empreinte SHA-256,
jamais une clé de session. Le banc UI vérifie la saisie du mot de passe à chaque
consultation et les durées de vie ; le banc crypto séparé teste le vrai codec
AES-GCM/PBKDF2 et la liaison exacte au fichier.

- Captures 480 × 800 dans `tmp/ui-p4-native/`, y compris claviers, restauration, QR, collecte et blocage de l'arrêt des capteurs.
- Vérification du seuil 320, des limites du pavé tactile, de l'annulation et du redémarrage.
- Vérification qu'un événement LVGL tactile produit un seul échantillon, sans répéter un état mis en cache à chaque passage de la boucle.
- Vérification qu'un arrêt non confirmé, même après le délai d'attente, ne permet pas d'afficher les secrets.
- Confirmation exacte de passphrase et conservation du mélange jusqu'à la génération.
- Palette sombre commune à tous les claviers P4 ; les champs de saisie restent blancs.
- Champs de saisie P4 blancs de 60 px avec police 24 px, liste d'ouverture compacte, choix du nombre de mots en grille 3+2 et configuration agrandie.
- Douze mots par page et QR P4 de 336 px centrés, avec la valeur en police 30 px sous le code, y compris les QR xpub, clé privée et xprv Umbrel.
- Titres principaux P4 à 20 px et sous-titres de section à 18 px ; les tests empêchent l'agrandissement involontaire des consignes.
- Création/restauration initiale hors ligne ; contrôle microSD à la sauvegarde.
  Sur P4, la carte est aussi requise à l’ouverture et à chaque consultation privée.
  Retrait/erreur/annulation détruisent les saisies et annulent les opérations différées.
- P4 : logo centré sous le séparateur, quatre boutons à 80 % de la largeur avec une cinquième rangée réservée, et actions standards de 64 pixels de haut, notamment PRÉCÉDENT/SUIVANT ; chaque écran à en-tête est contrôlé pour empêcher les informations ou commandes de couper le séparateur. CYD : coordonnées, tailles et trois choix du menu original conservés. Captures `mode-portrait.png`, `mode-cyd.png` et `sd-required.png` dans les répertoires respectifs.
- P4 : ouverture V1/V2 sans PIN. Le modèle public ne contient ni mots, WIF,
  descripteur privé, passphrase, mot de passe, clé de fichier ni vérificateur PIN.
  Il ne conserve que les données publiques et l’empreinte du fichier.
- P4 : mot de passe par catégorie privée, contrôle de l’empreinte du fichier,
  retrait SD, mauvais mot de passe, incohérence après dérivation et erreurs de
  relecture ; aucune donnée privée n’est affichée après un échec.
- Retour/annulation/expiration effacent les secrets. Une navigation entre pages
  de mots ne renouvelle pas la limite. Les durées simulées de dérivation testent
  les échéances privées et l’inactivité, avant affichage ou écriture.
- Le banc commun de navigation vérifie sur les deux interfaces le retour des
  consultations au portefeuille public, sans mot de passe/clé/mots/passphrase
  conservés, l’identité du fichier inchangée et une nouvelle authentification.
  Il couvre aussi les QR publics, l’annulation d’authentification d’export vers
  les formats, les retours des formulaires par flèche et clavier, ainsi que le
  retour depuis les mots d’une restauration manuelle. Verrouillage, retour à
  l’échéance et inactivité ferment toujours entièrement la session.
- La validation du mot de passe d’export et de sa confirmation planifie directement
  l’écriture, sans écran PIN. Les nouveaux fichiers sont sans PIN (V1 exact).
  L’export Electrum explicitement autorisé reste disponible.
- AEZEED sans PIN : QR `xprv` avec décompte de 3 minutes. Le sous-type Umbrel
  `.aurora` conserve seulement la racine BIP32 et l’anniversaire LND ; mots et
  passphrase restent désactivés. Le mot de passe est redemandé pour le `xprv`
  et l’export Sparrow en clair, puis la copie privée est effacée au retour.
- Verrouillage/démarrage/nettoyage d’urgence : contrôle des tampons applicatifs et
  remplacement des trois images d’affichage simulées. La synchronisation réelle
  de l’écran nécessite toujours une recette matérielle.
- Vérification des allocations LVGL écrasées **avant** libération/réallocation, avec contrôle du cas d'échec de réallocation.
- P4 : délai d'inactivité dès l'entrée dans chaque parcours, saisies et première
  génération comprises ; fermeture prioritaire même si l'arrêt d'un capteur
  reste en attente. Démarrage et erreur de sécurité effacent les données nommées.

Les tests complémentaires sont séparés pour ne pas confondre leurs garanties :

```powershell
python tests/crypto/run_memory_hardening.py
python tests/lvgl/test_patch.py
powershell -ExecutionPolicy Bypass -File tests/lvgl/run.ps1
powershell -ExecutionPolicy Bypass -File tests/memory/run.ps1 -Optimized
python tests/storage/run.py
```

Ils contrôlent les temporaires crypto, les erreurs d'allocation, le balayage des
blocs possédés au démarrage, les demandes de synchronisation cache et les tampons
stdio. Aucun de ces bancs natifs ne garantit un effacement physique lors d'une
coupure : les tâches SDK actives, caches et périphériques réels nécessitent des
essais distincts sur carte avec des données publiques.

Ces tests n'attestent ni les pilotes physiques, ni le caractère imprévisible des capteurs, ni la compatibilité électrique d'une caméra. La recette sur les deux appareils reste obligatoire. Pour la génération d'entropie et le format binaire figé, voir également `tests/native/run.cmd`.

Le profil `--cyd` compile LVGL **8.4.0** installé par le build CYD et les mêmes
sources UI/allocateur et le profil **CYD 1.9.9**. Il capture les formulaires en **320 × 240**
dans `tmp/ui-cyd-native/` et vérifie les événements réels de validation du clavier
ainsi que les consultations par mot de passe, les expirations de 15/60/120/180 secondes,
les calculs trop longs, la substitution du fichier, le retrait SD et le nettoyage
terminal des allocations possédées. Les tests de vérificateurs PIN restent des
tests de compatibilité du codec V2, pas du parcours actuel. Le profil P4 reste le test de parcours
complet avec import/export simulés. Ne pas compiler le CYD pendant son test UI.
