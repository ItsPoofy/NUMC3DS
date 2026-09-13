#!/usr/bin/env python3
"""Validate the release payload that Luma LayeredFS will resolve."""

from __future__ import annotations

import argparse
import hashlib
import json
import runpy
from font_bjson import encode_bjson
import struct
from pathlib import Path


SHADERS = (
    "cloud.shbin",
    "entity_color_based_use_uv_anim.shbin",
    "entity_colorbased_no_texture.shbin",
    "entity_item_in_hand.shbin",
    "entity_overlay.shbin",
    "entity.shbin",
    "rain_snow.shbin",
    "renderchunk_as_entity.shbin",
    "renderchunk_near_water.shbin",
    "renderchunk_seasons.shbin",
    "renderchunk.shbin",
    "weather.shbin",
)


def fail(message: str) -> None:
    raise ValueError(message)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def apply_ips(stock: bytes, patch: bytes) -> bytes:
    if not patch.startswith(b"PATCH"):
        fail("release code.ips has an invalid header")
    output = bytearray(stock)
    cursor = 5
    while True:
        if cursor + 3 > len(patch):
            fail("release code.ips is truncated")
        if patch[cursor:cursor + 3] == b"EOF":
            cursor += 3
            break
        offset = int.from_bytes(patch[cursor:cursor + 3], "big")
        cursor += 3
        if cursor + 2 > len(patch):
            fail("release code.ips has a truncated record")
        length = int.from_bytes(patch[cursor:cursor + 2], "big")
        cursor += 2
        if length:
            if cursor + length > len(patch) or offset + length > len(output):
                fail("release code.ips record is out of bounds")
            output[offset:offset + length] = patch[cursor:cursor + length]
            cursor += length
        else:
            if cursor + 3 > len(patch):
                fail("release code.ips has a truncated RLE record")
            run_length = int.from_bytes(patch[cursor:cursor + 2], "big")
            value = patch[cursor + 2]
            cursor += 3
            if offset + run_length > len(output):
                fail("release code.ips RLE record is out of bounds")
            output[offset:offset + run_length] = bytes((value,)) * run_length
    if cursor != len(patch):
        fail("release code.ips has unsupported trailing data")
    return bytes(output)


def validate_luma_layeredfs_symbols(code: bytes) -> None:
    text_size = 0x818824
    words = struct.unpack_from(f"<{text_size // 4}I", code)
    found: dict[str, int] = {}

    def function_start(index: int) -> int | None:
        while index >= 1:
            index -= 1
            if words[index] >> 16 == 0xE92D:
                return index * 4
        return None

    for index, word in enumerate(words):
        remaining = len(words) - index
        name = None
        if word == 0xE5970010 and remaining >= 3 and "mount" not in found:
            if words[index + 1] == 0xE1CD20D8 and words[index + 2] & 0xFFFFFF == 0x008D0000:
                name = "mount"
        elif word == 0xE24DD028 and remaining >= 4 and "mount" not in found:
            if words[index + 1:index + 4] == (0xE1A04000, 0xE59F60A8, 0xE3A0C001):
                name = "mount"
        elif word == 0xE2844001 and remaining >= 3 and "unmount" not in found:
            if words[index + 1:index + 3] == (0xE3540020, 0x3AFFFFF0):
                name = "unmount"
        elif word == 0xE353003A and remaining >= 3 and "unmount" not in found:
            if words[index + 1] & 0xFFFFFF0F == 0x0A000009 and words[index + 2] & 0xFFFF0FF0 == 0xE1A00400:
                name = "unmount"
        elif word == 0xE3500008 and remaining >= 3 and "register" not in found:
            if words[index + 1] & 0xFFF00FF0 == 0xE1800400 and words[index + 2] & 0xFFF00FF0 == 0xE1800FC0:
                name = "register"
        elif word == 0xE351003A and remaining >= 16 and "try_open" not in found:
            if words[index + 1] == 0x1AFFFFFC and words[index + 13] == 0xE590C000 and words[index + 15] == 0xE12FFF3C:
                name = "try_open"
        elif word == 0x08030204 and "open_direct" not in found:
            name = "open_direct"
        if name is not None:
            start = function_start(index)
            if start is not None:
                found[name] = start
        if len(found) == 5:
            break
    missing = sorted({"mount", "unmount", "register", "try_open", "open_direct"} - found.keys())
    if missing:
        fail("release code does not satisfy Luma 13.4 LayeredFS symbol discovery: " + ", ".join(missing))
    if found["unmount"] != 0x11E10:
        fail(f"release Luma unmount symbol resolved to unexpected code offset 0x{found['unmount']:X}")


