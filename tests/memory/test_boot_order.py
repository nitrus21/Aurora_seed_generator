"""Static ordering regression for P4 app_main; complements native wipe faults.

No hardware/ROM sequencing or physical remanence claim is made by this check.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
source = (root / "targets/waveshare_p4/main/main.cpp").read_text(encoding="utf-8")
source = re.sub(r"//[^\n]*|/\*.*?\*/", "", source, flags=re.S)
main = source.split('extern "C" void app_main() {', 1)[1]
stages = [
    "auroraSecuritySetEmergencyWipe(",
    "ui.emergencyWipeSecrets();",
    "gpio_set_level(",
    "auroraStartupMemoryScrub(",
    "auroraSecurityPanic();",
    "bsp_display_start();",
    "ui.begin();",
    "bsp_display_backlight_on();",
]
positions = [main.index(stage) for stage in stages]
assert positions == sorted(positions), "Display/input must follow mandatory cleanup"
assert re.search(r"if\s*\(!auroraStartupMemoryScrub\([^;]+\)\)\s*auroraSecurityPanic\(\);", main)
assert main.count("ui.emergencyWipeSecrets();") == 2  # callback and immediate wipe
assert "auroraSecuritySetEmergencyWipe" in main.lstrip().splitlines()[0]
print("PASS: P4 fixed-buffer wipe first; radio isolation then verified heap wipe; failure resets before display/UI/backlight")
