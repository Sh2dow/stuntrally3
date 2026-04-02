#!/usr/bin/env python3
"""
Generate a simple materials.ini manifest for an SR3 track folder.

This does not attempt to reconstruct Carbon shader semantics. It creates a
stable texture-to-material index that can be refined later by hand.

Usage:
    python tools/GenerateTrackMaterialsIni.py data/tracks/W2C
"""

from __future__ import annotations

import argparse
from pathlib import Path


def detect_blend_hint(name: str) -> str:
    upper = name.upper()
    if any(tag in upper for tag in ("GLASS", "WINDOW", "NEON", "LIGHT", "FLARE", "FENCE", "CHAINLINK", "GRATE", "LEAF", "TREE", "BANNER", "FLAG")):
        return "alpha"
    return "opaque"


def generate(track_dir: Path) -> Path:
    textures_dir = track_dir / "textures"
    if not textures_dir.is_dir():
        raise SystemExit(f"Missing textures directory: {textures_dir}")

    textures = sorted(p for p in textures_dir.iterdir() if p.is_file() and p.suffix.lower() in {".png", ".jpg", ".jpeg", ".dds"})
    if not textures:
        raise SystemExit(f"No textures found in: {textures_dir}")

    out_path = track_dir / "materials.ini"
    lines: list[str] = []
    lines.append("[materials]")
    lines.append(f"count = {len(textures)}")
    lines.append("default_blend = opaque")

    for idx, tex in enumerate(textures, start=1):
        lines.append("")
        lines.append(f"[mat_{idx:04d}]")
        lines.append(f"name = {tex.stem}")
        lines.append(f"texture = textures/{tex.name}")
        lines.append(f"blend = {detect_blend_hint(tex.stem)}")
        lines.append("flags = 0")

    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return out_path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("track_dir", type=Path, help="Track directory, e.g. data/tracks/W2C")
    args = parser.parse_args()

    out = generate(args.track_dir)
    print(f"Generated {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
