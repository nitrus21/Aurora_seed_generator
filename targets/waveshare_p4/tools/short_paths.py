"""Resolve existing Windows files to aliases of the SAME files, for ESP-IPA."""
import ctypes
from ctypes import wintypes
import os
import sys

get_short = ctypes.WinDLL("kernel32", use_last_error=True).GetShortPathNameW
get_short.argtypes = [wintypes.LPCWSTR, wintypes.LPWSTR, wintypes.DWORD]
get_short.restype = wintypes.DWORD
aliases = []
for path in sys.argv[1:]:
    path = os.path.abspath(path)
    buffer = ctypes.create_unicode_buffer(32768)
    length = get_short(path, buffer, len(buffer))
    if not length or length >= len(buffer) or any(c.isspace() for c in buffer.value):
        raise SystemExit("An existing, space-free Windows short path is required: " + path)
    aliases.append(buffer.value.replace("\\", "/"))
print(";".join(aliases))
