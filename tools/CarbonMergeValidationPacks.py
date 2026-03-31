#!/usr/bin/env python3
"""
Blender-side merger for parallel Carbon validation packs.

Builds one larger candidate slice from previously generated chunk packs.

Run from Blender:
    blender.exe --background --factory-startup --python CarbonMergeValidationPacks.py -- \
        --parallel-root D:\\Repos\\Games\\NFS-ModTools\\AssetDumper\\output\\L5RA_unified\\validation_packs_parallel\\sections_bridge \
        --pack-names sections_bridge_0001_1001_1083_50count sections_bridge_0002_1084_1147_50count \
        --out-dir D:\\Repos\\Games\\NFS-ModTools\\AssetDumper\\output\\L5RA_unified\\validation_merged\\slice_0001_0002 \
        --pack-name slice_0001_0002
"""

from __future__ import annotations

import argparse
import json
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
    parser.add_argument("--parallel-root", required=True)
    parser.add_argument("--pack-names", nargs="+", required=True)
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--pack-name", required=True)
    parser.add_argument("--skip-collada-export", action="store_true")
    return parser.parse_args(argv)


def clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)

    for collection in list(bpy.data.collections):
        if collection.users == 0:
            bpy.data.collections.remove(collection)

    for datablock_collection in (
        bpy.data.meshes,
        bpy.data.materials,
        bpy.data.images,
        bpy.data.cameras,
        bpy.data.lights,
        bpy.data.curves,
        bpy.data.armatures,
    ):
        for datablock in list(datablock_collection):
            if datablock.users == 0:
                datablock_collection.remove(datablock)


def ensure_collection(name: str) -> bpy.types.Collection:
    collection = bpy.data.collections.get(name)
    if collection is None:
        collection = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(collection)
    return collection


def ensure_child_collection(parent: bpy.types.Collection, name: str) -> bpy.types.Collection:
    collection = bpy.data.collections.get(name)
    if collection is None:
        collection = bpy.data.collections.new(name)
    if collection.name not in {child.name for child in parent.children}:
        parent.children.link(collection)
    return collection


def move_objects_to_collection(objects: list[bpy.types.Object], collection: bpy.types.Collection) -> None:
    for obj in objects:
        for prev in list(obj.users_collection):
            prev.objects.unlink(obj)
        if collection.objects.get(obj.name) is None:
            collection.objects.link(obj)


def import_dae(filepath: Path, collection: bpy.types.Collection) -> list[bpy.types.Object]:
    before = {obj.name for obj in bpy.data.objects}
    bpy.ops.wm.collada_import(filepath=str(filepath))
    after_objects = [obj for obj in bpy.data.objects if obj.name not in before]
    move_objects_to_collection(after_objects, collection)
    return after_objects


def compute_world_bounds(objects: list[bpy.types.Object]) -> tuple[Vector, Vector] | None:
    mesh_objects = [obj for obj in objects if obj.type == "MESH" and getattr(obj, "bound_box", None)]
    if not mesh_objects:
        return None

    mins = Vector((float("inf"), float("inf"), float("inf")))
    maxs = Vector((float("-inf"), float("-inf"), float("-inf")))

    for obj in mesh_objects:
        for corner in obj.bound_box:
            world_corner = obj.matrix_world @ Vector(corner)
            mins.x = min(mins.x, world_corner.x)
            mins.y = min(mins.y, world_corner.y)
            mins.z = min(mins.z, world_corner.z)
            maxs.x = max(maxs.x, world_corner.x)
            maxs.y = max(maxs.y, world_corner.y)
            maxs.z = max(maxs.z, world_corner.z)

    return mins, maxs


def recenter_objects(objects: list[bpy.types.Object]) -> dict[str, tuple[float, float, float]] | None:
    bounds = compute_world_bounds(objects)
    if bounds is None:
        return None

    mins, maxs = bounds
    center_xy = Vector(((mins.x + maxs.x) * 0.5, (mins.y + maxs.y) * 0.5, mins.z))
    offset = Vector((-center_xy.x, -center_xy.y, -center_xy.z))

    for obj in objects:
        obj.location += offset

    return {
        "offset": (float(offset.x), float(offset.y), float(offset.z)),
        "original_min": (float(mins.x), float(mins.y), float(mins.z)),
        "original_max": (float(maxs.x), float(maxs.y), float(maxs.z)),
    }


