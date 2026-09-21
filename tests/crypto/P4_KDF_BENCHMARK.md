# Benchmark KDF P4 — protocole F51-07

Ce diagnostic mesure le vrai `auroraWalletKdf` P4 et son accélérateur SHA avec
des constantes publiques. Il ne lance pas l'interface, ne monte pas la microSD,
ne génère aucun portefeuille et ne touche aucun eFuse. Il est réservé au P4
silicium 1.x ; le seul appareil disponible est une révision 1.3.

## Construction sans accès matériel

```powershell
python tests/crypto/verify_p4_benchmark.py
pio run --project-dir targets/waveshare_p4 -e waveshare-p4-rev1-kdf-benchmark
```

L'environnement doit produire une application marquée par
`AURORA_P4_KDF_BENCHMARK`. Il ne doit jamais être copié dans `releases/`, dans le
Web Flasher ou dans une release GitHub.

## Essai physique, uniquement après autorisation explicite

1. Vérifier que le P4 connecté est bien la révision 1.x de test et archiver
   l'image utilisateur actuelle si elle n'est pas déjà reproductible.
2. Flasher uniquement l'application diagnostique, sans `erase-all` et sans
   opération eFuse. Capturer la sortie série jusqu'à `done ok=1`.
3. Exiger trois échantillons valides pour 10 000, 120 000, 300 000 et 500 000
   itérations. Un seul `ok=0`, reset, watchdog ou valeur manquante invalide la
   campagne.
4. Consigner modèle, révision silicium, alimentation, build exact, température
   approximative, valeurs brutes et médianes. Les temps sont ceux d'une seule
   dérivation, pas d'une écriture/relecture `.aurora` complète.
5. Restaurer l'image utilisateur P4 voulue et contrôler son démarrage. Aucune
   sauvegarde personnelle ne doit être utilisée pendant la campagne.

Le diagnostic valide chaque résultat avec des vecteurs PBKDF2-HMAC-SHA-256
calculés indépendamment par Python. Il ne mesure pas la vitesse d'un attaquant,
ne prouve pas l'absence de canal auxiliaire et ne remplace pas un essai complet
de sauvegarde puis relecture sur microSD.
