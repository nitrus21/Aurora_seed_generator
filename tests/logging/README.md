# Journaux applicatifs

`python tests/logging/run.py` (MSVC sous Windows) vérifie le mode production
par défaut et explicite, le mode diagnostic, le refus d'une valeur invalide,
l'absence d'évaluation des arguments et de chaînes de diagnostic dans le
binaire de test. Il contrôle les appels des sources applicatives et le maintien
des autotests. Les suites UI vérifient séparément les parcours et erreurs.
Après compilation des trois profils, `python tests/logging/verify_images.py`
vérifie l'absence des chaînes de diagnostic dans les images locales et produit
une preuve destinée au dépôt privé de développement. Il ne remplace pas les
binaires publiés.
Une campagne limitée au P4 peut employer
`python tests/logging/verify_images.py --target p4-rev1 --target p4-rev3` ; le
mode sans argument continue d'exiger les trois profils.

`include/aurora_log.h` désactive les huit messages applicatifs de démarrage,
codes d'autotest et durées lorsque `AURORA_DEBUG` est absent ou égal à `0`.
Un build local `-D AURORA_DEBUG=1` permet ces seuls diagnostics non sensibles ;
il ne doit jamais être distribué comme firmware de production. Aucun secret,
nom de fichier saisi, ni dump mémoire ne doit être ajouté à ces messages.

Les niveaux `NONE` ESP-IDF/bootloader ont été introduits dans la 2.0.10 interne
et sont conservés en 2.0.11 ; ils ne sont **pas présents dans la 2.0.9 publiée**. Une future distribution exige
sa validation matérielle. Les autotests et erreurs visibles à l'écran restent actifs.

La capture physique du 20/09/2026 a confirmé que les sources 2.0.9 conservaient
les niveaux SDK/bootloader INFO. Depuis la 2.0.10 interne, le développement compile
les logs ESP-IDF et bootloader à `NONE`, en plus de `AURORA_DEBUG=0`; les panics
restent silencieux. Le profil KDF isolé garde volontairement ses mesures
publiques explicites et ne constitue jamais une image de production.

La bannière ROM précède le firmware. La supprimer exige des eFuses irréversibles,
hors périmètre. Aucun de ces réglages ne bloque USB/UART, JTAG ou le chargeur ROM.
Sur CYD, `CORE_DEBUG_LEVEL=0` et `LV_USE_LOG=0` restent inchangés ; son framework
est partiellement précompilé. Un test sur ordinateur ne remplace pas une nouvelle
capture physique de la future image 2.0.11.
