from pathlib import Path
import struct
import zlib
import binascii
import sys

WIDTH = 128
HEIGHT = 64
BUFFER_SIZE = WIDTH * HEIGHT // 8
SCALE = 4

DEFAULT_INPUT = Path("simulation_logs/oled_framebuffer.bin")
DEFAULT_OUTPUT = Path("simulation_logs/oled_framebuffer.png")


def png_chunk(chunk_type, data):
    chunk = chunk_type + data
    crc = binascii.crc32(chunk) & 0xFFFFFFFF
    return (
        struct.pack(">I", len(data)) +
        chunk_type +
        data +
        struct.pack(">I", crc)
    )


def write_png(path, width, height, rgb_bytes):
    raw = bytearray()

    for y in range(height):
        raw.append(0)
        row_start = y * width * 3
        raw.extend(rgb_bytes[row_start:row_start + width * 3])

    png = bytearray()
    png.extend(b"\x89PNG\r\n\x1a\n")

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png.extend(png_chunk(b"IHDR", ihdr))
    png.extend(png_chunk(b"IDAT", zlib.compress(bytes(raw))))
    png.extend(png_chunk(b"IEND", b""))

    path.write_bytes(png)


def main():
    input_path = Path(sys.argv[1]) if len(sys.argv) >= 2 else DEFAULT_INPUT
    output_path = Path(sys.argv[2]) if len(sys.argv) >= 3 else DEFAULT_OUTPUT

    data = input_path.read_bytes()

    if len(data) != BUFFER_SIZE:
        raise ValueError(f"Buffer deve ter {BUFFER_SIZE} bytes, mas tem {len(data)}")

    out_width = WIDTH * SCALE
    out_height = HEIGHT * SCALE
    rgb = bytearray(out_width * out_height * 3)

    for y in range(out_height):
        src_y = y // SCALE

        for x in range(out_width):
            src_x = x // SCALE

            page = src_y // 8
            bit = src_y % 8
            index = src_x + page * WIDTH

            pixel_on = (data[index] >> bit) & 0x01
            color = 255 if pixel_on else 0

            pos = (y * out_width + x) * 3
            rgb[pos + 0] = color
            rgb[pos + 1] = color
            rgb[pos + 2] = color

    output_path.parent.mkdir(exist_ok=True)
    write_png(output_path, out_width, out_height, rgb)

    print(f"Imagem gerada: {output_path}")


if __name__ == "__main__":
    main()
