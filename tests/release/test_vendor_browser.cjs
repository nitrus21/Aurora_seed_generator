// Real headless browser, simulated cancelled USB chooser. Never opens a port.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const path = require('node:path');
const {chromium} = require('playwright');
const root = path.resolve(__dirname, '../../webflasher');
let tamper = false;
const server = http.createServer((req, res) => {
  const pathname = decodeURIComponent(new URL(req.url, 'http://localhost').pathname);
  const file = path.resolve(root, '.' + (pathname === '/' ? '/index.html' : pathname));
  if (!file.startsWith(root + path.sep) || !fs.existsSync(file) || !fs.statSync(file).isFile()) {
    res.writeHead(404).end(); return;
  }
  let body = fs.readFileSync(file);
  if (tamper && file.endsWith(path.join('esp-web-tools', 'install-button.js')))
    body = Buffer.concat([body, Buffer.from('\n/* deliberate integrity failure */')]);
  const mime = {'.html':'text/html; charset=utf-8', '.js':'text/javascript', '.css':'text/css', '.json':'application/json', '.png':'image/png'};
  res.writeHead(200, {'Content-Type':mime[path.extname(file)] || 'application/octet-stream', 'Cache-Control':'no-store'}).end(body);
});
(async () => {
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const origin = `http://127.0.0.1:${server.address().port}`;
  let browser;
  try {
    browser = await chromium.launch({headless:true,
      ...(process.env.AURORA_TEST_CHROMIUM ? {executablePath:process.env.AURORA_TEST_CHROMIUM} : {})});
    for (const variant of ['cyd-1.9.9','cyd-1.7.5','p4-rev1-2.0.11','p4-rev3-2.0.11',
      'p4-rev1-2.0.9','p4-rev3-2.0.9',
      'p4-rev1-2.0.5','p4-rev3-2.0.5','p4-rev1-2.0.3','p4-rev3-2.0.3']) {
      const context = await browser.newContext();
      const external = [], errors = [];
      await context.route('**/*', route => {
        if (new URL(route.request().url()).origin !== origin) {
          external.push(route.request().url()); return route.abort();
        }
        return route.continue();
      });
      await context.addInitScript(() => {
        window.chooserCalls = 0;
        window.cspFailures = [];
        document.addEventListener('securitypolicyviolation', e => window.cspFailures.push(e.blockedURI));
        Object.defineProperty(navigator, 'serial', {value:{requestPort: async () => {
          window.chooserCalls++;
          throw new DOMException('Test: chooser cancelled', 'NotFoundError');
        }}});
      });
      const page = await context.newPage();
      page.on('pageerror', error => errors.push(error.message));
      await page.goto(origin);
      await page.waitForFunction(() => !!customElements.get('esp-web-install-button'));
      await page.selectOption('#firmware-selection', variant);
      const installer = page.locator(`esp-web-install-button[data-release="${variant}"]`);
      await installer.locator('button[slot="activate"]').click();
      await page.waitForFunction(() => window.chooserCalls === 1 &&
        !!customElements.get('ewt-install-dialog') && !!document.querySelector('ewt-no-port-picked-dialog'));
      const manifest = await installer.getAttribute('manifest');
      assert.equal(await page.evaluate(async url => {
        const m = await (await fetch(url)).json();
        return (await fetch(new URL(m.builds[0].parts[0].path, new URL(url, location.href)))).ok;
      }, manifest), true);
      assert.deepEqual(await page.evaluate(() => window.cspFailures), []);
      assert.deepEqual(external, []);
      assert.deepEqual(errors, []);
      await context.close();
      console.log(`PASS browser: ${variant}, local manifest/binary, cancelled chooser, no external request/CSP error`);
    }
    tamper = true;
    const context = await browser.newContext();
    const page = await context.newPage();
    const messages = [];
    page.on('console', message => messages.push(message.text()));
    await page.goto(origin);
    await page.waitForLoadState('networkidle');
    assert.equal(await page.evaluate(() => !!customElements.get('esp-web-install-button')), false);
    assert.ok(messages.some(message => /integrity|digest/i.test(message)));
    await context.close();
    console.log('PASS browser: altered bundle rejected by SRI before registration');
  } finally { if (browser) await browser.close(); server.close(); }
})().catch(error => {console.error(error); process.exitCode=1; server.close();});
