#!/usr/bin/env python3
"""Create the transport-independent metadata needed by the OTA writer."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import struct
import sys
import tempfile

from firmware_utils import crc16_ccitt, read_firmware

MAXIMUM_IMAGE_SIZE = 397_312
MAXIMUM_URL_BYTES = 511
MAXIMUM_FIRMWARE_HOST_BYTES = 127
MAXIMUM_FIRMWARE_PATH_BYTES = 255


def _encode_canonical_text_field(value: str) -> bytes:
    encoded = value.encode("utf-8")
    if not encoded or len(encoded) > 0xFFFF:
        raise ValueError("signed manifest text fields must be 1..65535 bytes")
    if any(byte < 0x20 or byte == 0x7F for byte in encoded):
        raise ValueError("signed manifest text fields must not contain controls")
    return struct.pack(">H", len(encoded)) + encoded


def canonical_manifest(manifest: dict[str, object]) -> bytes:
    """Encode the fields covered by the format-2 Ed25519 signature."""
    sha256 = bytes.fromhex(str(manifest["sha256"]))
    if len(sha256) != 32:
        raise ValueError("sha256 must contain 32 bytes")
    return b"".join(
        (
            b"WIO-OTA-MANIFEST-V2",
            struct.pack(">I", int(manifest["format"])),
            _encode_canonical_text_field(str(manifest["hardware"])),
            struct.pack(">I", int(manifest["version"])),
            _encode_canonical_text_field(str(manifest["url"])),
            struct.pack(">I", int(manifest["size"])),
            struct.pack(">H", int(str(manifest["crc16"]), 16)),
            sha256,
            _encode_canonical_text_field(str(manifest["release_id"])),
            struct.pack(">H", int(manifest["rollout"])),
            _encode_canonical_text_field(str(manifest["key_id"])),
        )
    )


def _validate_text(name: str, value: str, maximum_bytes: int) -> None:
    encoded = value.encode("utf-8")
    if not encoded or len(encoded) > maximum_bytes:
        raise ValueError(f"{name} must be 1..{maximum_bytes} UTF-8 bytes")
    if any(byte < 0x20 or byte == 0x7F for byte in encoded):
        raise ValueError(f"{name} must not contain control characters")


def validate_firmware_url(url: str) -> None:
    """Mirror the device parser's HTTP scheme, host, path, and port limits."""
    _validate_text("url", url, MAXIMUM_URL_BYTES)
    prefix = "http://"
    if not url.startswith(prefix):
        raise ValueError("url must use http://")
    authority_and_path = url[len(prefix):]
    slash = authority_and_path.find("/")
    if slash < 0:
        raise ValueError("url must include a firmware path")
    authority = authority_and_path[:slash]
    path = authority_and_path[slash:]
    host, separator, port_text = authority.partition(":")
    host_bytes = host.encode("utf-8")
    path_bytes = path.encode("utf-8")
    if not host_bytes or len(host_bytes) > MAXIMUM_FIRMWARE_HOST_BYTES:
        raise ValueError(
            f"url host must be 1..{MAXIMUM_FIRMWARE_HOST_BYTES} UTF-8 bytes"
        )
    if not path_bytes or len(path_bytes) > MAXIMUM_FIRMWARE_PATH_BYTES:
        raise ValueError(
            f"url path must be 1..{MAXIMUM_FIRMWARE_PATH_BYTES} UTF-8 bytes"
        )
    if separator:
        if not port_text.isascii() or not port_text.isdigit():
            raise ValueError("url port must contain decimal digits")
        port = int(port_text)
        if port < 1 or port > 0xFFFF:
            raise ValueError("url port must be in the range 1..65535")


