// Build generated vendor assets from the reviewed npm lock, without CDN imports.
import { build, version as esbuildVersion } from 'esbuild';
import { createHash } from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.dirname(fileURLToPath(import.meta.url));
const output = path.resolve(root, '../../webflasher/assets/vendor/esp-web-tools');
const check = process.argv.includes('--check');
const hash = (data, algorithm = 'sha256', encoding = 'hex') =>
  createHash(algorithm).update(data).digest(encoding);
const result = await build({
  absWorkingDir: root,
  // Repackage the exact upstream browser distribution previously loaded from
  // unpkg, including its lazy chip stubs/dialogs. Do not reconstruct upstream's
  // unpublished CSS transformation from its incomplete dist/ source modules.
  entryPoints: ['node_modules/esp-web-tools/dist/web/install-button.js'],
  bundle: true, splitting: false, format: 'esm', platform: 'browser',
  target: ['es2022'], minify: true, legalComments: 'inline',
  write: false, metafile: true, outfile: 'install-button.js',
  logLevel: 'warning',
});
if (result.warnings.length) throw new Error('Review build warnings before vendoring');
if (result.outputFiles.length !== 1) throw new Error('Expected one self-contained JS bundle');
for (const entry of Object.values(result.metafile.outputs)) {
  if (entry.imports.length) throw new Error('Unbundled executable import');
}
const packageDirs = new Set();
const lock = JSON.parse(fs.readFileSync(path.join(root, 'package-lock.json'), 'utf8'));
// The browser distribution already embeds dependency code. Preserve licenses
// for all declared runtime dependencies, not only the entry package. These
// resolved versions are NOT a claim about upstream's original build environment.
for (const name of Object.keys(lock.packages)) {
  // SSR shim is server-only and absent from upstream's browser distribution.
  if (name && !/node_modules\/(?:esbuild|@esbuild\/[^/]+|@types\/[^/]+|@lit-labs\/ssr-dom-shim)$/.test(name))
    packageDirs.add(path.join(root, name));
}
for (const name of Object.keys(result.metafile.inputs)) {
  let directory = path.dirname(path.resolve(root, name));
  while (!fs.existsSync(path.join(directory, 'package.json'))) {
    const parent = path.dirname(directory);
    if (parent === directory) throw new Error(`Missing package/license for ${name}`);
    directory = parent;
  }
  if (!directory.startsWith(path.join(root, 'node_modules') + path.sep))
    throw new Error('Unexpected input outside pinned dependencies');
  packageDirs.add(directory);
}
const packages = [...packageDirs].map(directory => {
  const pkg = JSON.parse(fs.readFileSync(path.join(directory, 'package.json'), 'utf8'));
  const licenses = fs.readdirSync(directory).filter(name =>
    /^(licen[sc]e|copying|notice|copyrightnotice)(\.|$)/i.test(name) && fs.statSync(path.join(directory, name)).isFile());
  if (!licenses.length) throw new Error(`Missing license text: ${pkg.name}`);
  return { name: pkg.name, version: pkg.version, license: pkg.license,
    texts: licenses.sort().map(name => `${name}\n${fs.readFileSync(path.join(directory, name), 'utf8').replace(/\r\n/g, '\n')}`) };
}).sort((a, b) => a.name.localeCompare(b.name, 'en'));
const bundle = result.outputFiles[0].contents;
const files = {
  'install-button.js': bundle,
  'LICENSES.txt': Buffer.from(packages.map(pkg =>
    `===== ${pkg.name}@${pkg.version} (${pkg.license}) =====\n${pkg.texts.join('\n')}\n`).join('\n')),
};
const metadata = {
  entry: 'esp-web-tools@10.4.0/dist/web/install-button.js',
  provenance: 'Upstream prebuilt browser distribution; listed packages are resolved license dependencies, not an attestation of upstream build inputs.',
  esbuild: esbuildVersion,
  lockSha256: hash(fs.readFileSync(path.join(root, 'package-lock.json'))),
  packages: packages.map(({ texts, ...pkg }) => pkg),
  inputs: Object.fromEntries(Object.keys(result.metafile.inputs).sort().map(name =>
    [name.replaceAll('\\', '/'), hash(fs.readFileSync(path.join(root, name)))])),
  files: Object.fromEntries(Object.entries(files).map(([name, data]) => [name, hash(data)])),
  sri: `sha384-${hash(bundle, 'sha384', 'base64')}`,
  imports: [],
};
files['integrity.json'] = Buffer.from(JSON.stringify(metadata, null, 2) + '\n');
if (!check) fs.mkdirSync(output, { recursive: true });
for (const [name, data] of Object.entries(files)) {
  const target = path.join(output, name);
  if (check) {
    if (!fs.existsSync(target) || !Buffer.from(data).equals(fs.readFileSync(target)))
      throw new Error(`Vendor artifact differs: ${name}`);
  } else fs.writeFileSync(target, data);
}
console.log(`${check ? 'PASS: locked rebuild matches' : 'Built'} ${packages.length} packages; ${bundle.length} bytes; ${metadata.sri}`);
