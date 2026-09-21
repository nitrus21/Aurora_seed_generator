"""Package verified Web Flasher images locally; never publish or access a device.

The explicit allowlist excludes source trees, private reports and build logs.
Pinned identities prevent replacing the validated candidate silently.
P4 rev3 is software-validated only; the release notes preserve this distinction.
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
        'aurora-1.7.5-esp32-2432s028r.factory.bin': '469A8912CD2A7BA3EF919467D84D6857CF60E726A27865157BD1A04653510218',
        'aurora-1.9.9-esp32-2432s028r.factory.bin': '34A7928985AA88D0E79F2D48EB5143A839E69106B2322DB78D2404EAB03DAD1E',
        'aurora-2.0.3-esp32-p4-rev1.factory.bin': 'C4DC3D3A0178402CCFB1F9A5B16246C346A0BDD4AAB5DFEA4FE8CC84E1261594',
        'aurora-2.0.3-esp32-p4-rev3.factory.bin': '72B909397C61C5731DFDA801C6BB2F503941152811FB13E740AB83CB97CCA6B7',
        'aurora-2.0.5-esp32-p4-rev1.factory.bin': '6D6253867D4962AACDB012F0445444E386C519044543F41DAE20CA04517722CB',
        'aurora-2.0.5-esp32-p4-rev3.factory.bin': '2BC03AAC78FBF9583B6B4918C9A89D807A55D96B554909B21FE486C67CA77884',
        'aurora-2.0.9-esp32-p4-rev1.factory.bin': 'F4F10C4D9D9204C949FCF544EFD1469F9225814C1C17B57B48AAC6E971C7B384',
        'aurora-2.0.9-esp32-p4-rev3.factory.bin': '128CB229102C4FCD45416574C2DF08B954EE1DE4DF1311FA8D3A67AE1CA94F4D',
        'aurora-2.0.11-esp32-p4-rev1.factory.bin': '962A04E1FF65D0CE38FD13E3700F74E15852315999C031E972761497D3E0994E',
        'aurora-2.0.11-esp32-p4-rev3.factory.bin': '7B6F2C69988862233341413677AD866131AABEC82F319A7DBF3F891962F2B0A7',
    }
    for name, checksum in expected.items():
        if digest(FIRMWARE / name) != checksum:
            raise RuntimeError(f'Image differs from validated release candidate: {name}')
    names = sorted(p.name for p in FIRMWARE.glob('*.factory.bin'))
    (FIRMWARE / 'SHA256SUMS.txt').write_text(
        ''.join(f'{digest(FIRMWARE / name)}  {name}\n' for name in names), encoding='ascii', newline='\n')
    for version, device, images in (
        ('1.7.5', 'CYD', ['aurora-1.7.5-esp32-2432s028r.factory.bin']),
        ('1.9.9', 'CYD', ['aurora-1.9.9-esp32-2432s028r.factory.bin']),
        ('2.0.3', 'P4', [f'aurora-2.0.3-esp32-p4-rev{r}.factory.bin' for r in (1, 3)]),
        ('2.0.5', 'P4', [f'aurora-2.0.5-esp32-p4-rev{r}.factory.bin' for r in (1, 3)]),
        ('2.0.9', 'P4', [f'aurora-2.0.9-esp32-p4-rev{r}.factory.bin' for r in (1, 3)]),
    ):
        checksums = ''.join(f'{digest(FIRMWARE / name)}  {name}\n' for name in images)
        notes = (WEB / f'releases/{version}.md').read_bytes()
        archive = OUT / f'AURORA-v{version}-{device}.zip'
        # Fixed entry metadata makes the ZIP reproducible for the same inputs.
        with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as bundle:
            entries = [(name, (FIRMWARE / name).read_bytes()) for name in images]
            entries += [('SHA256SUMS.txt', checksums.encode('ascii')), ('README.md', notes)]
            for name, data in entries:
                entry = zipfile.ZipInfo(name, (2026, 9, 16, 0, 0, 0))
                entry.compress_type = zipfile.ZIP_DEFLATED
                bundle.writestr(entry, data)
        (OUT / f'SHA256SUMS-v{version}.txt').write_text(
            checksums + f'{digest(archive)}  {archive.name}\n', encoding='ascii', newline='\n')
        print(f'{archive.name}: {digest(archive)}')
    print('Local candidates only. Use prepare_p4_2_0_11_candidate.py for the signed 2.0.11 package.')
    print('Run tests/release/verify.ps1 before publication.')


if __name__ == '__main__':
    main()
