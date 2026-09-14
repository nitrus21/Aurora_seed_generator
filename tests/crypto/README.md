# Test natif du codec chiffré commun

Prérequis : Python 3, outils C++ Visual Studio et SDK ESP-IDF installé par la compilation P4.

```powershell
python tests/crypto/run.py
```

Le test compile **`src/sd_export.cpp`, `src/pin_security.cpp` et Mbed TLS réel** avec une carte simulée uniquement en RAM. Il vérifie les vecteurs PBKDF2-HMAC-SHA-256/AES-256-GCM et BIP39 SHA-512 inchangés, l'équivalence des API, la lecture V1 de 1 120 octets avec firmware `1.7.6`, l'aller-retour V2 de 1 200 octets, le rejet d'un mauvais mot de passe, des altérations/troncatures, l'effacement après échec, le non-écrasement et le nettoyage d'une écriture dont la synchronisation échoue. Le PIN est testé avec zéros initiaux, bornes 4–8, caractères refusés, sels indépendants et blocage après trois erreurs cumulées même séparées par un succès.

Ce n'est pas un test des contrôleurs SD physiques ni des accélérateurs cryptographiques ESP32. Les fixtures sont fictives, sans fonds ; aucune carte réelle n'est ouverte. Les échanges entre les deux lecteurs et l'autotest E00 restent à vérifier sur les appareils.
