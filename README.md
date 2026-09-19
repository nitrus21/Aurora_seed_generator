<div align="center">

# AURORA

### Vos premiers mots Bitcoin. Hors ligne, entre vos mains.

<img src="assets/splash_320x240.png" alt="Une montagne sous le Bitcoin lumineux d’AURORA" width="480" />

**Créer · Restaurer · Vérifier · Sauvegarder**

Un écran tactile, vos gestes, vos sauvegardes. AURORA transforme une carte
compatible en générateur et lecteur de portefeuilles Bitcoin autonome,
avec une interface en français et sans service distant.

[**Installer AURORA**](https://nitrus21.github.io/Aurora_seed_generator/) ·
[Télécharger](https://github.com/nitrus21/Aurora_seed_generator/releases) ·
[Le parcours](#votre-premier-portefeuille-en-6-étapes)

</div>

---

## Pourquoi AURORA ?

**L’essentiel, directement sur l’appareil.** Créez une phrase de récupération,
vérifiez votre copie, consultez vos informations publiques et conservez une
sauvegarde chiffrée sur microSD. Pas de compte à ouvrir, pas de cloud à configurer.

| Ce que vous recherchez | Ce qu’AURORA vous apporte |
| --- | --- |
| Une utilisation autonome | Génération et consultation hors ligne ; aucun réseau utilisé par l’application. |
| Des standards connus | BIP39, BIP32 et adresses Legacy, Nested SegWit, Native SegWit ou Taproot. |
| Un parcours guidé | Écran tactile, consignes en français, suggestions de mots et vérification de la copie. |
| Une sauvegarde transportable | Fichier `.aurora` chiffré et authentifié ; les sauvegardes BIP39 restent lisibles par les deux modèles. |
| Une séparation public / privé | Informations publiques après ouverture ; mot de passe redemandé pour chaque consultation privée. |
| Un projet transparent | Sources sous licence MIT, dépendances épinglées et tests fournis avec le code. |

> [!IMPORTANT]
> AURORA est un **projet expérimental**, pas un portefeuille matériel certifié.
> Il ne signe pas de transactions et ne consulte pas votre solde. Commencez avec
> un portefeuille de test sans fonds ; vérifiez les adresses avant toute utilisation réelle.

## Deux cartes, une même philosophie

| Carte exacte | Expérience | Version à installer |
| --- | --- | --- |
| **ESP32-2432S028R — Cheap Yellow Display (CYD)** | Tactile résistif 2,8″, 320 × 240 ; interface compacte et clavier classique. | **1.9.9**, version finale ; **1.7.5** conservée. |
| **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3** | Tactile capacitif 4,3″, 480 × 800 portrait ; 12 mots par page, grands QR et clavier sombre. | **2.0.9**, image adaptée au silicium **1.x** ou **3.x**. |

La **1.9.9 clôt la branche ESP32-2432S028R**. Depuis la version 2.0.0, les nouvelles
fonctionnalités se développent sur la **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3**.
Les mentions CYD et P4 ci-dessous désignent ces deux références.
Le modèle Waveshare avec suffixe `-C` n’est pas la cible de ces images.

[Guide ESP32-2432S028R](targets/cyd/README.md) ·
[Guide Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3](targets/waveshare_p4/README.md) ·
[Différences entre versions](webflasher/CHANGELOG.md) ·
[Vérifier une release signée](RELEASE_SIGNATURES.md)

## Prêt à commencer

Il vous faut une carte compatible, un câble USB de données pour l’installation,
une alimentation autonome pour l’utilisation et une microSD FAT32 pour les fichiers.

1. Ouvrez le [Web Flasher officiel](https://nitrus21.github.io/Aurora_seed_generator/)
   dans Chrome ou Edge sur ordinateur.
2. Choisissez **la référence exacte**, la version et la révision silicium du P4.
   Ne choisissez pas la révision au hasard : les images 1.x et 3.x sont différentes.
3. Branchez la carte, sélectionnez son port et attendez la fin de l’installation.
4. Vérifiez le démarrage, puis **déconnectez les données USB**. Utilisez une
   alimentation autonome, idéalement avec un câble sans lignes de données.

Le site ne demande **jamais de seed, de clé privée ni de mot de passe**.
Une installation complète peut effacer la flash ; sauvegardez votre travail avant de flasher.

## Votre premier portefeuille en 6 étapes

### 01 — Choisissez votre portefeuille

Appuyez sur **NOUVEAU PORTEFEUILLE**. Choisissez 12, 15, 18, 21 ou 24 mots,
puis le type d’adresse Bitcoin. Les mots utilisent la liste anglaise officielle BIP39.

### 02 — Participez à la collecte

Déplacez votre doigt sur la zone tactile jusqu’à la fin de la jauge.
AURORA mélange le RNG matériel, 320 échantillons tactiles et les sources
disponibles : lumière sur CYD ; microphones et caméra OV5647 facultative sur P4.
Les capteurs complètent le RNG, ils ne le remplacent pas. La jauge indique
l’avancement, pas un nombre certifié de bits d’entropie.

### 03 — Ajoutez une passphrase, si vous le souhaitez

Laissez les deux champs vides, ou saisissez deux fois la même passphrase BIP39
(jusqu’à 63 caractères ASCII imprimables).

**La passphrase n’est pas le mot de passe du fichier.** Elle change le portefeuille
dérivé de vos mots : la perdre revient à perdre l’accès à ce portefeuille.

### 04 — Notez et vérifiez vos mots

Recopiez les mots dans leur ordre sur un support durable, à l’abri des regards.
AURORA demande ensuite trois mots pour vérifier votre copie.
Ne photographiez pas la phrase et ne la saisissez pas sur un site web.

### 05 — Retrouvez vos informations publiques

Consultez la première adresse, le chemin de dérivation et la clé publique étendue
du compte, en texte ou en QR. Ces informations peuvent servir à préparer un
portefeuille d’observation dans un logiciel compatible, sans lui donner vos mots.
Une clé publique étendue reste sensible pour votre vie privée : elle permet
d’observer les adresses de son compte.

### 06 — Sauvegardez, puis verrouillez

Insérez une microSD FAT32 et choisissez **Aurora Wallet**. Nommez le fichier
et choisissez un mot de passe long, unique et imprévisible. Le fichier `.aurora`
est chiffré avec AES-256-GCM. Le CYD 1.9.9 utilise **120 000 itérations
PBKDF2-HMAC-SHA-256** pour les nouvelles sauvegardes ; le P4 2.0.9 en utilise
**500 000**. Choisissez toujours un mot de passe long, unique et imprévisible.

Vérifiez l’ouverture de votre sauvegarde avant de compter sur elle. Conservez
aussi une copie durable des mots et de votre éventuelle passphrase.
**VERROUILLER** ferme la session et nettoie les tampons sensibles pris en charge.

## Vous avez déjà un portefeuille ?

### Restaurer une phrase BIP39

Choisissez **RESTAURER UNE SEED**, le nombre de mots, puis saisissez votre phrase.
L’autocomplétion aide à retrouver les mots ; le checksum est contrôlé avant la
dérivation. Utilisez la même passphrase et le même type d’adresse qu’à l’origine.
Création et restauration initiales fonctionnent sans microSD ; elle devient
nécessaire pour sauvegarder ou exporter.

### Ouvrir un fichier Aurora Wallet

Placez le fichier `.aurora` à la racine de la microSD, choisissez
**OUVRIR AURORA WALLET**, puis entrez son mot de passe.

L’écran du portefeuille présente les informations publiques. Pour consulter
les mots, la passphrase, la clé privée ou préparer un export, AURORA redemande
le mot de passe et relit le même fichier. Il ne conserve pas le mot de passe
ou la clé de déchiffrement entre ces consultations. Il n’y a pas de PIN.

| Consultation du fichier après authentification | Durée |
| --- | --- |
| Mots de récupération | **3 minutes**, décompte commun à toutes les pages. |
| Clé privée | **1 minute**, hors déchiffrement. |
| Clé maître privée Umbrel/LND | **3 minutes**, hors déchiffrement. |
| Passphrase | **15 secondes**, traitement compris. |
| Préparation d’export | **120 secondes**, avec contrôle avant écriture. |

Sur le CYD 1.9.9, **RETOUR** efface les secrets consultés et revient aux boutons
du portefeuille. Seules les informations publiques et l’identité du fichier
restent disponibles ; la prochaine consultation privée redemande le mot de passe.
**VERROUILLER** ou la fin du décompte des mots/de la clé privée ferme toute la session.
Changer de page ne prolonge pas la durée. Ces décomptes sont absents
de la création/restauration manuelle : les autres parcours du portefeuille
conservent **2 minutes d’inactivité**. L’accueil n’est pas concerné.
Verrouiller ne supprime pas les fichiers microSD.

### Récupération Umbrel / LND — sur P4

**RÉCUPÉRER UMBREL / LND** lit les 24 mots **AEZEED** et leur passphrase éventuelle,
puis produit une clé maître privée **BIP32 `xprv`** utilisable dans Sparrow.
Le QR et le texte restent affichés au maximum **3 minutes**, avec décompte.

La 2.0.9 peut enregistrer cette récupération dans un fichier `.aurora` protégé
par mot de passe. Le fichier conserve uniquement la racine BIP32 (`xpub` et
`xprv`) et l’anniversaire LND : **les mots AEZEED et leur passphrase ne sont pas
enregistrés**. Après ouverture, le `xpub` est public ; chaque affichage du `xprv`
redemande le mot de passe. Les boutons MOTS et PASSPHRASE restent visibles mais grisés.

Le bouton rouge **EXPORTER POUR SPARROW** écrit le `xprv` maître dans un fichier
texte non chiffré sur la microSD. Il doit être protégé comme une clé privée.

Ce n’est **pas une conversion vers une phrase BIP39**, et cela ne restaure pas
les canaux Lightning. Importer une clé privée sur un ordinateur l’expose à cet
ordinateur : travaillez dans un environnement de confiance.

## Des formats ouverts, des choix explicites

| Format / fonction | Usage |
| --- | --- |
| BIP39 : 12 / 15 / 18 / 21 / 24 mots | Création ou restauration d’une phrase anglaise standard. |
| BIP44 / 49 / 84 / 86 | Legacy / Nested SegWit / Native SegWit / Taproot sur Bitcoin Mainnet. |
| Adresse et clé publique étendue | Texte et QR ; compte public `xpub`, `ypub` ou `zpub`. |
| `.aurora` | Sauvegarde privée chiffrée ; lecture V1/V2, nouvelles écritures V1. |
| Export Electrum privé | Fichier contenant une clé privée étendue **en clair** sur microSD. |
| Export Sparrow Umbrel | Fichier texte contenant le `xprv` maître **en clair** sur microSD. |

Les anciennes sauvegardes restent lisibles et ne sont pas modifiées par une
mise à jour. Pour bénéficier du nouveau coût de chiffrement, réexportez sous
un nouveau nom et vérifiez l’ouverture. Une ancienne copie garde son ancien
niveau de protection. Un mot de passe faible reste devinable malgré le KDF.

## La sécurité, sans promesse impossible

> [!WARNING]
> **Les exports Electrum et Sparrow ne sont pas chiffrés.** Toute personne disposant de ces fichiers
> peut accéder aux clés correspondantes. Le mot de passe `.aurora` ne les protège pas ;
> verrouiller ou redémarrer ne les efface pas de la microSD.

AURORA nettoie les tampons sensibles pris en charge à la fermeture, à l’expiration,
sur les erreurs prévues et au démarrage. **Une coupure de courant ne peut pas
déclencher un effacement logiciel** ; aucune purge de chaque cellule de RAM,
cache ou périphérique n’est garantie. Ces images ne provisionnent ni Secure Boot,
ni chiffrement flash, ni protection irréversible contre un accès physique.
Le logiciel ne protège pas contre un appareil modifié ou un firmware remplacé.

**Gardez un appareil de confiance :** un appareil préflashé, prêté ou dont le
passé est inconnu peut avoir été modifié. Réinstaller un firmware de confiance
ne suffit pas à exclure une modification matérielle. Saisissez et consultez vos
secrets à l’abri des regards et des caméras ; les traces de doigts peuvent aussi
révéler des indices sur la saisie. Après le flash, utilisez une alimentation
autonome sans données USB, pas un ordinateur ou un hôte USB inconnu.
Supprimer un fichier de la microSD ne garantit pas l’effacement de ses anciennes
copies physiques.

Tests logiciels, contrôles d’images et essais de démarrage ne constituent pas
une certification. Consultez le
[suivi des dépendances](DEPENDENCIES.md).

## Pour aller plus loin

- [Installation et empreintes](webflasher/README.md)
- [Fonctions et nouveautés de chaque version](webflasher/CHANGELOG.md)
- [Compiler pour ESP32-2432S028R](targets/cyd/README.md#compiler)
- [Compiler pour Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3](targets/waveshare_p4/README.md#compilation)
- [Tests d’interface](tests/ui/README.md) · [Tests crypto](tests/crypto/README.md)
- [Licence MIT](LICENSE)

Les bancs de tests hôte sont principalement conçus pour Windows avec MSVC.
Les contrôles de publication et du Web Flasher restent exécutés sur GitHub Actions.

<div align="center">

**AURORA — Comprendre vos sauvegardes. Garder la main sur vos clés.**

</div>
