# Vérification native de l'interface P4

Après une première configuration du projet P4 (pour télécharger LVGL 9.5.0), sous Windows avec Python 3 et les outils C++ Visual Studio :

```powershell
python tests/crypto/run.py
python tests/ui/run.py
python tests/ui/run.py --cyd
```

Le test compile les **vraies sources** de `src/ui.cpp`, du PIN, de l'allocateur sécurisé, de l'écran portrait et de LVGL avec un matériel simulé. Il utilise les objets Mbed TLS du test crypto préalable. Le noyau portefeuille, la microSD et les capteurs sont remplacés par des fixtures ; aucune clé utilisable ni capture réelle n'est produite. Ne pas lancer pendant un build P4 : un verrou empêche le remplacement des dépendances gérées pendant le test.

- Captures 480 × 800 dans `tmp/ui-p4-native/`, y compris claviers, restauration, QR, collecte et blocage de l'arrêt des capteurs.
- Vérification du seuil 320, des limites du pavé tactile, de l'annulation et du redémarrage.
- Vérification qu'un événement LVGL tactile produit un seul échantillon, sans répéter un état mis en cache à chaque passage de la boucle.
- Vérification qu'un arrêt non confirmé, même après le délai d'attente, ne permet pas d'afficher les secrets.
- Confirmation exacte de passphrase et conservation du mélange jusqu'à la génération.
- SD obligatoire avant les formulaires, écran sans clavier/champs, RÉESSAYER sans carte et après réinsertion, FERMER avec effacement ; retrait avant validation de phrase, mot de passe et PIN. Conservation de l'entropie collectée et des erreurs PIN, abandon des saisies et absence de contrôle SD pendant l'arrêt des capteurs.
- Autorisation PIN par catégorie, compteur persistant, trois échecs avec effacement, délais 15/120 secondes, import V1/V2 et garde au point d'entrée de l'export.
- Vérification des allocations LVGL écrasées **avant** libération/réallocation, avec contrôle du cas d'échec de réallocation.

Ces tests n'attestent ni les pilotes physiques, ni le caractère imprévisible des capteurs, ni la compatibilité électrique d'une caméra. La recette sur les deux appareils reste obligatoire. Pour la génération d'entropie et le format binaire figé, voir également `tests/native/run.cmd`.

Le profil `--cyd` compile LVGL **8.4.0** installé par le build CYD et les mêmes
sources UI/PIN/allocateur. Il capture les nouveaux formulaires en **320 × 240**
dans `tmp/ui-cyd-native/` et vérifie les événements réels de validation du clavier
ainsi que la fermeture après trois erreurs. Le profil P4 reste le test de parcours
complet avec import/export simulés. Ne pas compiler le CYD pendant son test UI.
