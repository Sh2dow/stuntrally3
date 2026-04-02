#!/usr/bin/env python3
"""
Blender-side Carbon object scaffold exporter for SR3.

This script chunks a merged Carbon map .blend into spatial object packs,
exports one Ogre .mesh.xml per chunk, and writes SR3 <objects> placement XML
plus a manifest. The final .mesh conversion is handled by OgreMeshTool.
"""

from __future__ import annotations

import argparse
import json
import math
import shutil
import sys
import xml.etree.ElementTree as ET
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
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--track-id", required=True)
    parser.add_argument("--prefix", default="w2c_chunk")
    parser.add_argument("--chunk-size", type=float, default=500.0)
    parser.add_argument("--exclude-collection", action="append", default=["Carbon_RoadNetwork"])
    parser.add_argument(
        "--exclude-name-prefix",
        action="append",
        default=["PAN_", "ROCKPORTOCEAN", "CU_EXWAVES_", "DAMN_WATER"],
    )
    parser.add_argument("--exclude-overlay", action="store_true", default=True)
    parser.add_argument("--max-chunks", type=int, default=0)
    return parser.parse_args(argv)


def is_collection_excluded(obj: bpy.types.Object, excluded_names: set[str]) -> bool:
    return any(collection.name in excluded_names for collection in obj.users_collection)


def visible_mesh_objects(
    excluded_collections: set[str], excluded_name_prefixes: tuple[str, ...], exclude_overlay: bool
) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue
        if obj.hide_viewport or obj.hide_get():
            continue
        if excluded_name_prefixes and obj.name.upper().startswith(excluded_name_prefixes):
            continue
        if excluded_collections and is_collection_excluded(obj, excluded_collections):
            continue
        if exclude_overlay and obj.get("carbon_overlay_type"):
            continue
        objects.append(obj)
    return objects


def object_world_bounds(obj: bpy.types.Object) -> tuple[Vector, Vector]:
    corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    min_corner = Vector(
        (
            min(corner.x for corner in corners),
            min(corner.y for corner in corners),
            min(corner.z for corner in corners),
        )
    )
    max_corner = Vector(
        (
            max(corner.x for corner in corners),
            max(corner.y for corner in corners),
            max(corner.z for corner in corners),
        )
    )
    return min_corner, max_corner


def chunk_key_for_object(obj: bpy.types.Object, chunk_size: float) -> tuple[int, int]:
    min_corner, max_corner = object_world_bounds(obj)
    center = (min_corner + max_corner) * 0.5
    return (
        math.floor(center.x / chunk_size),
        math.floor(center.y / chunk_size),
    )


def bounds_from_objects(objects: list[bpy.types.Object]) -> tuple[Vector, Vector]:
    min_corner = Vector((float("inf"), float("inf"), float("inf")))
    max_corner = Vector((float("-inf"), float("-inf"), float("-inf")))
    for obj in objects:
        obj_min, obj_max = object_world_bounds(obj)
        min_corner.x = min(min_corner.x, obj_min.x)
        min_corner.y = min(min_corner.y, obj_min.y)
        min_corner.z = min(min_corner.z, obj_min.z)
        max_corner.x = max(max_corner.x, obj_max.x)
        max_corner.y = max(max_corner.y, obj_max.y)
        max_corner.z = max(max_corner.z, obj_max.z)
    return min_corner, max_corner


def sanitize_name(text: str) -> str:
    out = []
    for ch in text.lower():
        if ch.isalnum():
            out.append(ch)
        else:
            out.append("_")
    return "".join(out).strip("_")


def to_sr3_ogre_axes(vec: Vector) -> Vector:
    return Vector((vec.x, vec.z, -vec.y))


def first_material_image(material: bpy.types.Material | None) -> Path | None:
    if material is None or not material.use_nodes or material.node_tree is None:
        return None
    for node in material.node_tree.nodes:
        if node.type == "TEX_IMAGE" and getattr(node, "image", None):
            image_path = Path(bpy.path.abspath(node.image.filepath))
            if image_path.exists():
                return image_path
    return None


