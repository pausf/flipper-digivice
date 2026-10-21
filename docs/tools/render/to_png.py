"""Turns the renderer's raw frames into PNGs and GIFs (standard library only).

Input: <frames-dir>/<name>.f, a run of 128x64 frames (one byte per pixel, 1 = lit).
  shot-*   one frame      -> <name>.png
  anim-*   many frames    -> <name>.gif, plus <name>.png of a middle frame
  sheet-*  several panels -> <name>.png, panels tiled two per row
Images use the Flipper LCD palette inside a small dark bezel, scaled 4x (3x for GIFs).
"""
import os
import struct
import sys
import zlib

W, H = 128, 64
ON = (0x2E, 0x18, 0x06)
OFF = (0xFF, 0x8E, 0x22)
BEZEL = (0x1B, 0x1B, 0x1F)
PAD = 10


def paint(frame, scale):
    """Rows of palette indices (0 bezel, 1 off, 2 on) for one frame."""
    width = W * scale + 2 * PAD
    rows = [bytes(width)] * PAD
    for y in range(H):
        line = bytearray(width)
        for x in range(W):
            v = 2 if frame[y * W + x] else 1
            line[PAD + x * scale : PAD + (x + 1) * scale] = bytes([v]) * scale
        rows.extend([bytes(line)] * scale)
    rows.extend([bytes(width)] * PAD)
    return width, rows


def png(path, width, rows):
    raw = b"".join(b"\x00" + bytes(r) for r in rows)

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    plte = bytes(BEZEL) + bytes(OFF) + bytes(ON)
    data = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, len(rows), 8, 3, 0, 0, 0))
        + chunk(b"PLTE", plte)
        + chunk(b"IDAT", zlib.compress(raw, 9))
        + chunk(b"IEND", b"")
    )
    with open(path, "wb") as f:
        f.write(data)


def lzw(indices, min_bits=2):
    """GIF LZW of a byte sequence of palette indices."""
    clear = 1 << min_bits
    eoi = clear + 1
    out = bytearray()
    acc = nbits = 0

    def put(code, size):
        nonlocal acc, nbits
        acc |= code << nbits
        nbits += size
        while nbits >= 8:
            out.append(acc & 0xFF)
            acc >>= 8
            nbits -= 8

    table = {}
    next_code = eoi + 1
    size = min_bits + 1
    put(clear, size)
    prefix = None
    for b in indices:
        if prefix is None:
            prefix = b
            continue
        key = (prefix, b)
        if key in table:
            prefix = table[key]
            continue
        put(prefix, size)
        if next_code < 4096:
            table[key] = next_code
            if next_code == (1 << size) and size < 12:
                size += 1
            next_code += 1
        else:
            put(clear, size)
            table = {}
            next_code = eoi + 1
            size = min_bits + 1
        prefix = b
    put(prefix, size)
    put(eoi, size)
    if nbits:
        out.append(acc & 0xFF)
    return bytes(out)


def gif(path, frames, scale, delay_cs=10):
    width, rows = paint(frames[0], scale)
    height = len(rows)
    out = bytearray(b"GIF89a")
    out += struct.pack("<HHBBB", width, height, 0xF1, 0, 0)  # 4-entry global palette
    out += bytes(BEZEL) + bytes(OFF) + bytes(ON) + bytes(3)
    out += b"\x21\xff\x0bNETSCAPE2.0\x03\x01\x00\x00\x00"  # loop forever
    for frame in frames:
        _, rows = paint(frame, scale)
        out += b"\x21\xf9\x04\x00" + struct.pack("<H", delay_cs) + b"\x00\x00"
        out += b"\x2c" + struct.pack("<HHHHB", 0, 0, width, height, 0)
        out += bytes([2])
        data = lzw(b"".join(rows))
        for i in range(0, len(data), 255):
            blk = data[i : i + 255]
            out += bytes([len(blk)]) + blk
        out += b"\x00"
    out += b"\x3b"
    with open(path, "wb") as f:
        f.write(out)


def main(src, dst):
    os.makedirs(dst, exist_ok=True)
    for fn in sorted(os.listdir(src)):
        if not fn.endswith(".f"):
            continue
        name = fn[:-2]
        data = open(os.path.join(src, fn), "rb").read()
        frames = [data[i : i + W * H] for i in range(0, len(data), W * H)]
        if not frames:
            continue
        if name.startswith("anim-"):
            gif(os.path.join(dst, name + ".gif"), frames, 3)
            w, rows = paint(frames[len(frames) // 2], 4)
            png(os.path.join(dst, name + ".png"), w, rows)
        elif name.startswith("sheet-"):
            cols = 2 if len(frames) > 1 else 1
            tiles = [paint(f, 4) for f in frames]
            sheet_w = cols * tiles[0][0]
            rows_out = []
            for r in range(0, len(tiles), cols):
                group = tiles[r : r + cols]
                for y in range(len(group[0][1])):
                    line = b"".join(t[1][y] for t in group)
                    rows_out.append(line + bytes(sheet_w - len(line)))
            png(os.path.join(dst, name + ".png"), sheet_w, rows_out)
        else:
            w, rows = paint(frames[0], 4)
            png(os.path.join(dst, name + ".png"), w, rows)
        print(name, len(frames))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
