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

`include/aurora_log.h` désactive les huit messages applicatifs de démarrage,
codes d'autotest et durées lorsque `AURORA_DEBUG` est absent ou égal à `0`.
Un build local `-D AURORA_DEBUG=1` permet ces seuls diagnostics non sensibles ;
il ne doit jamais être distribué comme firmware de production. Aucun secret,
nom de fichier saisi, ni dump mémoire ne doit être ajouté à ces messages.

Ce changement de sources n'est **pas présent dans les binaires déjà publiés**
1.9.9 / 2.0.3. Une prochaine distribution nécessite un numéro distinct et une
validation matérielle. Les autotests et erreurs visibles à l'écran restent actifs.

Les messages ESP-IDF, BSP, bootloader et ROM sont distincts : le P4 conserve
actuellement ses niveaux SDK/bootloader INFO ; sur CYD `CORE_DEBUG_LEVEL=0` et
`LV_USE_LOG=0` restent inchangés. Le framework CYD est partiellement précompilé.
Ni ce flag ni l'absence d'initialisation Serial par l'application ne bloquent
USB/UART, JTAG ou le chargeur ROM. Les panics silencieux existants ne sont pas
modifiés. Une capture physique boot/erreurs/consultations reste nécessaire ;
un test sur ordinateur n'atteste pas le silence de l'appareil.
