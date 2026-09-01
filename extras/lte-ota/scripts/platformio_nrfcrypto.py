"""Teach PlatformIO where Adafruit_nRFCrypto keeps its CC310 archive."""

Import("env", "projenv")

from os.path import isfile, join

framework_dir = env.PioPlatform().get_package_dir(
    "framework-arduinoadafruitnrf52"
)
if not framework_dir:
    raise RuntimeError("framework-arduinoadafruitnrf52 is required")

archive_dir = join(
    framework_dir,
    "libraries",
    "Adafruit_nRFCrypto",
    "src",
    "cortex-m4",
    "fpv4-sp-d16-hard",
)
archive = join(archive_dir, "libnrf_cc310_0.9.13-no-interrupts.a")
if not isfile(archive):
    raise RuntimeError(f"Adafruit_nRFCrypto archive not found: {archive}")

env.AppendUnique(LIBPATH=[archive_dir])
projenv.AppendUnique(LIBPATH=[archive_dir])
