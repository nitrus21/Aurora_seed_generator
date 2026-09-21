// Exercise the real browser script with an isolated minimal DOM; no USB access.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const web = path.resolve(__dirname, '../../webflasher');
const html = fs.readFileSync(path.join(web, 'index.html'), 'utf8');
const code = fs.readFileSync(path.join(web, 'assets/app.js'), 'utf8');
const allVariants = [
  ['cyd-1.9.9', '1.9.9', 'manifest.json'],
  ['cyd-1.7.5', '1.7.5', 'manifests/cyd-1.7.5.json'],
  ['p4-rev1-2.0.12', '2.0.12', 'manifests/p4-rev1-2.0.12.json'],
  ['p4-rev3-2.0.12', '2.0.12', 'manifests/p4-rev3-2.0.12.json'],
  ['p4-rev1-2.0.11', '2.0.11', 'manifests/p4-rev1-2.0.11.json'],
  ['p4-rev3-2.0.11', '2.0.11', 'manifests/p4-rev3-2.0.11.json'],
  ['p4-rev1-2.0.9', '2.0.9', 'manifests/p4-rev1-2.0.9.json'],
  ['p4-rev3-2.0.9', '2.0.9', 'manifests/p4-rev3-2.0.9.json'],
  ['p4-rev1-2.0.5', '2.0.5', 'manifests/p4-rev1-2.0.5.json'],
  ['p4-rev3-2.0.5', '2.0.5', 'manifests/p4-rev3-2.0.5.json'],
  ['p4-rev1-2.0.3', '2.0.3', 'manifests/p4-rev1-2.0.3.json'],
  ['p4-rev3-2.0.3', '2.0.3', 'manifests/p4-rev3-2.0.3.json'],
];
const variants = process.env.AURORA_P4_ONLY === '1'
  ? allVariants.filter(([id]) => id.startsWith('p4-'))
  : allVariants;
assert.match(html, /<option value="p4-rev1-2\.0\.12" selected>/);
assert.doesNotMatch(html, /<option value="cyd-1\.9\.9" selected>/);
assert.match(html, /<esp-web-install-button id="install-button" data-release="p4-rev1-2\.0\.12"/);
assert.ok(!html.includes('id="header-target"') && !html.includes('id="header-version"'));
function element() {
  return { textContent: '', listeners: {}, classes: [], children: [], dataset: {},
    classList: {add() {}}, addEventListener(name, cb) {this.listeners[name] = cb;},
    replaceChildren(...items) {this.children = items;} };
}
async function test(secure, serial) {
  const elements = Object.fromEntries([...html.matchAll(/id="([^"]+)"/g)].map(m => ['#' + m[1], element()]));
  const installers = variants.map(([id]) => ({dataset: {release: id}, hidden: true}));
  elements['#firmware-selection'].value = variants[0][0];
  let clipboard;
  const context = { document: {
    querySelector(id) {assert.ok(elements[id], `Unknown DOM id ${id}`); return elements[id];},
    querySelectorAll(tag) {assert.equal(tag, 'esp-web-install-button'); return installers;},
    createElement() {return element();},
  }, window: {isSecureContext: secure, setTimeout() {}},
  navigator: {...(serial ? {serial: {}} : {}), clipboard: {async writeText(text) {clipboard = text;}}} };
  vm.runInNewContext(code, context);
  for (const [id, version, manifest] of variants) {
    elements['#firmware-selection'].value = id;
    elements['#firmware-selection'].listeners.change();
    assert.equal(installers.filter(b => !b.hidden).length, 1);
    assert.equal(installers.find(b => !b.hidden).dataset.release, id);
    assert.equal(elements['#release-version'].textContent, `v${version}`);
    assert.equal(elements['#changes-title'].textContent, version === '1.7.5' ? 'Version conservée' : 'Nouveautés de cette version');
    const board = id.startsWith('cyd-') ? 'ESP32-2432S028R' : 'Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3';
    for (const selector of ['#selected-board', '#release-description', '#target-warning']) {
      assert.ok(elements[selector].textContent.includes(board), `${selector} must show the complete board reference`);
    }
    assert.ok(html.includes(`value="${id}"${id === 'p4-rev1-2.0.12' ? ' selected' : ''}>${board}`));
    const config = JSON.parse(fs.readFileSync(path.join(web, manifest), 'utf8'));
    const binary = path.resolve(web, path.dirname(manifest), config.builds[0].parts[0].path);
    const digest = require('node:crypto').createHash('sha256').update(fs.readFileSync(binary)).digest('hex').toUpperCase();
    const applicationDigest = fs.readFileSync(binary).subarray(-32).toString('hex').toUpperCase();
    assert.equal(elements['#firmware-hash'].textContent, digest);
    assert.equal(elements['#application-hash'].textContent, applicationDigest);
    assert.equal(elements['#release-hash'].textContent, digest);
    assert.ok(elements['#release-changes'].children.length >= 2);
    assert.ok(fs.existsSync(path.resolve(web, elements['#release-notes'].href)));
    await elements['#copy-hash'].listeners.click();
    assert.equal(clipboard, digest);
    await elements['#copy-application-hash'].listeners.click();
    assert.equal(clipboard, applicationDigest);
    assert.ok(elements['#application-hash-note'].textContent.includes(
      ['2.0.11', '2.0.12'].includes(version) ? 'affichée' : 'ne l’affiche pas'
    ));
    if (id.includes('p4')) assert.ok(elements['#target-warning'].textContent.includes(id.includes('rev1') ? '1.x' : '3.x'));
  }
  assert.equal(elements['#sha-table-body'].children.length, allVariants.length);
  for (const row of elements['#sha-table-body'].children) {
    assert.equal(row.children.length, 5);
    assert.match(row.children[2].children[0].textContent, /^[0-9A-F]{64}$/);
    assert.match(row.children[3].children[0].textContent, /^[0-9A-F]{64}$/);
  }
  assert.ok(elements['#compatibility-text'].textContent.includes(!secure ? 'HTTPS' : serial ? 'compatible' : 'indisponible'));
}
(async () => {
  for (const [secure, serial] of [[true, true], [true, false], [false, true]]) await test(secure, serial);
  console.log(`PASS: ${variants.length} selections, both SHA-256 values, full table, notes, copy and browser compatibility`);
})().catch(error => {console.error(error); process.exitCode = 1;});