def validate_luma_layeredfs_reservation(code: bytes) -> None:
    payload_start = 0x818824
    payload_end = payload_start + 0x11C
    if any(code[payload_start:payload_end]):
        fail("release code overlaps Luma 13.4 LayeredFS payload reservation")


def validate_bjson(path: Path) -> None:
    data = path.read_bytes()
    if len(data) < 4:
        fail(f"truncated BJSON: {path}")
    values = struct.unpack_from("<I", data)[0]
    values_end = 4 + values * 12
    if values_end + 4 > len(data):
        fail(f"invalid BJSON value table: {path}")
    strings_size = struct.unpack_from("<I", data, values_end)[0]
    strings_end = values_end + 4 + strings_size
    if strings_end + 4 > len(data):
        fail(f"invalid BJSON string table: {path}")
    arrays = struct.unpack_from("<I", data, strings_end)[0]
    arrays_end = strings_end + 4 + arrays * 4
    if arrays_end + 4 > len(data):
        fail(f"invalid BJSON array table: {path}")
    headers = struct.unpack_from("<I", data, arrays_end)[0]
    headers_end = arrays_end + 4 + headers * 12
    if headers_end + 4 > len(data):
        fail(f"invalid BJSON header table: {path}")
    names = struct.unpack_from("<I", data, headers_end)[0]
    if headers_end + 4 + names != len(data):
        fail(f"invalid BJSON name table: {path}")


def validate_3dst(path: Path) -> None:
    data = path.read_bytes()
    if len(data) < 32 or data[:4] != b"3DST":
        fail(f"invalid 3DST signature: {path}")
    version, mode, width, height, stored_width, stored_height, mip_count = struct.unpack_from("<7I", data, 4)
    if version != 3 or mode != 0 or width == 0 or height == 0:
        fail(f"unsupported 3DST header: {path}")
    if (width, height) != (stored_width, stored_height) or mip_count != 1:
        fail(f"inconsistent 3DST dimensions: {path}")
    if width % 8 or height % 8 or len(data) != 32 + width * height * 4:
        fail(f"invalid 3DST payload length: {path}")


