#!/usr/bin/env python3
"""Export an Ed25519 PEM public key as a C++ header for WioOtaAgent."""

from __future__ import annotations

import argparse
import pathlib

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ed25519 import (
    Ed25519PrivateKey,
    Ed25519PublicKey,
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("key", type=pathlib.Path)
    parser.add_argument("--key-id", required=True)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()

    encoded_key_id = args.key_id.encode("utf-8")
    if not encoded_key_id or len(encoded_key_id) > 31 or any(
        byte < 0x20 or byte == 0x7F for byte in encoded_key_id
    ):
        parser.error("--key-id must be 1..31 UTF-8 bytes without controls")

    # JSON's UTF-16 surrogate escapes are not valid C++ universal characters.
    # Three-digit octal escapes preserve each UTF-8 byte and cannot consume
    # a following digit as part of the same escape.
    key_id_literal = '"' + "".join(f"\\{byte:03o}" for byte in encoded_key_id) + '"'

    key_data = args.key.read_bytes()
    try:
        key = serialization.load_pem_public_key(key_data)
    except ValueError:
        key = serialization.load_pem_private_key(key_data, password=None)
    if isinstance(key, Ed25519PrivateKey):
        public_key = key.public_key()
    elif isinstance(key, Ed25519PublicKey):
        public_key = key
    else:
        parser.error("key must be an Ed25519 public or private PEM key")

    raw = public_key.public_bytes(
        encoding=serialization.Encoding.Raw,
        format=serialization.PublicFormat.Raw,
    )
    rows = []
    for offset in range(0, len(raw), 8):
        rows.append(
            "    " + ", ".join(f"0x{value:02x}" for value in raw[offset : offset + 8])
        )
    encoded = (
        "#pragma once\n\n"
        "#include <stdint.h>\n\n"
        "namespace wio_ota_keys {\n"
        f"constexpr char kManifestKeyId[] = {key_id_literal};\n"
        "constexpr uint8_t kManifestPublicKey[32] = {\n"
        + ",\n".join(rows)
        + "\n};\n"
        "}  // namespace wio_ota_keys\n"
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(encoded, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
