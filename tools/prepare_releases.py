"""Package verified Web Flasher images locally; never publish or access a device.

The explicit allowlist excludes source trees, private reports and build logs.
Pinned identities prevent replacing the hardware-tested candidate silently.
"""
from pathlib import Path
import hashlib
import zipfile

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / 'webflasher'
FIRMWARE = WEB / 'firmware'
OUT = ROOT / 'tmp/release-candidates'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    expected = {
        'aurora-1.9.5-esp32-2432s028r.factory.bin': 'FFD0E180E90D0C44378CA4E6134C7CD1842F8B4C768831E5B7D77EA6D72276B5',
        'aurora-2.0.0-esp32-p4-rev1.factory.bin': 'C412BE99E5D42991C2D333437559836B452175C9CD785F50C59A8706F6A56D0F',
        'aurora-2.0.0-esp32-p4-rev3.factory.bin': 'E4454A36DC0928BD70D5F25AE62BEC284383C60E0A8740B4C27B5A83507C315E',
    }
    for name, checksum in expected.items():
        if digest(FIRMWARE / name) != checksum:
            raise RuntimeError(f'Image differs from validated release candidate: {name}')
    names = sorted(p.name for p in FIRMWARE.glob('*.bin'))
    (FIRMWARE / 'SHA256SUMS.txt').write_text(
        ''.join(f'{digest(FIRMWARE / name)}  {name}\n' for name in names), encoding='ascii')
    for version, device, images in (
        ('1.9.5', 'CYD', ['aurora-1.9.5-esp32-2432s028r.factory.bin']),
        ('2.0.0', 'P4', [f'aurora-2.0.0-esp32-p4-rev{r}.factory.bin' for r in (1, 3)]),
    ):
        checksums = ''.join(f'{digest(FIRMWARE / name)}  {name}\n' for name in images)
        notes = (WEB / f'releases/{version}.md').read_bytes()
        archive = OUT / f'AURORA-v{version}-{device}.zip'
        # Fixed entry metadata makes the ZIP reproducible for the same inputs.
        with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as bundle:
            entries = [(name, (FIRMWARE / name).read_bytes()) for name in images]
            entries += [('SHA256SUMS.txt', checksums.encode('ascii')), ('README.md', notes)]
            for name, data in entries:
                entry = zipfile.ZipInfo(name, (2026, 9, 15, 0, 0, 0))
                entry.compress_type = zipfile.ZIP_DEFLATED
                bundle.writestr(entry, data)
        (OUT / f'SHA256SUMS-v{version}.txt').write_text(
            checksums + f'{digest(archive)}  {archive.name}\n', encoding='ascii')
        print(f'{archive.name}: {digest(archive)}')
    print('Local candidates only. Run tests/release/verify.ps1 before publication.')


if __name__ == '__main__':
    main()
