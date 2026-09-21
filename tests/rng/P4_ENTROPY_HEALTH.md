# Protocole F51-05 — santé de l'entropie P4 rev1.3

## Objet et limites

Le diagnostic temporaire `waveshare-p4-rev1-entropy-health` exerce la même
activation `bootloader_random_enable()` que la collecte AURORA puis lit la
sortie conditionnée `esp_random()`. Sur le profil rev1, ESP-IDF active le bruit
interne du SAR ADC1, canal 10, atténuation 12 dB. La sortie analogique brute
n'est pas exposée par l'API supportée : ce protocole ne prétend donc ni la
capturer, ni estimer sa min-entropie, ni constituer une certification NIST
SP 800-90B.

Le conditionnement final AURORA par SHA-256 reste couvert séparément par les
tests déterministes de sérialisation, séparation de domaines, déduplication et
effacement. Tester l'apparence aléatoire d'un digest SHA-256 ne qualifierait pas
la source physique et n'est volontairement pas présenté comme une preuve.

## Invariants de confidentialité

- retirer toute microSD avant le diagnostic ;
- aucun portefeuille, seed ou capteur audio/caméra n'est démarré ;
- aucune valeur RNG brute, aucun digest et aucun fichier n'est affiché, écrit ou
  transmis ; seuls des agrégats et PASS/ÉCHEC apparaissent à l'écran ;
- aucun journal applicatif/SDK/bootloader n'est activé ; la bannière ROM
  transitoire reste possible et ne peut être supprimée sans eFuse irréversible ;
- aucune eFuse et aucune autre partition que l'application ne doit être écrite.

## Contrôles et témoins

Chaque série porte sur 500 000 mots de 32 bits. Elle exige : 49–51 % de bits à
1 au total et à chaque position, 49–51 % de transitions, aucune égalité entre
deux mots consécutifs et aucune série identique supérieure à 64 bits. Trois
séries sont exécutées avec désactivation/réactivation de la source entre elles.

Le test hôte `python tests/rng/run_entropy_health.py` démontre que ces contrôles
refusent les témoins bloqué à zéro, alternant, compteur et biaisé. Le témoin
déterministe xorshift n'est pas une source sûre ; il sert seulement à vérifier
que l'implémentation des contrôles accepte un flux blanc reproductible.

## Campagne physique autorisée séparément

1. Identifier le port et la révision sans écriture, sauvegarder puis hacher la
   partition application complète.
2. Avec autorisation explicite, écrire uniquement l'application diagnostique
   rev1 et vérifier l'écriture. Ne jamais utiliser `erase-all`.
3. Sans microSD, effectuer trois démarrages à froid. Pour chaque démarrage,
   noter uniquement le verdict final et les cinq agrégats conservés à l'écran.
   Le verdict final cumule les trois séries internes ; afin de ne rien persister
   ni transmettre, l'écran final ne conserve que les agrégats de la troisième
   série. Les deux écrans intermédiaires restent visibles 1,5 seconde chacun.
4. En cas d'ÉCHEC, conserver l'appareil hors usage secret ; ne jamais exporter
   le flux brut pour contourner le diagnostic.
5. Restaurer exactement la partition application sauvegardée, vérifier son
   SHA-256 sur toute sa taille et confirmer visuellement le firmware normal.

Un PASS indique uniquement l'absence des pannes franches visées dans les
conditions testées. Il ne prouve ni indépendance, ni imprédictibilité, ni niveau
d'entropie minimal et ne qualifie pas une autre révision de silicium.
