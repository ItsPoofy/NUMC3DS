"""Minimal encoder for the BJSON format used by MC3DS font metadata."""

from __future__ import annotations

import io
import struct


def joaat(text: str) -> int:
    value = 0
    for byte in text.lower().encode("utf-8"):
        value = (value + byte) & 0xFFFFFFFF
        value = (value + (value << 10)) & 0xFFFFFFFF
        value ^= value >> 6
    value = (value + (value << 3)) & 0xFFFFFFFF
    value ^= value >> 11
    return (value + (value << 15)) & 0xFFFFFFFF


def encode_bjson(document: object) -> bytes:
    structures: list[list[object]] = []
    strings = bytearray()
    array_indexes: list[int] = []
    headers: list[tuple[int, int, int]] = []
    header_strings = bytearray()
    item_index = 0
    object_length = 0
    array_length = 0

    def append_value(value: object) -> None:
        nonlocal item_index
        if value is None:
            structures.append([0, 0, 0])
        elif type(value) is bool:
            structures.append([1, int(value), 0])
        elif type(value) is int:
            structures.append([2, value, 0])
        elif type(value) is float:
            structures.append([3, value, 0])
        elif type(value) is str:
            structures.append([5, joaat(value), len(strings)])
            strings.extend(value.encode("utf-8") + b"\0")
        elif type(value) is list:
            structures.append([4, len(value), 0])
            append_list(value)
        elif type(value) is dict:
            structures.append([6, len(value), 0])
            append_object(value)
        else:
            raise TypeError(f"Unsupported BJSON value: {type(value).__name__}")

    def append_object(value: dict[str, object]) -> None:
        nonlocal item_index, object_length
        local_index = item_index
        local_headers: list[tuple[int, int, int]] = []
        for key, child in value.items():
            item_index += 1
            local_headers.append((joaat(key), len(header_strings), item_index))
            header_strings.extend(key.encode("utf-8") + b"\0")
            append_value(child)
        structures[local_index][2] = object_length if value else 0
        object_length += len(value)
        headers.extend(sorted(local_headers))

    def append_list(value: list[object]) -> None:
        nonlocal item_index, array_length
        local_index = item_index
        local_indexes: list[int] = []
        for child in value:
            item_index += 1
            local_indexes.append(item_index)
            append_value(child)
        structures[local_index][2] = array_length if value else 0
        array_length += len(value)
        array_indexes.extend(local_indexes)

    if type(document) is dict:
        structures.append([6, len(document), 0])
        append_object(document)
    elif type(document) is list:
        structures.append([4, len(document), 0])
        append_list(document)
    else:
        raise TypeError("BJSON root must be an object or array")

    output = io.BytesIO()
    output.write(struct.pack("<I", len(structures)))
    for data_type, value1, value2 in structures:
        output.write(struct.pack("<I", data_type))
        if data_type == 2:
            output.write(struct.pack("<iI", value1, 0))
        elif data_type == 3:
            output.write(struct.pack("<fI", value1, 0))
        else:
            output.write(struct.pack("<II", value1, value2))
    output.write(struct.pack("<I", len(strings)))
    output.write(strings)
    output.write(struct.pack("<I", len(array_indexes)))
    for index in array_indexes:
        output.write(struct.pack("<I", index))
    output.write(struct.pack("<I", len(headers)))
    for entry in headers:
        output.write(struct.pack("<III", *entry))
    output.write(struct.pack("<I", len(header_strings)))
    output.write(header_strings)
    return output.getvalue()
