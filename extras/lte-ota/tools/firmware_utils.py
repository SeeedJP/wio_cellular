from __future__ import annotations

import json
import pathlib
import zipfile


def read_firmware(path: pathlib.Path) -> bytes:
    if path.suffix.lower() in (".hex", ".elf"):
        raise ValueError("use a raw .bin or an application DFU .zip, not HEX/ELF")
    if path.suffix.lower() != ".zip":
        return path.read_bytes()
    with zipfile.ZipFile(path) as package:
        binary_name = "firmware.bin"
        if "manifest.json" in package.namelist():
            document = json.loads(package.read("manifest.json"))
            if not isinstance(document, dict):
                raise ValueError("DFU manifest must be a JSON object")
            manifest = document.get("manifest", {})
            if not isinstance(manifest, dict) or any(
                key in manifest
                for key in ("bootloader", "softdevice", "softdevice_bootloader")
            ):
                raise ValueError("OTA accepts application-only DFU packages")
            application = manifest.get("application")
            if not isinstance(application, dict):
                raise ValueError("DFU manifest does not describe an application")
            binary_name = application.get("bin_file")
            if (
                not isinstance(binary_name, str)
                or not binary_name.endswith(".bin")
                or pathlib.PurePosixPath(binary_name).is_absolute()
                or ".." in pathlib.PurePosixPath(binary_name).parts
            ):
                raise ValueError("invalid application bin_file in DFU manifest")
        try:
            return package.read(binary_name)
        except KeyError as error:
            raise ValueError(f"{path} does not contain {binary_name}") from error


def crc16_ccitt(data: bytes, crc: int = 0xFFFF) -> int:
    for value in data:
        crc = ((crc >> 8) & 0xFF) | ((crc << 8) & 0xFFFF)
        crc ^= value
        crc ^= (crc & 0xFF) >> 4
        crc ^= (crc << 12) & 0xFFFF
        crc ^= ((crc & 0xFF) << 5) & 0xFFFF
    return crc & 0xFFFF
