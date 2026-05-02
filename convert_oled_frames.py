from PIL import Image, ImageSequence, ImageOps
import argparse
import os
import glob

def load_frames(input_path):
    if os.path.isdir(input_path):
        files = sorted(
            glob.glob(os.path.join(input_path, "*.png")) +
            glob.glob(os.path.join(input_path, "*.jpg")) +
            glob.glob(os.path.join(input_path, "*.jpeg"))
        )

        if not files:
            raise ValueError("No image files found in folder.")

        return [Image.open(f).convert("RGBA") for f in files]

    img = Image.open(input_path)

    if getattr(img, "is_animated", False):
        return [frame.convert("RGBA") for frame in ImageSequence.Iterator(img)]

    return [img.convert("RGBA")]


def prepare_frame(img, width, height, threshold, invert):
    background = Image.new("RGBA", img.size, (0, 0, 0, 255))
    background.alpha_composite(img)

    gray = ImageOps.grayscale(background)

    gray.thumbnail((width, height), Image.Resampling.LANCZOS)

    canvas = Image.new("L", (width, height), 0)
    x = (width - gray.width) // 2
    y = (height - gray.height) // 2
    canvas.paste(gray, (x, y))

    if invert:
        canvas = ImageOps.invert(canvas)

    bw = canvas.point(lambda p: 255 if p > threshold else 0, mode="1")

    return bw


def pack_bitmap(img):
    width, height = img.size
    pixels = img.load()
    data = []

    for y in range(height):
        byte = 0
        bit_count = 0

        for x in range(width):
            pixel_on = pixels[x, y] != 0

            byte <<= 1
            if pixel_on:
                byte |= 1

            bit_count += 1

            if bit_count == 8:
                data.append(byte)
                byte = 0
                bit_count = 0

        if bit_count > 0:
            byte <<= (8 - bit_count)
            data.append(byte)

    return data


def write_header(frames, width, height, output_file):
    frame_bytes = len(frames[0])

    with open(output_file, "w") as f:
        f.write("#pragma once\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"#define FRAME_WIDTH {width}\n")
        f.write(f"#define FRAME_HEIGHT {height}\n")
        f.write(f"#define FRAME_COUNT {len(frames)}\n")
        f.write(f"#define FRAME_BYTES {frame_bytes}\n\n")

        f.write("const uint8_t PROGMEM frames[FRAME_COUNT][FRAME_BYTES] = {\n")

        for frame in frames:
            f.write("  {\n    ")

            for i, byte in enumerate(frame):
                f.write(f"0x{byte:02X}")

                if i != len(frame) - 1:
                    f.write(", ")

                if (i + 1) % 16 == 0:
                    f.write("\n    ")

            f.write("\n  },\n")

        f.write("};\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="Input GIF file or folder of PNG frames")
    parser.add_argument("--width", type=int, default=128)
    parser.add_argument("--height", type=int, default=64)
    parser.add_argument("--threshold", type=int, default=120)
    parser.add_argument("--invert", action="store_true")
    parser.add_argument("--output", default="frames.h")

    args = parser.parse_args()

    raw_frames = load_frames(args.input)

    packed_frames = []

    for frame in raw_frames:
        prepared = prepare_frame(
            frame,
            args.width,
            args.height,
            args.threshold,
            args.invert
        )

        packed_frames.append(pack_bitmap(prepared))

    write_header(packed_frames, args.width, args.height, args.output)

    print(f"Created {args.output}")
    print(f"Frames: {len(packed_frames)}")
    print(f"Bytes per frame: {len(packed_frames[0])}")
    print(f"Total animation size: {len(packed_frames) * len(packed_frames[0])} bytes")


if __name__ == "__main__":
    main()