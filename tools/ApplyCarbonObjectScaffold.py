#!/usr/bin/env python3
"""
Apply a generated scene_objects_scaffold.xml block into an SR3 track scene.xml.
"""

from __future__ import annotations

import argparse
import shutil
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scene-xml", required=True)
    parser.add_argument("--objects-xml", required=True)
    parser.add_argument("--backup", action="store_true", default=True)
    parser.add_argument("--no-backup", dest="backup", action="store_false")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    scene_path = Path(args.scene_xml)
    objects_path = Path(args.objects_xml)
    if not scene_path.exists():
        raise SystemExit(f"Missing scene.xml: {scene_path}")
    if not objects_path.exists():
        raise SystemExit(f"Missing scaffold xml: {objects_path}")

    scene_tree = ET.parse(scene_path)
    scene_root = scene_tree.getroot()
    scaffold_root = ET.parse(objects_path).getroot()

    if scaffold_root.tag != "objects":
        raise SystemExit(f"Expected <objects> root in {objects_path}")

    existing = scene_root.find("objects")
    if existing is not None:
        scene_root.remove(existing)

    new_objects = ET.Element("objects")
    for child in list(scaffold_root):
        new_objects.append(child)
    scene_root.append(new_objects)

    if args.backup:
        backup_path = scene_path.with_suffix(scene_path.suffix + ".bak")
        shutil.copy2(scene_path, backup_path)

    ET.indent(scene_tree, space="    ")
    scene_tree.write(scene_path, encoding="utf-8", xml_declaration=False)
    print(f"Applied scaffold: {objects_path}")
    print(f"Updated scene: {scene_path}")
    if args.backup:
        print(f"Backup: {backup_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
