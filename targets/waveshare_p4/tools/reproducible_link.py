"""Make pioarduino's ESP-IDF library order deterministic across hosts."""

from pathlib import PurePath

Import("env")


def stable_library_key(library):
    """Sort build nodes by a host-neutral path, with the archive name first."""
    value = str(library).replace("\\", "/")
    parts = value.split("/")
    for marker in (".pio", "build"):
        if marker in parts:
            parts = parts[parts.index(marker) + 1:]
            break
    normalized = "/".join(parts).casefold()
    return PurePath(normalized).name, normalized


def enforce_stable_link_order(source, target, env):
    libraries = list(env.get("LIBS", []))
    env.Replace(LIBS=sorted(libraries, key=stable_library_key))


# pioarduino derives LIBS from CMake's dependency graph. CMake may emit that
# graph in a different traversal order on Windows and Linux; GNU ld then lays
# out otherwise identical sections differently. Sort immediately before the
# application link so the same archives always enter the linker in one order.
env.AddPreAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(enforce_stable_link_order, "AURORA: stable library order"),
)
