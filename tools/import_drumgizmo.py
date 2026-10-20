#!/usr/bin/env python3
"""Turn a DrumGizmo instrument into the plugin's five velocity layers.

DrumGizmo records every hit on all microphones (one multichannel WAV per hit,
each with a "power" value). This mixes the chosen microphones down to mono,
picks five hits spread from soft to hard, trims silence and writes them as
<Prefix>-Softest.wav ... <Prefix>-Hardest.wav (24-bit mono).

Example:
  tools/import_drumgizmo.py MuldjordKit3/KdrumL/KdrumL.xml \\
      --mix KdrumL=1.0,OHL=0.3,OHR=0.3,AmbL=0.2,AmbR=0.2 \\
      --prefix Kick --out resources/kits/GMRockKit
"""

import argparse
import os
import struct
import sys
import wave
import xml.etree.ElementTree as ET

import numpy as np

LAYER_NAMES = ["Softest", "Soft", "Med", "Hard", "Hardest"]
# Loudness of each layer relative to the hardest hit, in dB.
LAYER_TARGETS_DB = [-18.0, -12.0, -7.0, -3.0, 0.0]


def read_wav(path):
    """Read a PCM or IEEE float WAV into a (frames, channels) float array."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError(f"{path}: not a WAV file")

    pos, fmt, samples = 12, None, None
    while pos + 8 <= len(data):
        chunk_id, size = data[pos:pos + 4], struct.unpack("<I", data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + size]
        if chunk_id == b"fmt ":
            fmt = struct.unpack("<HHIIHH", body[:16])
            if fmt[0] == 0xFFFE and len(body) >= 26:  # WAVE_FORMAT_EXTENSIBLE
                fmt = (struct.unpack("<H", body[24:26])[0],) + fmt[1:]
        elif chunk_id == b"data":
            samples = body
        pos += 8 + size + (size & 1)

    if fmt is None or samples is None:
        raise ValueError(f"{path}: missing fmt or data chunk")

    format_tag, channels, rate, _, _, bits = fmt
    if format_tag == 3 and bits == 32:
        audio = np.frombuffer(samples, dtype="<f4")
    elif format_tag == 1 and bits == 16:
        audio = np.frombuffer(samples, dtype="<i2") / 32768.0
    elif format_tag == 1 and bits == 24:
        raw = np.frombuffer(samples, dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        ints = raw[:, 0] | (raw[:, 1] << 8) | (raw[:, 2] << 16)
        audio = np.where(ints & 0x800000, ints - (1 << 24), ints) / 8388608.0
    else:
        raise ValueError(f"{path}: unsupported format {format_tag}/{bits} bit")

    frames = len(audio) // channels
    return audio[: frames * channels].reshape(frames, channels).astype(np.float64), rate


def write_wav24(path, audio, rate):
    ints = np.clip(np.round(audio * 8388607.0), -8388608, 8388607).astype(np.int32)
    raw = np.stack([ints & 0xFF, (ints >> 8) & 0xFF, (ints >> 16) & 0xFF], axis=1).astype(np.uint8)
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(3)
        w.setframerate(rate)
        w.writeframes(raw.tobytes())


def load_instrument(xml_path):
    root = ET.parse(xml_path).getroot()
    base = os.path.dirname(xml_path)
    hits = []
    for sample in root.iter("sample"):
        channels = {}
        for af in sample.iter("audiofile"):
            channels[af.get("channel")] = (os.path.join(base, af.get("file")), int(af.get("filechannel")) - 1)
        hits.append((float(sample.get("power")), sample.get("name"), channels))
    hits.sort(key=lambda h: h[0])
    return hits


def mixdown(channels, mix):
    out, rate = None, None
    cache = {}
    for name, gain in mix.items():
        if name not in channels:
            raise ValueError(f"microphone {name!r} not found; available: {', '.join(channels)}")
        path, index = channels[name]
        if path not in cache:
            cache[path] = read_wav(path)
        audio, rate = cache[path]
        signal = audio[:, index] * gain
        out = signal if out is None else out + signal
    return out, rate


def trim(audio, rate, floor_db=-60.0, max_seconds=2.5, fade_seconds=0.05):
    peak = np.max(np.abs(audio))
    threshold = peak * 10 ** (floor_db / 20.0)

    loud = np.nonzero(np.abs(audio) > peak * 0.02)[0]
    start = max(0, loud[0] - int(0.002 * rate)) if len(loud) else 0

    above = np.nonzero(np.abs(audio) > threshold)[0]
    end = above[-1] + 1 if len(above) else len(audio)
    end = min(end, start + int(max_seconds * rate))

    out = audio[start:end].copy()
    fade = min(len(out), int(fade_seconds * rate))
    out[-fade:] *= np.linspace(1.0, 0.0, fade)
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("instrument", help="DrumGizmo instrument XML")
    parser.add_argument("--mix", required=True, help="mic=gain list, e.g. KdrumL=1,OHL=0.3,OHR=0.3")
    parser.add_argument("--prefix", required=True, help="output name prefix, e.g. Kick")
    parser.add_argument("--out", required=True, help="output directory")
    parser.add_argument("--peak", type=float, default=0.95, help="peak level of the hardest layer")
    args = parser.parse_args()

    mix = {}
    for item in args.mix.split(","):
        name, gain = item.split("=")
        mix[name.strip()] = float(gain)

    hits = load_instrument(args.instrument)
    if len(hits) < len(LAYER_NAMES):
        sys.exit(f"only {len(hits)} hits, need at least {len(LAYER_NAMES)}")

    loudest = hits[-1][0]
    chosen = []
    for target_db in LAYER_TARGETS_DB:
        target = loudest * 10 ** (target_db / 20.0)
        candidates = [h for h in hits if h not in chosen]
        chosen.append(min(candidates, key=lambda h: abs(h[0] - target)))
    chosen.sort(key=lambda h: h[0])

    layers, rate = [], None
    for power, name, channels in chosen:
        audio, rate = mixdown(channels, mix)
        layers.append((name, power, trim(audio, rate)))

    # One gain for all layers so their natural dynamics are kept.
    scale = args.peak / max(np.max(np.abs(a)) for _, _, a in layers)

    os.makedirs(args.out, exist_ok=True)
    for layer_name, (name, power, audio) in zip(LAYER_NAMES, layers):
        path = os.path.join(args.out, f"{args.prefix}-{layer_name}.wav")
        audio = audio * scale
        write_wav24(path, audio, rate)
        print(f"{os.path.basename(path):22s} <- {name:12s} power {power:7.3f}  "
              f"peak {np.max(np.abs(audio)):.3f}  {len(audio) / rate:.2f}s")


if __name__ == "__main__":
    main()
