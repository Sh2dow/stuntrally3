#!/usr/bin/env python3
"""
Blender-side Carbon section scene builder.

Run from Blender:
    blender.exe --background --factory-startup --python CarbonSectionBlendBuilder.py -- \
        --unified-root D:\\Repos\\Games\\NFS-ModTools\\AssetDumper\\output\\L5RA_unified \
        --section-number 1001 \
        --out-blend D:\\Repos\\Games\\NFS-ModTools\\AssetDumper\\output\\L5RA_unified\\blender_sections\\1001.blend
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
    parser.add_argument("--unified-root", required=True)
    parser.add_argument("--section-number", required=True, type=int)
    parser.add_argument("--out-blend", required=True)
    parser.add_argument("--skip-assetdumper", action="store_true")
    parser.add_argument("--skip-modelulator", action="store_true")
    parser.add_argument("--hide-modelulator", action="store_true")
    parser.add_argument("--hide-modelulator-external", action="store_true")
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
        if obj.name not in collection.objects:
            collection.objects.link(obj)


def import_dae(filepath: Path, collection_name: str) -> list[bpy.types.Object]:
    before = {obj.name for obj in bpy.data.objects}
    bpy.ops.wm.collada_import(filepath=str(filepath))
    after_objects = [obj for obj in bpy.data.objects if obj.name not in before]
    collection = ensure_collection(collection_name)
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


def classify_modelulator_objects(
    objects: list[bpy.types.Object],
    bridge_section: dict,
) -> dict[str, int]:
    local_files = {
        entry["file"].lower()
        for entry in bridge_section.get("resolved_solids", [])
        if entry.get("matches_raw_local")
    }
    external_files = {
        entry["file"].lower()
        for entry in bridge_section.get("resolved_solids", [])
        if not entry.get("matches_raw_local")
    }

    parent = ensure_collection("Modelulator_Classified")
    local_collection = ensure_child_collection(parent, "Modelulator_Local")
    external_collection = ensure_child_collection(parent, "Modelulator_External")
    unknown_collection = ensure_child_collection(parent, "Modelulator_Unclassified")

    counts = {"local_objects": 0, "external_objects": 0, "unclassified_objects": 0}

    for obj in objects:
        if obj.type != "MESH" or getattr(obj, "data", None) is None:
            obj["carbon_bridge_class"] = "unclassified"
            move_objects_to_collection([obj], unknown_collection)
            counts["unclassified_objects"] += 1
            continue

        mesh_name = obj.data.name
        if mesh_name.startswith("xSolid_"):
            file_name = f"{mesh_name.removeprefix('xSolid_')}.dae".lower()
            if file_name in local_files:
                obj["carbon_bridge_class"] = "local"
                move_objects_to_collection([obj], local_collection)
                counts["local_objects"] += 1
                continue
            if file_name in external_files:
                obj["carbon_bridge_class"] = "external"
                move_objects_to_collection([obj], external_collection)
                counts["external_objects"] += 1
                continue

        obj["carbon_bridge_class"] = "unclassified"
        move_objects_to_collection([obj], unknown_collection)
        counts["unclassified_objects"] += 1

    return counts


def main() -> int:
    args = parse_args()
    unified_root = Path(args.unified_root)
    manifest = json.loads((unified_root / "unified_manifest.json").read_text(encoding="utf-8"))
    bridge = json.loads((unified_root / "section_solid_bridge.json").read_text(encoding="utf-8"))
    section_key = str(args.section_number)

    section_manifest = next(
        (section for section in manifest["sections"] if section["section_number"] == args.section_number),
        None,
    )
    if section_manifest is None:
        raise SystemExit(f"Section {args.section_number} not found in unified_manifest.json")

    bridge_section = bridge.get("sections", {}).get(section_key, {})

    clear_scene()
    bpy.context.scene.name = f"CarbonSection_{args.section_number}"

    imported_asset_objects = 0
    imported_modelulator_objects = 0
    modelulator_classification = {"local_objects": 0, "external_objects": 0, "unclassified_objects": 0}

    if not args.skip_assetdumper:
        patched = section_manifest.get("patched_assetdumper") or {}
        patched_path = patched.get("patched_path")
        if patched_path:
            imported_asset_objects = len(import_dae(Path(patched_path), "AssetDumper_Textured"))

    if not args.skip_modelulator:
        modelulator = section_manifest.get("modelulator") or {}
        modelulator_path = modelulator.get("path")
        if modelulator_path:
            imported_modelulator_objects = len(import_dae(Path(modelulator_path), "Modelulator_Scenery"))
            modelulator_collection = bpy.data.collections.get("Modelulator_Scenery")
            if modelulator_collection is not None:
                modelulator_classification = classify_modelulator_objects(
                    list(modelulator_collection.objects),
                    bridge_section,
                )

            if args.hide_modelulator:
                modelulator_collection = bpy.data.collections.get("Modelulator_Scenery")
                if modelulator_collection is not None:
                    modelulator_collection.hide_viewport = True
                    modelulator_collection.hide_render = True

            if args.hide_modelulator_external:
                external_collection = bpy.data.collections.get("Modelulator_External")
                if external_collection is not None:
                    external_collection.hide_viewport = True
                    external_collection.hide_render = True

    scene = bpy.context.scene
    scene["carbon_section_number"] = args.section_number
    scene["carbon_section_alias"] = ",".join(section_manifest.get("aliases", []))
    scene["carbon_bridge_raw_local_match_count"] = int(bridge_section.get("raw_local_match_count", 0))
    scene["carbon_bridge_external_match_count"] = int(bridge_section.get("external_match_count", 0))
    scene["carbon_bridge_resolved_solid_count"] = int(bridge_section.get("resolved_solid_count", 0))
    scene["carbon_asset_objects"] = imported_asset_objects
    scene["carbon_modelulator_objects"] = imported_modelulator_objects
    scene["carbon_modelulator_local_objects"] = modelulator_classification["local_objects"]
    scene["carbon_modelulator_external_objects"] = modelulator_classification["external_objects"]
    scene["carbon_modelulator_unclassified_objects"] = modelulator_classification["unclassified_objects"]

    recenter_info = recenter_objects(list(bpy.data.objects))
    if recenter_info is not None:
        scene["carbon_recenter_offset"] = ",".join(f"{value:.6f}" for value in recenter_info["offset"])
        scene["carbon_original_bounds_min"] = ",".join(
            f"{value:.6f}" for value in recenter_info["original_min"]
        )
        scene["carbon_original_bounds_max"] = ",".join(
            f"{value:.6f}" for value in recenter_info["original_max"]
        )

    out_blend = Path(args.out_blend)
    out_blend.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(out_blend))

    print(
        f"Saved {out_blend} "
        f"(asset_objects={imported_asset_objects}, "
        f"modelulator_objects={imported_modelulator_objects}, "
        f"local={modelulator_classification['local_objects']}, "
        f"external={modelulator_classification['external_objects']}, "
        f"unclassified={modelulator_classification['unclassified_objects']})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
