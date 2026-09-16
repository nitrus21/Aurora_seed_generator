# Suivi des dépendances

Ce guide décrit les dépendances du projet et la procédure de revue avant une
publication. Il ne constitue pas un rapport de vulnérabilités ni une déclaration
d'absence de CVE. Les preuves et analyses détaillées restent locales sous `tmp/`.

## Sources de référence

| Couche | Référence actuelle | Où la vérifier |
| --- | --- | --- |
| Plateforme P4 | pioarduino `55.03.311`, archive de version | `targets/waveshare_p4/platformio.ini` |
| ESP-IDF P4 | `5.5.5` résolu ; manifeste compatible `>=5.5,<5.6` | `targets/waveshare_p4/dependencies.lock`, journal de build et SDK installé |
| Mbed TLS P4 | `3.6.6`, intégré au SDK avec overlays locaux | `mbedtls/build_info.h` du SDK, `tools/prepare_p4_crypto_overlay.py` |
| LVGL P4 / BSP Waveshare | `9.5.0` / `1.0.1` | `targets/waveshare_p4/main/idf_component.yml` et verrou |
| Vidéo P4 | contrainte `~2.0`, résolution `2.0.1` | manifeste et verrou P4 |
| Autres composants P4 | 24 entrées résolues au total, IDF compris | `targets/waveshare_p4/dependencies.lock` |
| uBitcoin / trezor-crypto incorporé | commit `877542fdc16319dd92a7d2a679ea9dacce474bd2` | `platformio.ini`, CMake P4, sources incorporées et scripts de patch |
| Plateforme CYD | Espressif32 `6.9.0`, Arduino | `platformio.ini` et métadonnées des paquets installés |
| Interface CYD | LVGL `8.4.0`, TFT_eSPI `2.5.43`, XPT2046 au commit épinglé | `platformio.ini` |
| Web Flasher | `esp-web-tools@10.4.0` depuis un CDN | `webflasher/index.html` ; dépendance web distincte du firmware |
| Publication GitHub | actions référencées par commits complets | `.github/workflows/deploy-webflasher-pages.yml` |

Le verrou IDF ne couvre pas à lui seul uBitcoin, les bibliothèques intégrées au
SDK, la chaîne de compilation, les outils Python, le navigateur ou les modules
web transitifs. Le firmware du coprocesseur C6 n'est pas construit par ce projet ;
l'application P4 maintient ce coprocesseur en reset selon le profil matériel.

## Épinglage et correctifs locaux

Le KDF des fichiers utilise l'API incrémentale PBKDF2-HMAC-SHA-256 du commit
uBitcoin ci-dessus. Ses sources et en-têtes, la préparation HMAC et SHA-2 sont
vérifiés par empreinte dans `tools/patch_ubitcoin_p4.py`. Les adaptations locales
conservent les vecteurs : temporaires HMAC locaux, effacement SHA-256 regroupé,
état KDF effacé avant retour et pauses RTOS entre blocs. La compilation P4
optimise explicitement SHA-2 et PBKDF2 en `-O2`, avec les boucles SHA-2 déroulées,
sans changer le SDK global. Son adaptateur `main/kdf_sha256.c` conserve la
convention de première itération et la boucle PBKDF2 épinglées, mais utilise
l'accélérateur SHA pendant chaque bloc de calcul. Les sources HAL SHA et
l'interface du pilote sont aussi vérifiées par empreinte dans
`tools/prepare_p4_crypto_overlay.py`. Tester les limites de blocs sur le P4
avec les vecteurs publics de `tests/crypto/p4_kdf_vectors.h` ; les tests hôte
du KDF logiciel ne valident pas cet adaptateur matériel.

Conserver le manifeste et `dependencies.lock` ensemble dans Git. Une plage de
versions dans le manifeste n'est pas la version résolue. Examiner chaque diff
du verrou ; ne pas le supprimer pour faire disparaître une erreur. Le format du
verrou et son intérêt pour la reproductibilité sont décrits par
[Espressif](https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/dependencies_lock.html).

Les scripts `patch_ubitcoin.py`, `patch_ubitcoin_p4.py`, `patch_ubitcoin_rng.py`,
`patch_lvgl.py`, `harden_cyd.py` et `prepare_p4_crypto_overlay.py` participent au
build. Conserver leurs versions avec celles des dépendances. Les contrôles de
motifs ou d'empreintes attendues ne doivent pas être désactivés pour accepter une
mise à jour. Un numéro de version amont ne décrit pas entièrement une bibliothèque
modifiée par ces correctifs.

## Revue avant publication

1. Relever les versions résolues et installées, la révision du code, les options
   SDK, la chaîne de compilation, les correctifs locaux et les SHA-256 des
   bootloaders, applications et images factory pour chaque profil silicium.
2. Consulter les avis des fournisseurs : [ESP-IDF](https://github.com/espressif/esp-idf/security/advisories),
   [tableau de suivi Espressif](https://espressif.github.io/esp-idf-security-dashboard/),
   [Mbed TLS](https://mbed-tls.readthedocs.io/en/latest/security-advisories/),
   ainsi que les projets d'origine des composants graphiques, vidéo et crypto.
3. Pour chaque avis pertinent, consigner en privé la version affectée, les
   préconditions, les options activées, les API utilisées et le code réellement
   lié. Distinguer **affecté**, **non applicable avec preuve** et **à examiner**.
   L'absence de réseau ne permet pas d'écarter une faille de traitement de
   fichiers, de mémoire ou de canal auxiliaire.
4. Pour une mise à jour, revoir les patches avant de modifier les empreintes,
   puis tester vecteurs crypto, compatibilité de fichiers, erreurs de stockage,
   nettoyage, interface et les deux profils P4. Conserver distincts les résultats
   sur ordinateur et les essais sur carte.
5. Archiver l'inventaire et la décision de publication. Un composant installé
   peut être éliminé au lien : une simple recherche dans le fichier `.map`, qui
   inclut aussi des sections rejetées, ne prouve pas sa présence dans l'image.

Le workflow GitHub actuel publie et vérifie les artefacts du Web Flasher ; il ne
met pas en place une veille automatique complète des dépendances firmware.
L'option de build `IDF_COMPONENT_CHECK_NEW_VERSION=0` supprime les recherches de
nouvelles versions pendant la compilation, pas le besoin d'une revue de sécurité.

## Limites à conserver dans toute documentation de version

- Le KDF du fichier `.aurora` ne protège ni contre un firmware remplacé ni contre
  la rémanence physique pendant/après une coupure.
- Une coupure brutale n'exécute aucun nettoyage ; un test de reset ne la remplace pas.
- L'export Electrum privé demeure volontairement **en clair sur microSD**. Aucun
  KDF `.aurora` ne le protège, et verrouiller l'appareil ne supprime pas ce fichier.
- Épinglage, SHA-256 et tests réussis ne sont pas une certification de sécurité.
