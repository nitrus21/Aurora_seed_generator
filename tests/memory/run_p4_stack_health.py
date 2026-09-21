"""Static policy checks for the temporary P4 rev1 stack diagnostic."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
platformio = (root / "targets/waveshare_p4/platformio.ini").read_text(encoding="utf-8")
main = (root / "targets/waveshare_p4/main/main.cpp").read_text(encoding="utf-8")
ui = (root / "src/ui.cpp").read_text(encoding="utf-8")
diagnostic = (root / "targets/waveshare_p4/main/p4_stack_health.cpp").read_text(encoding="utf-8")
revision = (root / "targets/waveshare_p4/tools/image_revision.py").read_text(encoding="utf-8")

section = re.search(r"\[env:waveshare-p4-rev1-stack-health\](.*?)(?:\n\[|\Z)",
                    platformio, re.DOTALL)
assert section and "extends = env:waveshare-p4-rev1" in section.group(1)
assert "-DAURORA_P4_STACK_HEALTH=1" in section.group(1)
assert "default_envs = waveshare-p4\n" in platformio.replace("\r\n", "\n")
assert '"waveshare-p4-rev1-stack-health": (100, 199)' in revision
assert "auroraStackHealthBegin(xTaskGetCurrentTaskHandle());" in main
assert "auroraStackHealthSample();" in main
assert "uxTaskGetStackHighWaterMark" in diagnostic
assert 'xTaskGetHandle("lvgl")' in diagnostic
assert "AURORA_P4_STACK_HEALTH" in ui
for forbidden in ("AURORA_DIAG", "sdmmc", "writeAuroraWallet", "fopen", "printf(",
                  "Serial", "esp_log"):
    assert forbidden not in diagnostic
print("PASS: rev1 stack diagnostic is non-default, display-only and revision bounded")