def validate_shader(path: Path) -> None:
    data = path.read_bytes()
    if len(data) < 16 or data[:4] != b"DVLB" or b"DVLP" not in data or b"DVLE" not in data:
        fail(f"invalid DVLB shader: {path}")
    programs = struct.unpack_from("<I", data, 4)[0]
    if programs == 0 or programs > 64:
        fail(f"invalid DVLB program count: {path}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("build_root", type=Path)
    parser.add_argument("--stock-code", type=Path, required=True)
    parser.add_argument("--stock-exheader", type=Path, required=True)
    parser.add_argument("--diagnostic-stage", choices=("module", "bootstrap", "layout"), default="module")
    parser.add_argument("--expected-native-size", type=lambda value: int(value, 0), default=0x80000)
    args = parser.parse_args()
    root = args.build_root.resolve()
    romfs = root / "romfs"

    required = (root / "code.ips", root / "exheader.bin", romfs / "uniforms.bjson")
    for path in required:
        if not path.is_file():
            fail(f"missing release payload: {path}")
    if (romfs / "romfs").exists():
        fail("release payload contains an invalid nested romfs directory")
    if (root / "code.bin").exists():
        fail("release payload must not contain code.bin")
    code = apply_ips(args.stock_code.resolve().read_bytes(), (root / "code.ips").read_bytes())
    exheader = (root / "exheader.bin").read_bytes()
    if len(code) != 0x93A000 or len(exheader) != 0x400:
        fail("release executable or ExHeader has the wrong stock layout")
    data_address, data_pages, data_size, bss_size = struct.unpack_from("<4I", exheader, 0x30)
    if (data_address, data_pages, data_size, bss_size) != (0x00A29000, 0x11, 0x10B70, 0x1060C0):
        fail("release ExHeader does not preserve the stock code/data layout")
    jump_id = struct.unpack_from("<Q", exheader, 0x1C8)[0]
    program_id = struct.unpack_from("<Q", exheader, 0x200)[0]
    if jump_id != 0x0004000E001B8700 or program_id != 0x00040000001B8700:
        fail("release ExHeader does not preserve the update jump ID and base program ID")
    stock_exheader = args.stock_exheader.resolve().read_bytes()
    expected_exheader = bytearray(stock_exheader[:0x400])
    fs_access = struct.unpack_from("<I", expected_exheader, 0x248)[0] | (1 << 7)
    struct.pack_into("<I", expected_exheader, 0x248, fs_access)
    io_access = struct.unpack_from("<H", expected_exheader, 0x3F0)[0] | (1 << 9)
    struct.pack_into("<H", expected_exheader, 0x3F0, io_access)
    if exheader != expected_exheader:
        fail("release ExHeader_Info differs from stock outside the required DirectSdmc permissions")
    if fs_access & (1 << 3):
        fail("release ExHeader_Info enables the Debug filesystem permission")
    validate_luma_layeredfs_symbols(code)
    validate_luma_layeredfs_reservation(code)
    native = romfs / "numc3ds" / "native.bin"
    if args.diagnostic_stage == "module":
        if not native.is_file() or native.stat().st_size != args.expected_native_size or native.read_bytes()[:4] != b"NuMC":
            fail("release does not contain the expected runtime native module")
    elif native.exists():
        fail("non-module diagnostic unexpectedly contains a native module")
    for texture in sorted(romfs.rglob("*.3dst")):
        validate_3dst(texture)
    for name in SHADERS:
        validate_shader(romfs / "shaders" / "3DS" / name)
    for metadata in sorted(romfs.rglob("*.bjson")):
        validate_bjson(metadata)

    pack = Path("resourcepacks/vanilla/client")
    texture = "textures/gui/command_block_modes"
    validate_3dst(romfs / pack / (texture + ".3dst"))
    catalogue = json.loads((romfs / pack / "textures/textures_list.json").read_text())
    if texture not in catalogue:
        fail("texture catalogue does not contain the command block icons")
    if (romfs / pack / "textures/textures_list.bjson").read_bytes() != encode_bjson(catalogue):
        fail("binary texture catalogue differs from JSON")
    index = runpy.run_path(str(Path(__file__).with_name("Patch-ResourceIndex.py")))
    required_hashes = {index["resource_hash"](name) for name in index["resource_variants"](texture)}
    actual_hashes = set(index["read_index"](romfs / pack / "resindex.idx"))
    if not required_hashes.issubset(actual_hashes):
        fail("resource index does not contain the command block icons")

    forbidden = (
        romfs / "images" / "numc3ds" / "quick_commands.3dst",
        romfs / "numc3ds" / "quick_commands.3dst",
        romfs / "resourcepacks" / "vanilla" / "client" / "textures" / "numc3ds" / "quick_commands.3dst",
    )
    stale = [str(path.relative_to(root)) for path in forbidden if path.exists()]
    if stale:
        fail("release payload contains stale resource overrides: " + ", ".join(stale))

    files = [path for path in root.rglob("*") if path.is_file() and "objects" not in path.relative_to(root).parts]
    print(f"Validated {len(files)} release files, {len(list(romfs.rglob('*.3dst')))} textures, and {len(SHADERS)} shaders.")


if __name__ == "__main__":
    main()