def write_chunk_materials(materials_dir: Path, mesh_name: str, material_entries: list[dict[str, str]]) -> Path:
    material_path = materials_dir / f"{mesh_name}.material.json"
    pbs: dict[str, object] = {}
    for entry in material_entries:
        material_def: dict[str, object] = {
            "workflow": "specular_ogre",
            "diffuse": {
                "value": [1, 1, 1],
                "background": [1, 1, 1, 1],
            },
            "specular": {
                "value": [0.08, 0.08, 0.08],
            },
            "fresnel": {
                "value": 0.02,
                "mode": "coeff",
            },
            "roughness": {
                "value": 0.9,
            },
        }
        texture_name = entry.get("texture_name", "")
        if texture_name:
            material_def["diffuse"]["texture"] = texture_name
        pbs[entry["name"]] = material_def

    material_json = {"pbs": pbs}
    material_path.write_text(json.dumps(material_json, indent=2), encoding="utf-8")
    return material_path


def build_chunk_mesh_xml(
    filepath: Path,
    materials_dir: Path,
    textures_dir: Path,
    mesh_name: str,
    objects: list[bpy.types.Object],
    origin: Vector,
) -> dict[str, object]:
    depsgraph = bpy.context.evaluated_depsgraph_get()
    bounds_min = Vector((float("inf"), float("inf"), float("inf")))
    bounds_max = Vector((float("-inf"), float("-inf"), float("-inf")))
    groups: dict[str, dict[str, object]] = {}
    copied_textures: dict[str, str] = {}

    for obj in objects:
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
        try:
            mesh.calc_loop_triangles()
            uv_layer = mesh.uv_layers.active.data if mesh.uv_layers.active else None
            material_slots = list(obj.material_slots)
            for tri in mesh.loop_triangles:
                mat = None
                if tri.material_index < len(material_slots):
                    mat = material_slots[tri.material_index].material
                tex_path = first_material_image(mat)
                if tex_path is not None:
                    texture_name = tex_path.name
                    if texture_name not in copied_textures:
                        dest = textures_dir / texture_name
                        if not dest.exists():
                            shutil.copy2(tex_path, dest)
                        copied_textures[texture_name] = str(dest)
                    material_name = f"{mesh_name}_{sanitize_name(mat.name if mat else texture_name)}"
                else:
                    texture_name = ""
                    material_name = f"{mesh_name}_fallback"
                group = groups.setdefault(
                    material_name,
                    {"texture_name": texture_name, "vertices": [], "faces": []},
                )
                tri_indices: list[int] = []
                for loop_index, vert_index in zip(tri.loops, tri.vertices):
                    vertex = mesh.vertices[vert_index]
                    world_pos = obj.matrix_world @ vertex.co
                    local_pos = to_sr3_ogre_axes(world_pos - origin)
                    normal = to_sr3_ogre_axes((obj.matrix_world.to_3x3() @ vertex.normal).normalized()).normalized()
                    uv = (0.0, 0.0)
                    if uv_layer is not None:
                        luv = uv_layer[loop_index].uv
                        uv = (float(luv.x), float(1.0 - luv.y))

                    bounds_min.x = min(bounds_min.x, local_pos.x)
                    bounds_min.y = min(bounds_min.y, local_pos.y)
                    bounds_min.z = min(bounds_min.z, local_pos.z)
                    bounds_max.x = max(bounds_max.x, local_pos.x)
                    bounds_max.y = max(bounds_max.y, local_pos.y)
                    bounds_max.z = max(bounds_max.z, local_pos.z)

                    tri_indices.append(len(group["vertices"]))
                    group["vertices"].append(
                        (
                            (float(local_pos.x), float(local_pos.y), float(local_pos.z)),
                            (float(normal.x), float(normal.y), float(normal.z)),
                            uv,
                        )
                    )
                group["faces"].append((tri_indices[0], tri_indices[1], tri_indices[2]))
        finally:
            evaluated.to_mesh_clear()

    root = ET.Element("mesh")
    submeshes = ET.SubElement(root, "submeshes")
    material_entries: list[dict[str, str]] = []
    total_vertices = 0
    total_faces = 0

    for material_name, group in groups.items():
        vertices = group["vertices"]
        faces = group["faces"]
        sub = ET.SubElement(
            submeshes,
            "submesh",
            material=material_name,
            usesharedvertices="false",
            use32bitindexes="true",
            operationtype="triangle_list",
        )
        faces_el = ET.SubElement(sub, "faces", count=str(len(faces)))
        for i0, i1, i2 in faces:
            ET.SubElement(faces_el, "face", v1=str(i0), v2=str(i1), v3=str(i2))
        geom = ET.SubElement(sub, "geometry", vertexcount=str(len(vertices)))
        vb = ET.SubElement(
            geom,
            "vertexbuffer",
            positions="true",
            normals="true",
            texture_coords="1",
            texture_coord_dimensions_0="2",
        )
        for pos, normal, uv in vertices:
            v = ET.SubElement(vb, "vertex")
            ET.SubElement(v, "position", x=f"{pos[0]:.9g}", y=f"{pos[1]:.9g}", z=f"{pos[2]:.9g}")
            ET.SubElement(v, "normal", x=f"{normal[0]:.9g}", y=f"{normal[1]:.9g}", z=f"{normal[2]:.9g}")
            ET.SubElement(v, "texcoord", u=f"{uv[0]:.9g}", v=f"{uv[1]:.9g}")

        material_entries.append({"name": material_name, "texture_name": group["texture_name"]})
        total_vertices += len(vertices)
        total_faces += len(faces)

    tree = ET.ElementTree(root)
    ET.indent(tree, space="    ")
    tree.write(filepath, encoding="utf-8", xml_declaration=False)
    material_path = write_chunk_materials(materials_dir, mesh_name, material_entries)
    return {
        "vertex_count": total_vertices,
        "triangle_count": total_faces,
        "local_bounds_min": [bounds_min.x, bounds_min.y, bounds_min.z],
        "local_bounds_max": [bounds_max.x, bounds_max.y, bounds_max.z],
        "material_path": str(material_path),
        "texture_names": sorted(copied_textures.keys()),
        "submesh_count": len(groups),
    }


