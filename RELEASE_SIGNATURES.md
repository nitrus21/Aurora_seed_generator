# Vérifier les signatures des releases AURORA

À partir de la release P4 2.0.9, AURORA signe le manifeste de sommes SHA-256
avec une clé Ed25519 dédiée et le format de signature OpenSSH. Cette signature
authentifie le manifeste ; les SHA-256 qu'il contient couvrent ensuite les
images et le paquet ZIP de la release.

La signature de distribution ne remplace pas le Secure Boot du P4 et ne prouve
pas l'état matériel de la carte. Elle permet de détecter un manifeste ou un
fichier de release modifié après signature.

## Clé officielle

- Fichier : [`keys/aurora-release-signing.pub`](keys/aurora-release-signing.pub)
- Empreinte : `SHA256:DCIpjHyAHdQ0h0muM5Uz72S+B/hjYsd8iZyNLh+AJuQ`
- Algorithme : Ed25519
- Espace de signature : `aurora-release`

Comparez cette empreinte avec une copie obtenue par un canal de confiance avant
la première vérification. Une copie présente uniquement dans la même release ne
constitue pas un canal indépendant.

## Vérification sous Windows

Téléchargez le manifeste `SHA256SUMS-vX.Y.Z.txt` et sa signature `.sig` dans le
même dossier, puis exécutez depuis la racine du dépôt :

```powershell
.\tools\verify_release_signature.ps1 -Manifest .\SHA256SUMS-vX.Y.Z.txt
```

Le script utilise `ssh-keygen` fourni par Windows et refuse une signature, une
clé ou un manifeste incorrect.

## Vérification sous Linux ou macOS

Avec OpenSSH récent :

```sh
ssh-keygen -Y verify \
  -f keys/aurora-release-allowed_signers \
  -I aurora-release \
  -n aurora-release \
  -s SHA256SUMS-vX.Y.Z.txt.sig \
  < SHA256SUMS-vX.Y.Z.txt
```

Après le message `Good "aurora-release" signature`, vérifiez les fichiers avec
`sha256sum -c SHA256SUMS-vX.Y.Z.txt` sous Linux ou comparez chaque valeur avec
`shasum -a 256` sous macOS.

La clé privée reste chiffrée hors du dépôt, hors des assets GitHub et hors du
firmware. En cas de rotation ou de révocation, une nouvelle empreinte sera
annoncée explicitement ; une ancienne clé ne doit jamais être remplacée
silencieusement.
