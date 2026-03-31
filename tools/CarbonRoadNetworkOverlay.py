#!/usr/bin/env python3
"""
Blender-side overlay tool for Carbon validation/merged packs.

Opens an existing .blend pack, imports WRoadNetwork.obj into a dedicated
collection, optionally applies a translation, and saves a new .blend.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import bpy
from mathutils import Vector


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1 :]
    else:
        argv = []

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--in-blend", required=True)
    parser.add_argument("--road-obj", required=True)
    parser.add_argument("--out-blend", required=True)
    parser.add_argument("--translate", nargs=3, type=float, default=(0.0, 0.0, 0.0))
    parser.add_argument("--collection-name", default="Carbon_RoadNetwork")
    return parser.parse_args(argv)


def ensure_collection(name: str) -> bpy.types.Collection:
    collection = bpy.data.collections.get(name)
    if collection is None:
        collection = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(collection)
    return collection


def move_objects_to_collection(objects: list[bpy.types.Object], collection: bpy.types.Collection) -> None:
    for obj in objects:
        for prev in list(obj.users_collection):
            prev.objects.unlink(obj)
        if collection.objects.get(obj.name) is None:
            collection.objects.link(obj)


def main() -> int:
    args = parse_args()
    in_blend = Path(args.in_blend)
    road_obj = Path(args.road_obj)
    out_blend = Path(args.out_blend)

    if not in_blend.exists():
        raise SystemExit(f"Missing input blend: {in_blend}")
    if not road_obj.exists():
        raise SystemExit(f"Missing road OBJ: {road_obj}")

    bpy.ops.wm.open_mainfile(filepath=str(in_blend))

    before = {obj.name for obj in bpy.data.objects}
    bpy.ops.wm.obj_import(filepath=str(road_obj), forward_axis="Y", up_axis="Z")
    imported = [obj for obj in bpy.data.objects if obj.name not in before]

    road_collection = ensure_collection(args.collection_name)
    move_objects_to_collection(imported, road_collection)

    offset = Vector((float(args.translate[0]), float(args.translate[1]), float(args.translate[2])))
    for obj in imported:
        obj.location += offset
        obj["carbon_overlay_type"] = "road_network"
        obj["carbon_overlay_source"] = str(road_obj)

    bpy.context.scene["carbon_road_overlay_obj"] = str(road_obj)
    bpy.context.scene["carbon_road_overlay_translate"] = ",".join(f"{v:.6f}" for v in offset)

    out_blend.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(out_blend))
    print(f"Saved road overlay blend: {out_blend}")
    print(f"Imported road objects: {len(imported)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
