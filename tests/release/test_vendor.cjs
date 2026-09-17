const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const root = path.resolve(__dirname, '../..');
const dir = path.join(root, 'webflasher/assets/vendor/esp-web-tools');
const metadata = JSON.parse(fs.readFileSync(path.join(dir, 'integrity.json'), 'utf8'));
const hash = (data, algorithm = 'sha256', encoding = 'hex') => crypto.createHash(algorithm).update(data).digest(encoding);
const verify = (data, expected) => assert.equal(hash(data), expected, 'Vendor integrity mismatch');
for (const [name, expected] of Object.entries(metadata.files)) {
  assert.ok(['install-button.js', 'LICENSES.txt'].includes(name));
  verify(fs.readFileSync(path.join(dir, name)), expected);
}
verify(fs.readFileSync(path.join(root, 'tools/webflasher/package-lock.json')), metadata.lockSha256);
const bundle = fs.readFileSync(path.join(dir, 'install-button.js'));
assert.equal(metadata.sri, `sha384-${hash(bundle, 'sha384', 'base64')}`);
assert.deepEqual(metadata.imports, []);
assert.ok(Object.keys(metadata.inputs).length > 5, 'Lazy upstream chunks must be included');
for (const chip of ['stub_flasher_32-', 'stub_flasher_32p4-', 'stub_flasher_32p4rc1-'])
  assert.ok(Object.keys(metadata.inputs).some(name => name.includes('/' + chip)), `Missing stub: ${chip}`);
const damaged = Buffer.from(bundle);
damaged[20] ^= 1;
assert.throws(() => verify(damaged, metadata.files['install-button.js']), /integrity mismatch/i);
const html = fs.readFileSync(path.join(root, 'webflasher/index.html'), 'utf8');
assert.ok(html.includes("script-src 'self';"));
assert.ok(!html.includes('unpkg.com'));
assert.ok(html.includes(`integrity="${metadata.sri}"`));
const scripts = [...html.matchAll(/<script\b([^>]*)>/g)].map(match => match[1]);
assert.equal(scripts.length, 2);
for (const script of scripts) assert.match(script, /src="\.\/assets\//);
assert.ok(scripts.some(script => script.includes('vendor/esp-web-tools/install-button.js') && script.includes(metadata.sri)));
assert.ok(metadata.packages.some(pkg => pkg.name === 'esp-web-tools' && pkg.version === '10.4.0'));
console.log('PASS: vendor hashes, lock, licenses, single bundle, local scripts, CSP, SRI and tamper rejection');