def _write_outputs_atomically(outputs: list[tuple[pathlib.Path, bytes]]) -> None:
    """Stage every output, then replace destinations as one rollback group."""
    normalized = [path.absolute() for path, _ in outputs]
    if len(set(normalized)) != len(normalized):
        raise ValueError("--output and --firmware-output must be different paths")

    staged: dict[pathlib.Path, pathlib.Path] = {}
    backups: dict[pathlib.Path, pathlib.Path] = {}
    installed: list[pathlib.Path] = []
    committed = False
    try:
        for path, data in outputs:
            path.parent.mkdir(parents=True, exist_ok=True)
            descriptor, temporary_name = tempfile.mkstemp(
                prefix=f".{path.name}.", dir=path.parent
            )
            temporary_path = pathlib.Path(temporary_name)
            staged[path] = temporary_path
            with os.fdopen(descriptor, "wb") as stream:
                stream.write(data)
                stream.flush()
                os.fsync(stream.fileno())

        for path, _ in outputs:
            if path.exists():
                descriptor, backup_name = tempfile.mkstemp(
                    prefix=f".{path.name}.backup.", dir=path.parent
                )
                os.close(descriptor)
                backup_path = pathlib.Path(backup_name)
                backup_path.unlink()
                os.replace(path, backup_path)
                backups[path] = backup_path

        for path, _ in outputs:
            os.replace(staged[path], path)
            installed.append(path)
        committed = True
    except Exception as primary_error:
        for path in reversed(installed):
            path.unlink(missing_ok=True)
        restore_failures: list[str] = []
        for path, backup in backups.items():
            if backup.exists():
                try:
                    os.replace(backup, path)
                except OSError as restore_error:
                    restore_failures.append(
                        f"{backup} -> {path}: {restore_error}"
                    )
        if restore_failures:
            raise RuntimeError(
                "output rollback failed; preserved backups: "
                + "; ".join(restore_failures)
            ) from primary_error
        raise
    finally:
        for temporary_path in staged.values():
            temporary_path.unlink(missing_ok=True)
        if committed:
            for backup_path in backups.values():
                backup_path.unlink(missing_ok=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("firmware", type=pathlib.Path)
    parser.add_argument("--version", required=True, type=int)
    parser.add_argument("--url", required=True)
    parser.add_argument("--hardware", default="wio-bg770a-v1.0")
    parser.add_argument(
        "--signing-key",
        type=pathlib.Path,
        help="Ed25519 private key in PEM format; never commit this file",
    )
    parser.add_argument("--key-id")
    parser.add_argument("--release-id")
    parser.add_argument(
        "--rollout",
        type=int,
        default=10000,
        help="eligible share in basis points (0..10000)",
    )
    parser.add_argument(
        "--output",
        type=pathlib.Path,
        help="write JSON to this file instead of stdout",
    )
    parser.add_argument(
        "--firmware-output",
        type=pathlib.Path,
        help="also write the extracted raw firmware image to this path",
    )
    args = parser.parse_args()

    if args.version < 0 or args.version > 0xFFFFFFFF:
        parser.error("--version must be in the range 0..4294967295")
    if args.rollout < 0 or args.rollout > 10000:
        parser.error("--rollout must be in the range 0..10000")
    signing = args.signing_key is not None
    if signing and (not args.key_id or not args.release_id):
        parser.error("--signing-key requires --key-id and --release-id")
    if not signing and (args.key_id or args.release_id or args.rollout != 10000):
        parser.error(
            "--key-id, --release-id, and --rollout require --signing-key"
        )
    if args.firmware_output is not None and args.output is None:
        parser.error("--firmware-output requires --output for transactional output")

    image = read_firmware(args.firmware)
    if len(image) < 8 or len(image) > MAXIMUM_IMAGE_SIZE:
        parser.error(
            f"firmware size must be 8..{MAXIMUM_IMAGE_SIZE} bytes"
        )
    try:
        _validate_text("hardware", args.hardware, 63)
        validate_firmware_url(args.url)
        if signing:
            _validate_text("key-id", args.key_id, 31)
            _validate_text("release-id", args.release_id, 63)
    except ValueError as error:
        parser.error(str(error))
    crc16 = crc16_ccitt(image)
    if crc16 == 0:
        parser.error("firmware CRC16 is zero and cannot be represented")
    manifest = {
        "format": 2 if signing else 1,
        "hardware": args.hardware,
        "version": args.version,
        "url": args.url,
        "size": len(image),
        "crc16": f"{crc16:04x}",
        "sha256": hashlib.sha256(image).hexdigest(),
    }
    if signing:
        from cryptography.hazmat.primitives import serialization
        from cryptography.hazmat.primitives.asymmetric.ed25519 import (
            Ed25519PrivateKey,
        )

        try:
            private_key = serialization.load_pem_private_key(
                args.signing_key.read_bytes(), password=None
            )
        except (OSError, ValueError) as error:
            parser.error(f"unable to load --signing-key: {error}")
        if not isinstance(private_key, Ed25519PrivateKey):
            parser.error("--signing-key must contain an Ed25519 private key")
        manifest.update(
            {
                "release_id": args.release_id,
                "rollout": args.rollout,
                "key_id": args.key_id,
            }
        )
        manifest["signature"] = private_key.sign(
            canonical_manifest(manifest)
        ).hex()
    encoded = json.dumps(manifest, ensure_ascii=False, indent=2) + "\n"
    if args.output is None:
        sys.stdout.write(encoded)
    else:
        outputs = [(args.output, encoded.encode("utf-8"))]
        if args.firmware_output is not None:
            outputs.insert(0, (args.firmware_output, image))
        try:
            _write_outputs_atomically(outputs)
        except (OSError, ValueError) as error:
            parser.error(f"unable to write outputs: {error}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
