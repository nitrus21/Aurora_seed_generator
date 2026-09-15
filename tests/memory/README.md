# P4 memory-cleanup control-flow tests

Run from the project root on Windows with Visual Studio C/C++ build tools:

```powershell
powershell -ExecutionPolicy Bypass -File tests/memory/run.ps1
powershell -ExecutionPolicy Bypass -File tests/memory/run.ps1 -Optimized
```

The test includes the current production `security_memory.c` and
`secure_lvgl_memory.c` with both `AURORA_BOARD_P4` and `ESP_PLATFORM` defined.
Only SDK, hardware and heap-capability operations are replaced by local mocks.
Fixtures are public dummy bytes; no device, wallet, microSD or network is used.
Build artifacts stay under ignored `tmp/memory-native/`.

Checks cover:

- One visit per allocatable heap payload, complete wiping before release, no
  writes into live SDK allocations or excluded tiny tails.
- Allocation failures at four stages and the bounded fragmentation failure,
  including release of every previously acquired block.
- Exact-range zeroing, C2M/data/unaligned cache flags without invalidation,
  uncached addresses, zero-length input, and contiguous bounded 64 KiB
  writeback requests through the partial tail of a large unaligned buffer.
- Fatal null-pointer/cache-sync errors; both core selections; frozen-registry
  wiping versus the deliberate busy-registry fallback.
- Fixed emergency callback, no allocation or SDK cache-lock calls after the
  mocked CPU stall, and ROM L1/L2 writeback followed by reset.

Before-free checks inspect still-owned memory, not freed pointers. The test
uses `longjmp` solely to observe the terminal reset path; production never
resumes after that path. RISC-V assembly and physical caches are not executed.

Passing these tests is **not** proof of complete physical-memory erasure. They
cannot establish ROM writeback success, real cross-core coherence, DMA or
peripheral quiescence, watchdog timing, hardware-debug protection, or erasure
during sudden power loss. The startup sweep intentionally excludes live SDK
memory, heap metadata and unusable tails. The busy-registry panic path leaves
its UI allocations for later cleanup. Board-level validation remains required.
