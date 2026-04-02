#!/usr/bin/env python3
"""
Blender-side Carbon slice to SR3 intermediate exporter.

Opens a validation/merged .blend, collects map mesh objects, writes an SR3M
track mesh, emits a manifest, and can optionally invoke the existing
SR3TrackBuilder pipeline for heightmap/road/scene generation.
"""

from __future__ import annotations

import argparse
import json
import struct
import subprocess
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Vector


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1 :]
    else:
        argv = []

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--in-blend", required=True)
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--track-name", required=True)
    parser.add_argument("--builder-script")
    parser.add_argument("--run-track-builder", action="store_true")
    parser.add_argument("--exclude-collection", action="append", default=["Carbon_RoadNetwork"])
    parser.add_argument("--exclude-overlay", action="store_true", default=True)
    return parser.parse_args(argv)


def is_collection_excluded(obj: bpy.types.Object, excluded_names: set[str]) -> bool:
    return any(collection.name in excluded_names for collection in obj.users_collection)


def iter_export_objects(excluded_collections: set[str], exclude_overlay: bool) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue
        if obj.hide_viewport or obj.hide_get():
            continue
        if excluded_collections and is_collection_excluded(obj, excluded_collections):
            continue
        if exclude_overlay and obj.get("carbon_overlay_type"):
            continue
        objects.append(obj)
    return objects


def gather_vertices(objects: list[bpy.types.Object]) -> tuple[list[bytes], dict[str, object]]:
    depsgraph = bpy.context.evaluated_depsgraph_get()
    rows: list[bytes] = []
    object_stats: list[dict[str, object]] = []

    total_triangles = 0
    min_corner = Vector((float("inf"), float("inf"), float("inf")))
    max_corner = Vector((float("-inf"), float("-inf"), float("-inf")))

    for obj in objects:
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
        try:
            mesh.calc_loop_triangles()
            uv_layer = mesh.uv_layers.active.data if mesh.uv_layers.active else None
            tri_count = len(mesh.loop_triangles)
            vert_count = tri_count * 3

            for tri in mesh.loop_triangles:
                for loop_index, vert_index in zip(tri.loops, tri.vertices):
                    vertex = mesh.vertices[vert_index]
                    position = obj.matrix_world @ vertex.co
                    normal = (obj.matrix_world.to_3x3() @ vertex.normal).normalized()
                    uv = (0.0, 0.0)
                    if uv_layer is not None:
                        luv = uv_layer[loop_index].uv
                        uv = (float(luv.x), float(luv.y))

                    min_corner.x = min(min_corner.x, position.x)
                    min_corner.y = min(min_corner.y, position.y)
                    min_corner.z = min(min_corner.z, position.z)
                    max_corner.x = max(max_corner.x, position.x)
                    max_corner.y = max(max_corner.y, position.y)
                    max_corner.z = max(max_corner.z, position.z)

                    rows.append(
                        struct.pack(
                            "<fff fff ff ffff",
                            float(position.x),
                            float(position.y),
                            float(position.z),
                            float(normal.x),
                            float(normal.y),
                            float(normal.z),
                            uv[0],
                            uv[1],
                            0.0,
                            0.0,
                            0.0,
                            1.0,
                        )
                    )

            total_triangles += tri_count
            object_stats.append(
                {
                    "name": obj.name,
                    "triangles": tri_count,
                    "vertices_written": vert_count,
                }
            )
        finally:
            evaluated.to_mesh_clear()

    bounds = None
    if rows:
        bounds = {
            "min": [float(min_corner.x), float(min_corner.y), float(min_corner.z)],
            "max": [float(max_corner.x), float(max_corner.y), float(max_corner.z)],
        }

    return rows, {
        "object_count": len(objects),
        "triangle_count": total_triangles,
        "vertex_count": len(rows),
        "bounds": bounds,
        "sample_objects": object_stats[:64],
    }


def write_sr3m(path: Path, rows: list[bytes]) -> None:
    with path.open("wb") as f:
        f.write(b"SR3M")
        f.write(struct.pack("<I", len(rows)))
        for row in rows:
            f.write(row)


def write_manifest(path: Path, manifest: dict[str, object]) -> None:
    path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")


def maybe_run_track_builder(builder_script: Path, intermediate_dir: Path, output_dir: Path, track_name: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            sys.executable,
            str(builder_script),
            str(intermediate_dir),
            str(output_dir),
            track_name,
        ],
        text=True,
        capture_output=True,
        check=False,
    )


def main() -> int:
    args = parse_args()
    in_blend = Path(args.in_blend)
    out_dir = Path(args.out_dir)
    builder_script = Path(args.builder_script) if args.builder_script else None

    if not in_blend.exists():
        raise SystemExit(f"Missing input blend: {in_blend}")
    if args.run_track_builder and (builder_script is None or not builder_script.exists()):
        raise SystemExit("Track builder was requested, but --builder-script is missing or invalid")

    bpy.ops.wm.open_mainfile(filepath=str(in_blend))

    out_dir.mkdir(parents=True, exist_ok=True)
    intermediate_dir = out_dir / "intermediate"
    generated_dir = out_dir / "generated_track"
    intermediate_dir.mkdir(parents=True, exist_ok=True)
    generated_dir.mkdir(parents=True, exist_ok=True)

    excluded_collections = set(args.exclude_collection or [])
    export_objects = iter_export_objects(excluded_collections, args.exclude_overlay)
    if not export_objects:
        raise SystemExit("No mesh objects selected for export")

    rows, mesh_stats = gather_vertices(export_objects)
    if not rows:
        raise SystemExit("No mesh triangles were written to SR3M")

    track_mesh_path = intermediate_dir / "track.mesh"
    write_sr3m(track_mesh_path, rows)

    road_overlay = bpy.context.scene.get("carbon_road_overlay_obj")
    manifest = {
        "track_name": args.track_name,
        "input_blend": str(in_blend),
        "intermediate_dir": str(intermediate_dir),
        "generated_track_dir": str(generated_dir),
        "outputs": {
            "track_mesh": str(track_mesh_path),
            "road_overlay_source": road_overlay,
        },
        "export_filters": {
            "exclude_collections": sorted(excluded_collections),
            "exclude_overlay": bool(args.exclude_overlay),
        },
        "mesh_stats": mesh_stats,
        "scene_properties": {
            "carbon_recenter_offset": bpy.context.scene.get("carbon_recenter_offset"),
            "carbon_original_bounds_min": bpy.context.scene.get("carbon_original_bounds_min"),
            "carbon_original_bounds_max": bpy.context.scene.get("carbon_original_bounds_max"),
        },
    }

    builder_result = None
    if args.run_track_builder:
        builder_result = maybe_run_track_builder(builder_script, intermediate_dir, generated_dir, args.track_name)
        manifest["track_builder"] = {
            "script": str(builder_script),
            "return_code": builder_result.returncode,
            "stdout": builder_result.stdout[-8000:],
            "stderr": builder_result.stderr[-8000:],
        }

    write_manifest(out_dir / "slice_to_sr3_manifest.json", manifest)

    print(f"SR3M written: {track_mesh_path}")
    print(f"Objects exported: {mesh_stats['object_count']}")
    print(f"Triangles exported: {mesh_stats['triangle_count']}")
    print(f"Vertices written: {mesh_stats['vertex_count']}")
    print(f"Manifest: {out_dir / 'slice_to_sr3_manifest.json'}")

    if builder_result is not None:
        print(f"Track builder return code: {builder_result.returncode}")
        if builder_result.stdout:
            print(builder_result.stdout)
        if builder_result.stderr:
            print(builder_result.stderr)
        return builder_result.returncode

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
