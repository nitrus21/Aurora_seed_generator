"""Reject diagnostic strings in current local builds, not historical releases."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json

root = Path(__file__).resolve().parents[2]
targets = {
    "cyd": root / ".pio/build/esp32-2432S028R/firmware.bin",
    "p4-rev1": root / "targets/waveshare_p4/.pio/build/waveshare-p4-rev1/firmware.bin",
    "p4-rev3": root / "targets/waveshare_p4/.pio/build/waveshare-p4/firmware.bin",
}
markers = [
    b"AURORA autotest :", "AURORA autotest durée :".encode(),
    b"AURORA export SD :", b"AURORA lecture SD :",
    b"AURORA: startup scrub verified", b"AURORA P4: unexpected display profile",
    b"AURORA P4: display startup lock timed out",
    b"interface P4 480x800 initialisee",
]
results = {}
for name, file in targets.items():
    data = file.read_bytes()
    assert all(marker not in data for marker in markers), f"Diagnostic message remains: {name}"
    results[name] = {"file": file.relative_to(root).as_posix(), "bytes": len(data),
                     "sha256": hashlib.sha256(data).hexdigest(), "applicationDiagnosticMarkers": 0}
    print("PASS:", name, results[name]["sha256"], "no application diagnostic strings")
report = root / "tmp/evidence/firmware-logging.json"
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps({"dateUTC": datetime.now(timezone.utc).isoformat(),
    "scope": "Local unpublished builds; no physical serial capture; SDK/ROM logs not covered",
    "targets": results}, indent=2) + "\n", encoding="utf-8")
