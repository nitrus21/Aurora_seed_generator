"""Package existing builds locally; never commit, publish or access a device.

Run with the PlatformIO Python (esptool installed), after building both P4
profiles and CYD. The explicit allowlist excludes source trees/private reports.
"""
from pathlib import Path
import hashlib
import shutil
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / 'webflasher'
FIRMWARE = WEB / 'firmware'
OUT = ROOT / 'tmp/release-candidates'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    cyd = ROOT / '.pio/build/esp32-2432S028R'
    # Release identity: the CYD application validated and flashed for 1.9.3.
    if digest(cyd / 'firmware.bin') != '45F552833A0D45A660AAC8B1405B8D81E89AE7425E9A0F4E77C55CA46CC75DEC':
        raise RuntimeError('CYD application differs from the validated 1.9.3 candidate')
    factory = FIRMWARE / 'aurora-1.9.3-esp32-2432s028r.factory.bin'
    subprocess.run([sys.executable, '-X', 'utf8', '-m', 'esptool', '--chip', 'esp32',
                    'merge-bin', '-o', str(factory), '--flash-mode', 'dio',
                    '--flash-freq', '40m', '--flash-size', '4MB',
                    '0x1000', str(cyd / 'bootloader.bin'),
                    '0x8000', str(cyd / 'partitions.bin'),
                    '0xe000', str(FIRMWARE / 'boot_app0.bin'),
                    '0x10000', str(cyd / 'firmware.bin')], check=True)
    for name in ('firmware.bin', 'partitions.bin'):
        shutil.copy2(cyd / name, FIRMWARE / name)
    # merge-bin normalizes the bootloader flash header/digest. Distribute those
    # exact bytes for separate-part installations as well as the factory image.
    boot_len = (cyd / 'bootloader.bin').stat().st_size
    (FIRMWARE / 'bootloader.bin').write_bytes(factory.read_bytes()[0x1000:0x1000 + boot_len])
    for revision, profile in ((1, 'waveshare-p4-rev1'), (3, 'waveshare-p4')):
        source = ROOT / f'targets/waveshare_p4/.pio/build/{profile}/firmware.factory.bin'
        shutil.copy2(source, FIRMWARE / f'aurora-2.0.0-esp32-p4-rev{revision}.factory.bin')
    names = sorted(p.name for p in FIRMWARE.glob('*.bin'))
    (FIRMWARE / 'SHA256SUMS.txt').write_text(
        ''.join(f'{digest(FIRMWARE / name)}  {name}\n' for name in names), encoding='ascii')
    for version, device, images in (
        ('1.9.3', 'CYD', [factory.name]),
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
