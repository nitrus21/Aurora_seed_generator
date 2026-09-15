# Audit de régression ESP32-2432S028R 1.9.3

Après compilation du CYD et installation des dépendances natives indiquées dans
`tests/crypto/README.md` et `tests/ui/README.md` :

```powershell
python tests/cyd_security/run.py
```

Ce banc ne flashe rien. Les preuves et copies de binaires restent dans `tmp/`.
Il vérifie le binaire Xtensa lié (absence de sauvegarde de crash et de session
PIN), les appels d’effacement SHA via les pools de constantes ELF, la table de
partitions, les erreurs de démarrage et de synchronisation, les durées de vie
de l’interface CYD et la non-régression de l’interface P4.

Les sondes SHA sont compilées en `/O2` depuis la bibliothèque du build CYD.
Les essais de stockage exécutent les méthodes de production avec des flux CRT
et des erreurs injectées. Le contrôle du démarrage compile la fonction réelle
avec une partition et un tas simulés. Les seeds/marqueurs sont exclusivement
des données publiques de test ; aucun appareil ni fichier utilisateur n’est lu.

Le codec natif utilise Mbed TLS portable 3.6.6, alors que le firmware CYD utilise
le SDK Arduino et Mbed TLS 2.28.7. Ces tests ne remplacent donc ni l’autotest E00
sur le CYD, ni une recette microSD réelle, ni des essais de panique/coupure avec
inspection matérielle. Ils ne constituent pas une certification de sécurité.