def chunk_sort_key(item: tuple[tuple[int, int], list[bpy.types.Object]]) -> tuple[int, int]:
    key, _ = item
    return (key[1], key[0])


def build_scaffold(args: argparse.Namespace) -> dict[str, object]:
    in_blend = Path(args.in_blend)
    out_dir = Path(args.out_dir)
    if not in_blend.exists():
        raise SystemExit(f"Missing input blend: {in_blend}")

    bpy.ops.wm.open_mainfile(filepath=str(in_blend))

    out_dir.mkdir(parents=True, exist_ok=True)
    mesh_xml_dir = out_dir / "mesh_xml"
    materials_dir = out_dir / "materials"
    textures_dir = out_dir / "textures"
    mesh_xml_dir.mkdir(parents=True, exist_ok=True)
    materials_dir.mkdir(parents=True, exist_ok=True)
    textures_dir.mkdir(parents=True, exist_ok=True)

    excluded = set(args.exclude_collection or [])
    excluded_name_prefixes = tuple((args.exclude_name_prefix or []))
    objects = visible_mesh_objects(excluded, excluded_name_prefixes, args.exclude_overlay)
    if not objects:
        raise SystemExit("No visible mesh objects found for scaffold export")

    chunks: dict[tuple[int, int], list[bpy.types.Object]] = {}
    for obj in objects:
        chunks.setdefault(chunk_key_for_object(obj, args.chunk_size), []).append(obj)

    ordered_chunks = sorted(chunks.items(), key=chunk_sort_key)
    if args.max_chunks > 0:
        ordered_chunks = ordered_chunks[: args.max_chunks]

    xml_lines = ["<objects>"]
    manifest_chunks: list[dict[str, object]] = []
    exported_chunk_count = 0
    total_object_refs = 0

    for index, (chunk_key, chunk_objects) in enumerate(ordered_chunks, start=1):
        chunk_min, chunk_max = bounds_from_objects(chunk_objects)
        origin = (chunk_min + chunk_max) * 0.5
        mesh_name = f"{sanitize_name(args.prefix)}_{index:04d}"
        mesh_xml_path = mesh_xml_dir / f"{mesh_name}.mesh.xml"
        xml_stats = build_chunk_mesh_xml(mesh_xml_path, materials_dir, textures_dir, mesh_name, chunk_objects, origin)

        xml_lines.append(
            f'    <o name="{mesh_name}" pos="{origin.x:.6f} {origin.y:.6f} {origin.z:.6f}" rot="0 0 0 1"/>'
        )

        manifest_chunks.append(
            {
                "index": index,
                "name": mesh_name,
                "chunk_key": [chunk_key[0], chunk_key[1]],
                "mesh_xml_path": str(mesh_xml_path),
                "origin": [origin.x, origin.y, origin.z],
                "bounds_min": [chunk_min.x, chunk_min.y, chunk_min.z],
                "bounds_max": [chunk_max.x, chunk_max.y, chunk_max.z],
                "object_count": len(chunk_objects),
                "vertex_count": xml_stats["vertex_count"],
                "triangle_count": xml_stats["triangle_count"],
                "submesh_count": xml_stats["submesh_count"],
                "material_path": xml_stats["material_path"],
                "texture_names": xml_stats["texture_names"],
                "sample_objects": [obj.name for obj in chunk_objects[:32]],
            }
        )
        exported_chunk_count += 1
        total_object_refs += len(chunk_objects)

    xml_lines.append("</objects>")
    scene_xml_path = out_dir / "scene_objects_scaffold.xml"
    scene_xml_path.write_text("\n".join(xml_lines) + "\n", encoding="utf-8")

    manifest = {
        "track_id": args.track_id,
        "input_blend": str(in_blend),
        "output_dir": str(out_dir),
        "chunk_size": args.chunk_size,
        "exclude_collections": sorted(excluded),
        "exclude_name_prefixes": list(excluded_name_prefixes),
        "exclude_overlay": bool(args.exclude_overlay),
        "exported_chunk_count": exported_chunk_count,
        "total_object_refs": total_object_refs,
        "runtime_note": (
            "Ogre mesh XML scaffold exported with per-chunk .material scripts and copied textures. "
            "Convert each mesh_xml/<name>.mesh.xml to data/models/objectsC/<name>.mesh "
            "before applying scene_objects_scaffold.xml to the runtime scene."
        ),
        "chunks": manifest_chunks,
    }
    manifest_path = out_dir / "objects_scaffold_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    todo_lines = [
        f"Track: {args.track_id}",
        f"Chunks exported: {exported_chunk_count}",
        "",
        "Next steps:",
        "1. Convert each mesh_xml/<name>.mesh.xml into Ogre .mesh files with OgreMeshTool.",
        "2. Copy resulting <name>.mesh files plus generated materials/textures into data/models/objectsC/.",
        "3. Optionally generate matching <name>.bullet files for collision.",
        "4. Apply scene_objects_scaffold.xml into data/tracks/<TrackId>/scene.xml.",
        "",
        "Generated files:",
        f"- {manifest_path}",
        f"- {scene_xml_path}",
        f"- {mesh_xml_dir}",
        f"- {materials_dir}",
        f"- {textures_dir}",
    ]
    (out_dir / "README_scaffold.txt").write_text("\n".join(todo_lines) + "\n", encoding="utf-8")
    return manifest


def main() -> int:
    args = parse_args()
    manifest = build_scaffold(args)
    print(f"Track: {manifest['track_id']}")
    print(f"Chunks exported: {manifest['exported_chunk_count']}")
    print(f"Manifest: {Path(manifest['output_dir']) / 'objects_scaffold_manifest.json'}")
    print(f"Scene XML scaffold: {Path(manifest['output_dir']) / 'scene_objects_scaffold.xml'}")
    print(f"Mesh XML directory: {Path(manifest['output_dir']) / 'mesh_xml'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