def select_only(objects: list[bpy.types.Object]) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = None
    for obj in objects:
        if bpy.data.objects.get(obj.name) is None:
            continue
        obj.select_set(True)
        if bpy.context.view_layer.objects.active is None:
            bpy.context.view_layer.objects.active = obj


def as_vector(values: list[float] | tuple[float, float, float] | None) -> Vector:
    if not values:
        return Vector((0.0, 0.0, 0.0))
    return Vector((float(values[0]), float(values[1]), float(values[2])))


def main() -> int:
    args = parse_args()
    parallel_root = Path(args.parallel_root)
    out_dir = Path(args.out_dir)

    clear_scene()
    bpy.context.scene.name = f"CarbonMerged_{args.pack_name}"

    merged_root = ensure_collection("Merged_Validation_Packs")
    merged_local = ensure_child_collection(merged_root, "Merged_Local")

    out_dir.mkdir(parents=True, exist_ok=True)
    blend_path = out_dir / f"{args.pack_name}.blend"
    collada_path = out_dir / f"{args.pack_name}_local_only.dae"
    manifest_path = out_dir / "merged_manifest.json"

    total_objects = 0
    total_sections = 0
    all_local_objects: list[bpy.types.Object] = []
    per_pack: list[dict[str, object]] = []

    for pack_name in args.pack_names:
        pack_dir = parallel_root / pack_name
        pack_manifest_path = pack_dir / "pack_manifest.json"
        if not pack_manifest_path.exists():
            raise SystemExit(f"Missing pack manifest: {pack_manifest_path}")

        pack_manifest = json.loads(pack_manifest_path.read_text(encoding="utf-8"))
        collada_source = Path(pack_manifest["outputs"]["collada_local_only"])
        if not collada_source.exists():
            raise SystemExit(f"Missing local-only Collada: {collada_source}")

        pack_collection = ensure_child_collection(merged_local, pack_name)
        imported_objects = import_dae(collada_source, pack_collection)
        restore_offset = -as_vector((pack_manifest.get("recentering") or {}).get("offset"))
        for obj in imported_objects:
            obj.location += restore_offset
            obj["carbon_source_pack"] = pack_name

        mesh_objects = [obj for obj in imported_objects if obj.type == "MESH"]
        all_local_objects.extend(mesh_objects)
        total_objects += len(mesh_objects)
        total_sections += int(pack_manifest["counts"]["section_count"])
        per_pack.append(
            {
                "pack_name": pack_name,
                "path": str(pack_dir),
                "section_count": int(pack_manifest["counts"]["section_count"]),
                "local_object_count": len(mesh_objects),
                "source_collada": str(collada_source),
                "restore_offset": [float(restore_offset.x), float(restore_offset.y), float(restore_offset.z)],
            }
        )

    recenter_info = recenter_objects(list(bpy.data.objects))

    if all_local_objects and not args.skip_collada_export:
        select_only(all_local_objects)
        bpy.ops.wm.collada_export(filepath=str(collada_path), selected=True)

    scene = bpy.context.scene
    scene["carbon_merged_pack_name"] = args.pack_name
    scene["carbon_merged_source_count"] = len(args.pack_names)
    scene["carbon_merged_section_count"] = total_sections
    scene["carbon_merged_local_object_count"] = total_objects
    if recenter_info is not None:
        scene["carbon_recenter_offset"] = ",".join(f"{value:.6f}" for value in recenter_info["offset"])
        scene["carbon_original_bounds_min"] = ",".join(
            f"{value:.6f}" for value in recenter_info["original_min"]
        )
        scene["carbon_original_bounds_max"] = ",".join(
            f"{value:.6f}" for value in recenter_info["original_max"]
        )

    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))

    merged_manifest = {
        "pack_name": args.pack_name,
        "parallel_root": str(parallel_root),
        "outputs": {
            "blend": str(blend_path),
            "collada_local_only": None if args.skip_collada_export else str(collada_path),
        },
        "counts": {
            "source_pack_count": len(args.pack_names),
            "section_count": total_sections,
            "local_object_count": total_objects,
        },
        "recentering": recenter_info,
        "packs": per_pack,
    }
    manifest_path.write_text(json.dumps(merged_manifest, indent=2), encoding="utf-8")

    print(
        f"Saved merged validation pack {blend_path} "
        f"(packs={len(args.pack_names)}, sections={total_sections}, local={total_objects})"
    )
    if not args.skip_collada_export:
        print(f"Local-only Collada: {collada_path}")
    print(f"Merged manifest: {manifest_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
