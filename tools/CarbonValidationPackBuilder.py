#!/usr/bin/env python3
"""
Blender-side Carbon validation pack builder.

Builds one merged local-only validation pack from a list of Carbon section
numbers inside a unified toolkit bundle.

Run from Blender:
    blender.exe --background --factory-startup --python CarbonValidationPackBuilder.py -- \
        --unified-root D:\\Repos\\Games\\NFS-ModTools\\AssetDumper\\output\\L5RA_unified \
        --section-numbers 1001 1002 \
        --out-dir D:\\Repos\\Games\\NFS-ModTools\\AssetDumper\\output\\L5RA_unified\\validation_packs\\sections_1001_1002 \
        --pack-name sections_1001_1002
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

import bpy
from mathutils import Vector


ASSET_GEOMETRY_RE = re.compile(
    r'<geometry id="geometry-(?P<hash>0x[0-9A-Fa-f]{8})" name="(?P<name>[^"]+)"',
    re.IGNORECASE,
)
BLENDER_DUPLICATE_SUFFIX_RE = re.compile(r"\.\d{3}$")


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1 :]
    else:
        argv = []

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unified-root", required=True)
    parser.add_argument("--section-numbers", required=True, nargs="+", type=int)
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--pack-name", required=True)
    parser.add_argument("--skip-assetdumper", action="store_true")
    parser.add_argument("--skip-modelulator", action="store_true")
    parser.add_argument("--hide-assetdumper-reference", action="store_true")
    parser.add_argument("--keep-nonlocal-modelulator", action="store_true")
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


def tag_objects(
    objects: list[bpy.types.Object],
    *,
    section_number: int,
    section_alias: str,
    source: str,
    bridge_class: str | None = None,
) -> None:
    for obj in objects:
        obj["carbon_section_number"] = int(section_number)
        obj["carbon_section_alias"] = section_alias
        obj["carbon_source"] = source
        if bridge_class is not None:
            obj["carbon_bridge_class"] = bridge_class


def import_dae(filepath: Path, collection: bpy.types.Collection) -> list[bpy.types.Object]:
    before = {obj.name for obj in bpy.data.objects}
    bpy.ops.wm.collada_import(filepath=str(filepath))
    after_objects = [obj for obj in bpy.data.objects if obj.name not in before]
    move_objects_to_collection(after_objects, collection)
    return after_objects


def strip_blender_duplicate_suffix(name: str) -> str:
    return BLENDER_DUPLICATE_SUFFIX_RE.sub("", name)


def parse_asset_geometry_map(path: Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8", errors="replace")
    mapping: dict[str, str] = {}
    for match in ASSET_GEOMETRY_RE.finditer(text):
        mapping[match.group("hash").lower()] = match.group("name")
    return mapping


def classify_modelulator_objects(
    objects: list[bpy.types.Object],
    bridge_section: dict,
) -> dict[str, list[bpy.types.Object]]:
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

    classified = {"local": [], "external": [], "unclassified": []}

    for obj in objects:
        if obj.type != "MESH" or getattr(obj, "data", None) is None:
            classified["unclassified"].append(obj)
            continue

        mesh_name = obj.data.name
        if mesh_name.startswith("xSolid_"):
            file_name = f"{mesh_name.removeprefix('xSolid_')}.dae".lower()
            if file_name in local_files:
                classified["local"].append(obj)
                continue
            if file_name in external_files:
                classified["external"].append(obj)
                continue

        classified["unclassified"].append(obj)

    return classified


def classify_assetdumper_objects(
    objects: list[bpy.types.Object],
    local_geometry_names: set[str],
) -> dict[str, list[bpy.types.Object]]:
    classified = {"local": [], "other": [], "unclassified": []}

    for obj in objects:
        if obj.type != "MESH" or getattr(obj, "data", None) is None:
            classified["unclassified"].append(obj)
            continue

        mesh_name = strip_blender_duplicate_suffix(obj.data.name)
        if mesh_name in local_geometry_names:
            classified["local"].append(obj)
        else:
            classified["other"].append(obj)

    return classified


def delete_objects(objects: list[bpy.types.Object]) -> None:
    for obj in objects:
        if bpy.data.objects.get(obj.name) is not None:
            bpy.data.objects.remove(obj, do_unlink=True)


def hide_collection(collection: bpy.types.Collection | None) -> None:
    if collection is None:
        return
    collection.hide_viewport = True
    collection.hide_render = True


def select_only(objects: list[bpy.types.Object]) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = None
    for obj in objects:
        if bpy.data.objects.get(obj.name) is None:
            continue
        obj.select_set(True)
        if bpy.context.view_layer.objects.active is None:
            bpy.context.view_layer.objects.active = obj


def collect_reference_paths(modelulator_root: Path | None) -> dict[str, object]:
    if modelulator_root is None or not modelulator_root.exists():
        return {"road_networks": [], "collision_root": None, "collision_count": 0}

    roads_root = modelulator_root / "RoadNetworks"
    collision_root = modelulator_root / "CollisionPacks"
    road_paths = [str(path) for path in sorted(roads_root.glob("*.obj"))] if roads_root.exists() else []
    collision_paths = list(collision_root.glob("*.obj")) if collision_root.exists() else []

    return {
        "road_networks": road_paths,
        "collision_root": str(collision_root) if collision_root.exists() else None,
        "collision_count": len(collision_paths),
    }


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


def main() -> int:
    args = parse_args()
    unified_root = Path(args.unified_root)
    out_dir = Path(args.out_dir)
    section_numbers = sorted(set(args.section_numbers))

    manifest = json.loads((unified_root / "unified_manifest.json").read_text(encoding="utf-8"))
    bridge = json.loads((unified_root / "section_solid_bridge.json").read_text(encoding="utf-8"))
    sections_by_number = {section["section_number"]: section for section in manifest["sections"]}

    missing_sections = [number for number in section_numbers if number not in sections_by_number]
    if missing_sections:
        raise SystemExit(f"Sections missing from unified_manifest.json: {missing_sections}")

    clear_scene()

    pack_root = ensure_collection("Validation_Pack")
    local_parent = ensure_child_collection(pack_root, "Validation_Local")
    asset_parent = ensure_child_collection(pack_root, "Validation_AssetReference")
    dropped_parent = ensure_child_collection(pack_root, "Validation_Dropped")

    if args.skip_assetdumper:
        hide_collection(asset_parent)
    if not args.keep_nonlocal_modelulator:
        hide_collection(dropped_parent)

    bpy.context.scene.name = f"CarbonValidation_{args.pack_name}"

    out_dir.mkdir(parents=True, exist_ok=True)
    blend_path = out_dir / f"{args.pack_name}.blend"
    collada_path = out_dir / f"{args.pack_name}_local_only.dae"
    manifest_path = out_dir / "pack_manifest.json"

    total_asset_objects = 0
    total_modelulator_objects = 0
    total_local_objects = 0
    total_external_objects = 0
    total_unclassified_objects = 0
    local_export_objects: list[bpy.types.Object] = []
    per_section_stats: list[dict[str, object]] = []

    for section_number in section_numbers:
        section_manifest = sections_by_number[section_number]
        bridge_section = bridge.get("sections", {}).get(str(section_number), {})
        section_aliases = section_manifest.get("aliases", [])
        section_alias = section_aliases[0] if section_aliases else ""
        patched = section_manifest.get("patched_assetdumper") or {}
        patched_path = patched.get("patched_path")
        local_hashes = {
            entry["hash"].lower()
            for entry in bridge_section.get("resolved_solids", [])
            if entry.get("matches_raw_local")
        }
        asset_local_geometry_names: set[str] = set()
        if patched_path and local_hashes:
            geometry_map = parse_asset_geometry_map(Path(patched_path))
            asset_local_geometry_names = {
                geometry_name
                for hash_value, geometry_name in geometry_map.items()
                if hash_value in local_hashes
            }

        asset_count = 0
        modelulator_count = 0
        local_count = 0
        external_count = 0
        unclassified_count = 0

        if not args.skip_assetdumper:
            if patched_path:
                asset_collection = ensure_child_collection(
                    asset_parent,
                    f"Section_{section_number}_AssetReference",
                )
                imported_asset_objects = import_dae(Path(patched_path), asset_collection)
                tag_objects(
                    imported_asset_objects,
                    section_number=section_number,
                    section_alias=section_alias,
                    source="assetdumper",
                )
                asset_count = len(imported_asset_objects)
                total_asset_objects += asset_count

                asset_classified = classify_assetdumper_objects(
                    imported_asset_objects,
                    asset_local_geometry_names,
                )
                local_collection = ensure_child_collection(
                    local_parent,
                    f"Section_{section_number}_Local",
                )
                move_objects_to_collection(asset_classified["local"], local_collection)
                tag_objects(
                    asset_classified["local"],
                    section_number=section_number,
                    section_alias=section_alias,
                    source="assetdumper-local",
                    bridge_class="local",
                )

                local_count = len(asset_classified["local"])
                external_count = len(asset_classified["other"])
                unclassified_count = len(asset_classified["unclassified"])

                total_local_objects += local_count
                total_external_objects += external_count
                total_unclassified_objects += unclassified_count
                local_export_objects.extend(asset_classified["local"])

                if args.keep_nonlocal_modelulator:
                    other_collection = ensure_child_collection(
                        dropped_parent,
                        f"Section_{section_number}_AssetOther",
                    )
                    unknown_collection = ensure_child_collection(
                        dropped_parent,
                        f"Section_{section_number}_AssetUnclassified",
                    )
                    move_objects_to_collection(asset_classified["other"], other_collection)
                    move_objects_to_collection(asset_classified["unclassified"], unknown_collection)
                    tag_objects(
                        asset_classified["other"],
                        section_number=section_number,
                        section_alias=section_alias,
                        source="assetdumper-other",
                        bridge_class="external",
                    )
                    tag_objects(
                        asset_classified["unclassified"],
                        section_number=section_number,
                        section_alias=section_alias,
                        source="assetdumper-other",
                        bridge_class="unclassified",
                    )
                    hide_collection(other_collection)
                    hide_collection(unknown_collection)
                else:
                    delete_objects(asset_classified["other"])
                    delete_objects(asset_classified["unclassified"])

        if not args.skip_modelulator:
            modelulator = section_manifest.get("modelulator") or {}
            modelulator_path = modelulator.get("path")
            if modelulator_path:
                modelulator_count = int(modelulator.get("instance_count") or 0)
                total_modelulator_objects += modelulator_count

        per_section_stats.append(
            {
                "section_number": section_number,
                "aliases": section_aliases,
                "asset_object_count": asset_count,
                "modelulator_object_count": modelulator_count,
                "local_object_count": local_count,
                "external_object_count": external_count,
                "unclassified_object_count": unclassified_count,
                "raw_local_match_count": int(bridge_section.get("raw_local_match_count", 0)),
                "external_match_count": int(bridge_section.get("external_match_count", 0)),
                "resolved_solid_count": int(bridge_section.get("resolved_solid_count", 0)),
                "asset_local_geometry_name_count": len(asset_local_geometry_names),
                "sample_asset_local_geometry_names": sorted(asset_local_geometry_names)[:16],
                "sample_local_solid_files": bridge_section.get("sample_local_solid_files", []),
                "sample_external_solid_files": bridge_section.get("sample_external_solid_files", []),
            }
        )

    if args.hide_assetdumper_reference:
        hide_collection(asset_parent)

    local_mesh_objects = [obj for obj in local_export_objects if bpy.data.objects.get(obj.name) is not None]
    recenter_targets = local_mesh_objects
    if not recenter_targets:
        recenter_targets = [obj for obj in bpy.data.objects if obj.type == "MESH"]
    recenter_info = recenter_objects(list(bpy.data.objects) if recenter_targets else [])

    if local_mesh_objects and not args.skip_collada_export:
        select_only(local_mesh_objects)
        bpy.ops.wm.collada_export(filepath=str(collada_path), selected=True)

    scene = bpy.context.scene
    scene["carbon_pack_name"] = args.pack_name
    scene["carbon_track_id"] = manifest.get("track_id", "")
    scene["carbon_section_numbers"] = ",".join(str(number) for number in section_numbers)
    scene["carbon_local_object_count"] = total_local_objects
    scene["carbon_asset_reference_object_count"] = total_asset_objects
    scene["carbon_modelulator_object_count"] = total_modelulator_objects
    scene["carbon_external_object_count"] = total_external_objects
    scene["carbon_unclassified_object_count"] = total_unclassified_objects
    if recenter_info is not None:
        scene["carbon_recenter_offset"] = ",".join(f"{value:.6f}" for value in recenter_info["offset"])
        scene["carbon_original_bounds_min"] = ",".join(
            f"{value:.6f}" for value in recenter_info["original_min"]
        )
        scene["carbon_original_bounds_max"] = ",".join(
            f"{value:.6f}" for value in recenter_info["original_max"]
        )

    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))

    modelulator_root = Path(manifest["roots"]["modelulator"]) if manifest["roots"].get("modelulator") else None
    pack_manifest = {
        "track_id": manifest.get("track_id"),
        "pack_name": args.pack_name,
        "section_numbers": section_numbers,
        "outputs": {
            "blend": str(blend_path),
            "collada_local_only": None if args.skip_collada_export else str(collada_path),
        },
        "counts": {
            "section_count": len(section_numbers),
            "asset_reference_objects": total_asset_objects,
            "modelulator_objects_imported": total_modelulator_objects,
            "local_objects_retained": total_local_objects,
            "external_objects_removed_or_hidden": total_external_objects,
            "unclassified_objects_removed_or_hidden": total_unclassified_objects,
        },
        "options": {
            "skip_assetdumper": bool(args.skip_assetdumper),
            "skip_modelulator": bool(args.skip_modelulator),
            "hide_assetdumper_reference": bool(args.hide_assetdumper_reference),
            "keep_nonlocal_modelulator": bool(args.keep_nonlocal_modelulator),
            "skip_collada_export": bool(args.skip_collada_export),
        },
        "references": collect_reference_paths(modelulator_root),
        "recentering": recenter_info,
        "sections": per_section_stats,
    }
    manifest_path.write_text(json.dumps(pack_manifest, indent=2), encoding="utf-8")

    print(
        f"Saved validation pack {blend_path} "
        f"(sections={len(section_numbers)}, "
        f"local={total_local_objects}, "
        f"asset_reference={total_asset_objects}, "
        f"external={total_external_objects}, "
        f"unclassified={total_unclassified_objects})"
    )
    if not args.skip_collada_export:
        print(f"Local-only Collada: {collada_path}")
    print(f"Pack manifest: {manifest_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
