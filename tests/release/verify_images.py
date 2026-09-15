"""Offline release gate: verify the bytes actually installed, not filenames."""
from pathlib import Path
import hashlib
from html.parser import HTMLParser
import json
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[2]
WEB = ROOT / 'webflasher'
FW = WEB / 'firmware'
CATALOG = {
    'cyd-1.9.4': ('manifest.json', '1.9.4', 0, 0x1000, None),
    'cyd-1.9.3': ('manifests/cyd-1.9.3.json', '1.9.3', 0, 0x1000, None),
    'cyd-1.7.5': ('manifests/cyd-1.7.5.json', '1.7.5', 0, 0x1000, None),
    'p4-rev1-2.0.0': ('manifests/p4-rev1-2.0.0.json', '2.0.0', 18, 0x2000, (100, 199)),
    'p4-rev3-2.0.0': ('manifests/p4-rev3-2.0.0.json', '2.0.0', 18, 0x2000, (300, 399)),
}


def image(data, offset, chip, revisions):
    assert data[offset] == 0xe9, 'ESP magic'
    assert struct.unpack_from('<H', data, offset + 12)[0] == chip, 'Wrong chip'
    if revisions:
        assert struct.unpack_from('<HH', data, offset + 15) == revisions, 'Wrong silicon range'
    assert data[offset + 2] == 2, 'Expected DIO'
    assert data[offset + 3] == (0x5f if chip else 0x20), 'Wrong flash size/frequency'
    end = offset + 24
    checksum = 0xef
    for _ in range(data[offset + 1]):
        _, size = struct.unpack_from('<II', data, end)
        end += 8
        assert end + size <= len(data), 'Truncated segment'
        for value in data[end:end + size]:
            checksum ^= value
        end += size
    end = offset + ((end - offset) // 16 + 1) * 16
    assert data[end - 1] == checksum, 'ESP segment checksum'
    assert data[offset + 23] == 1, 'ESP appended digest required'
    assert hashlib.sha256(data[offset:end]).digest() == data[end:end + 32], 'ESP digest'
    return end + 32


class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.options = []
        self.installers = {}
    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == 'option':
            self.options.append(attrs['value'])
        if tag == 'esp-web-install-button':
            assert attrs['data-release'] not in self.installers
            self.installers[attrs['data-release']] = attrs['manifest'].removeprefix('./')


def main():
    page = Page()
    page.feed((WEB / 'index.html').read_text(encoding='utf-8'))
    assert set(page.options) == set(CATALOG) and len(page.options) == len(CATALOG)
    assert set(page.installers) == set(CATALOG)
    assert {p.relative_to(WEB).as_posix() for p in WEB.rglob('*.json')} == {v[0] for v in CATALOG.values()} | {'firmware/flash-layout.json'}
    names = set()
    for release, (manifest, version, chip, boot, revisions) in CATALOG.items():
        assert page.installers[release] == manifest
        path = WEB / manifest
        config = json.loads(path.read_text(encoding='utf-8-sig'))
        assert config['version'] == version
        part = config['builds'][0]['parts'][0]
        path = (path.parent / part['path']).resolve()
        assert path.parent == FW.resolve() and part['offset'] == 0
        names.add(path.name)
        data = path.read_bytes()
        image(data, boot, chip, revisions)
        end = image(data, 0x10000, chip, revisions)
        assert end == len(data), 'Unexpected tail in factory image'
        assert data[0x8000:0x8002] == b'\xaa\x50', 'Missing partition table'
        if chip:
            # ESP-IDF app descriptor begins in the first app segment.
            assert struct.unpack_from('<I', data, 0x10020)[0] == 0xabcd5432
            actual = data[0x10030:0x10050].split(b'\0')[0].decode()
            assert actual == version, (release, actual)
        else:
            assert version.encode() + b'\0' in data[0x10000:]
        if release == 'cyd-1.7.5':
            assert hashlib.sha256(data[0x10000:]).hexdigest() == 'ba275c95507a713335a15c2452d5ae47f70d95f077f3f624f93a96f83beb6bf4'
        if release in ('cyd-1.9.3', 'cyd-1.9.4'):
            assert b'aurora_scrub\0' in data[0x8000:0x8c00]
            assert b'coredump' not in data[0x8000:0x8c00]
        print('PASS: embedded images, checksums, version and silicon range:', release)
    assert {p.name for p in FW.glob('*.factory.bin')} == names
    # Optional locally prepared ZIPs: strictly allowlist public release inputs.
    for version, device, images in (
        ('1.9.4', 'CYD', ['aurora-1.9.4-esp32-2432s028r.factory.bin']),
        ('2.0.0', 'P4', [f'aurora-2.0.0-esp32-p4-rev{r}.factory.bin' for r in (1, 3)]),
    ):
        path = ROOT / f'tmp/release-candidates/AURORA-v{version}-{device}.zip'
        if not path.exists():
            continue
        with zipfile.ZipFile(path) as archive:
            assert set(archive.namelist()) == set(images) | {'README.md', 'SHA256SUMS.txt'}
            assert len(archive.namelist()) == len(images) + 2
            assert archive.read('README.md') == (WEB / f'releases/{version}.md').read_bytes()
            for name in images:
                assert archive.read(name) == (FW / name).read_bytes()
            expected = ''.join(f'{hashlib.sha256((FW / name).read_bytes()).hexdigest().upper()}  {name}\n' for name in images)
            assert archive.read('SHA256SUMS.txt').decode() == expected
        print('PASS: allowlisted release ZIP:', path.name)


if __name__ == '__main__':
    main()
