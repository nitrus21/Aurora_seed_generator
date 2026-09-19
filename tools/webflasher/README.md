# Installateur Web auto-hébergé

Depuis la racine du dépôt, avec Node.js 20 et npm :

```sh
npm ci --prefix tools/webflasher --ignore-scripts --no-audit --no-fund
npm run check --prefix tools/webflasher
node tests/release/test_vendor.cjs
node tests/release/test_webflasher.cjs
```

`package-lock.json` verrouille les téléchargements et leurs intégrités npm.
`build.mjs` regroupe la distribution navigateur amont `dist/web` de
`esp-web-tools@10.4.0`, y compris tous ses imports dynamiques, dans **un seul
module sans import restant**. Il ne recompile pas les sources originales
d'ESP Web Tools : les entrées amont précompilées sont listées avec leurs hashes.
Les versions des dépendances listées servent à tracer les licences résolues,
pas à attester l'environnement de compilation de l'éditeur amont.

Pour une mise à jour examinée : changer explicitement les versions, régénérer
le verrou, exécuter `npm run build --prefix tools/webflasher`, puis reporter
l'empreinte `sri` de `integrity.json` dans `webflasher/index.html`. Examiner le
diff, les licences et les avis de sécurité ; ne pas exécuter `npm audit fix`
automatiquement. Relancer les tests avant toute publication. Le mode `check`
reconstruit en mémoire et ne remplace jamais les fichiers distribués.

Les assets générés sont suivis dans Git. `node_modules` et le cache ne le sont
pas. Le workflow Pages vérifie la reconstruction avant de déployer. Le site
n'a besoin ni de Node ni de npm lorsqu'un visiteur l'utilise.

Le contrôle statique teste aussi une altération en mémoire. Le test navigateur
`node tests/release/test_vendor_browser.cjs` nécessite Playwright et son Chromium
(résolution Node standard ou `NODE_PATH`) : huit choix, absence de requête
externe, dialogue sans port, refus SRI d'un bundle altéré. Il **simule l'annulation
du sélecteur USB**, ne connecte aucun appareil et ne remplace pas un flash réel.
`AURORA_TEST_CHROMIUM` peut désigner un exécutable Chromium local de test ;
en son absence, Playwright utilise son navigateur installé par défaut.

SRI et hashes ne protègent pas contre le remplacement simultané du code et des
références par un attaquant contrôlant le dépôt ou l'hébergement. La signature
indépendante des releases reste un sujet distinct.
