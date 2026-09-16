# Hardware RNG bridge regression

Run `python tests/rng/run.py` on Windows with the existing Visual Studio C++ tools
and pinned CYD uBitcoin dependency installed. No board, seed or SD card is used.

The test compiles the actual patched uBitcoin `rand.c` and shared hardware source
ownership implementation for CYD and P4. It checks:

- full-file patch validation, idempotence and rejection of a modified bridge;
- `random32` and `random_buffer` read only while the entropy source is enabled;
- real scalar/point multiplication retains the public scalar-1 generator vector;
- nested collection/cryptographic reads do not disable another owner's source;
- concurrent owners balance activation and deactivation without caching RNG data.

Also run the existing entropy, codec, UI, storage and memory suites. Native mocks
cannot measure physical entropy or establish resistance to physical side channels.
Firmware builds must include `src/hardware_rng.cpp` and apply
`tools/patch_ubitcoin_rng.py` after the original pinned uBitcoin patches.
After compiling all three profiles, run `python tests/rng/verify_images.py` to
check the actual linked `random32` calls in their ELF files.

The entropy API is task-context only. Balanced ownership does not authorize a
new ADC/RF consumer to run concurrently: peripheral exclusion must still be
reviewed before adding hardware features.
