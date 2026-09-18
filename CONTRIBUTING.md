# Contribuer à AURORA

Ce dépôt est la distribution publique d’AURORA. Le développement courant,
les audits, les essais matériels et leurs preuves sont conservés dans un dépôt
privé distinct. Une modification n’arrive ici qu’après validation et nettoyage.

## Contenu public autorisé

- sources nécessaires à la compilation et à la compréhension du produit ;
- documentation utilisateur et documentation de compilation ;
- tests reproductibles utilisant exclusivement des données synthétiques ;
- images Web Flasher finales, nommées et versionnées `*.factory.bin` ;
- manifestes, empreintes SHA-256 et notes des versions réellement distribuées.

## Contenu qui ne doit jamais être publié ici

- rapport d’audit, `SECURITY.md`, rapport de validation ou preuve privée ;
- seed réelle, passphrase, clé privée, export de portefeuille ou fichier `.aurora` ;
- cache, sortie brute de compilation, dump, readback ou journal d’appareil ;
- chemin local, nom d’utilisateur du poste ou adresse électronique personnelle ;
- `sdkconfig` généré, `managed_components`, `.pio`, `tmp`, `.vscode`, `.agents`,
  `.codex` ou `AGENTS.md` ;
- binaire retiré, non validé ou non associé à une version publique.

Les exports privés Electrum et Sparrow sont des fonctions du produit, pas des
artefacts à joindre au dépôt. Les vecteurs de test doivent être clairement
identifiés comme synthétiques et ne jamais être utilisés avec des fonds.

## Avant une publication

Exécuter :

```powershell
python tools/check_public_repo.py
pwsh tests/release/verify.ps1 -ReleasedArtifactsOnly
```

Le contrôle automatique complète la revue humaine ; il ne prouve pas qu’un
fichier est sûr à publier. Les branches de travail sont poussées uniquement sur
le dépôt privé. Le dépôt public conserve une branche de distribution propre.
