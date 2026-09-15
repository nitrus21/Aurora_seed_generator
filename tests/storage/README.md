# Tests des tampons stdio P4

Depuis la racine, avec Python 3 et les outils C++ Visual Studio :

```powershell
python tests/storage/run.py
```

Le test compile **le vrai `targets/waveshare_p4/main/storage.cpp`** sans dépendre
du build P4 ou de LVGL. Il utilise de vrais `FILE` et `setvbuf` du CRT Windows,
des adaptateurs POSIX et des stubs SDK pour le montage/alimentation. Les hooks
de l'allocateur vérifient que les 512 octets sont initialisés et effacés avant
libération, après `fclose`, y compris en cas d'erreur injectée.

Couverture : lecture/écriture, identité binaire et des retours à la ligne,
création exclusive sans écrasement, allocation/`setvbuf`/`fdopen` en échec,
`fclose`/`fflush`/`fsync` en erreur, déplacements de fichiers/répertoires,
auto-déplacement, libération du propriétaire précédent, itération avec un
fichier refusé, parcours non autorisés et durée de vie du montage/LDO.

Les fichiers de test publics sont conservés dans un sous-répertoire unique de
`tmp/storage-native/`. Aucune carte SD, clé réelle ou donnée existante n'est lue.
L'export Electrum n'est pas modifié par ce correctif : ses octets sont toujours
écrits tels quels. Les tests du sérialiseur restent dans `tests/crypto`.

Limites : ces tests ne valident ni Newlib/FATFS sur le P4, ni l'effacement
physique de la carte ou des caches, ni une coupure réelle d'alimentation. Le
hook d'allocation renvoie NULL pour injecter l'échec récupérable ; la politique
terminale réelle de l'allocateur sécurisé est testée séparément.
