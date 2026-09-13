#!/usr/bin/env python3
"""Register custom resources in the stock MC3DS resource hash index."""

from __future__ import annotations

import argparse
import os
import struct
from pathlib import Path


UINT32_MASK = 0xFFFFFFFF


def resource_hash(value: str) -> int:
    """Match the game's case-folded 32-bit resource-path hash."""

    hash_value = 0
    for byte in value.encode("ascii"):
        if 0x41 <= byte <= 0x5A:
            byte += 0x20
        hash_value = (hash_value + byte) & UINT32_MASK
        hash_value = (hash_value + (hash_value << 10)) & UINT32_MASK
        hash_value ^= hash_value >> 6
    hash_value = (hash_value + (hash_value << 3)) & UINT32_MASK
    hash_value ^= hash_value >> 11
    hash_value = (hash_value + (hash_value << 15)) & UINT32_MASK
    return hash_value


def resource_variants(resource: str) -> list[str]:
    """Query forms the game looks up: bare, .png (texture query form), and .3dst (file form).

    The stock index carries both the .png query hash and the .3dst file hash for every
    texture (e.g. textures/gui/gui.png and textures/gui/gui.3dst); the loader rewrites
    .png to .3dst before opening.  An extension-less entry alone is never queried.
    """

    base = resource.strip("/")
    stem, extension = os.path.splitext(base)
    if extension:
        return [base]
    return [base, base + ".png", base + ".3dst"]


def read_index(path: Path) -> list[int]:
    data = path.read_bytes()
    if len(data) < 4:
        raise ValueError(f"Resource index is truncated: {path}")
    count = struct.unpack_from("<I", data, 0)[0]
    expected_length = 4 + count * 4
    if len(data) != expected_length:
        raise ValueError(
            f"Resource index has invalid length: count={count}, "
            f"length={len(data)}, expected={expected_length}"
        )
    hashes = list(struct.unpack_from(f"<{count}I", data, 4))
    if hashes != sorted(hashes) or len(hashes) != len(set(hashes)):
        raise ValueError("Resource index must contain sorted unique hashes")
    return hashes


def write_index(path: Path, hashes: list[int]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(struct.pack("<I", len(hashes)) + struct.pack(f"<{len(hashes)}I", *hashes))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("resources", nargs="+")
    args = parser.parse_args()

    hashes = read_index(args.input)
    known = set(hashes)
    registered: list[tuple[str, int]] = []
    for resource in args.resources:
        for variant in resource_variants(resource):
            digest = resource_hash(variant)
            if digest not in known:
                hashes.append(digest)
                known.add(digest)
                registered.append((variant, digest))
    hashes.sort()
    write_index(args.output, hashes)

    for resource, digest in registered:
        print(f"Registered resource {resource} (0x{digest:08X})")
    print(f"Resource index: {len(hashes)} entries ({len(registered)} added)")


if __name__ == "__main__":
    main()
