#!/usr/bin/env python3
"""
Convert Carbon object scaffold Ogre .mesh.xml chunks into Ogre .mesh using OgreMeshTool.

This script assumes the Ogre .mesh.xml files were already exported by
CarbonObjectScaffoldBuilder.py and that OgreMeshTool.exe exists.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--track-root", required=True)
    parser.add_argument("--mesh-tool", required=True)
    parser.add_argument("--objects-dir", required=True)
    parser.add_argument("--prefix", default="w2c_chunk_")
    parser.add_argument("--first", type=int, default=0)
    parser.add_argument("--max-count", type=int, default=0)
    parser.add_argument("--overwrite", action="store_true")
    return parser.parse_args()


def run_cmd(args: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=str(cwd) if cwd else None, text=True, capture_output=True, check=False)


def merge_material_jsons(material_files: list[Path], output_path: Path) -> int:
    merged: dict[str, object] = {}
    samplers: dict[str, object] = {}
    macroblocks: dict[str, object] = {}
    blendblocks: dict[str, object] = {}
    pbs: dict[str, object] = {}

    sampler_name_map: dict[str, str] = {}
    macroblock_name_map: dict[str, str] = {}
    blendblock_name_map: dict[str, str] = {}

    def remap_name(kind: str, original: str, name_map: dict[str, str]) -> str:
        remapped = name_map.get(original)
        if remapped:
            return remapped
        safe_original = original.replace(" ", "_")
        remapped = f"W2C_{kind}_{safe_original}"
        name_map[original] = remapped
        return remapped

    for material_file in material_files:
        data = json.loads(material_file.read_text(encoding="utf-8"))
        for key, value in data.get("samplers", {}).items():
            samplers[remap_name("Sampler", key, sampler_name_map)] = value
        for key, value in data.get("macroblocks", {}).items():
            macroblocks[remap_name("Macroblock", key, macroblock_name_map)] = value
        for key, value in data.get("blendblocks", {}).items():
            blendblocks[remap_name("Blendblock", key, blendblock_name_map)] = value
        for key, value in data.get("pbs", {}).items():
            entry = json.loads(json.dumps(value))
            if "macroblock" in entry:
                entry["macroblock"] = remap_name("Macroblock", entry["macroblock"], macroblock_name_map)
            if "blendblock" in entry:
                entry["blendblock"] = remap_name("Blendblock", entry["blendblock"], blendblock_name_map)
            for tex_block_name in ("diffuse", "specular", "normal", "roughness", "emissive", "detail_weight"):
                tex_block = entry.get(tex_block_name)
                if isinstance(tex_block, dict) and "sampler" in tex_block:
                    tex_block["sampler"] = remap_name("Sampler", tex_block["sampler"], sampler_name_map)
            pbs[key] = entry

    merged["samplers"] = samplers
    merged["macroblocks"] = macroblocks
    merged["blendblocks"] = blendblocks
    merged["pbs"] = pbs
    output_path.write_text(json.dumps(merged, indent=2), encoding="utf-8")
    return len(pbs)


def main() -> int:
    args = parse_args()
    track_root = Path(args.track_root)
    mesh_tool = Path(args.mesh_tool)
    objects_dir = Path(args.objects_dir)

    scaffold_root = track_root / "objects_scaffold"
    manifest_path = scaffold_root / "objects_scaffold_manifest.json"
    mesh_xml_dir = scaffold_root / "mesh_xml"
    materials_dir = scaffold_root / "materials"
    textures_dir = scaffold_root / "textures"
    if not manifest_path.exists():
        scaffold_root = track_root
        manifest_path = scaffold_root / "objects_scaffold_manifest.json"
        mesh_xml_dir = scaffold_root / "mesh_xml"
        materials_dir = scaffold_root / "materials"
        textures_dir = scaffold_root / "textures"

    if not manifest_path.exists():
        raise SystemExit(f"Missing scaffold manifest: {manifest_path}")
    if not mesh_xml_dir.exists():
        raise SystemExit(f"Missing scaffold mesh XML directory: {mesh_xml_dir}")
    if not mesh_tool.exists():
        raise SystemExit(f"Missing OgreMeshTool: {mesh_tool}")

    objects_dir.mkdir(parents=True, exist_ok=True)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    chunks = manifest.get("chunks", [])
    if args.first > 0:
        chunks = chunks[args.first - 1 :]
    if args.max_count > 0:
        chunks = chunks[: args.max_count]

    results: list[dict[str, object]] = []
    failures = 0

    for legacy_material in objects_dir.glob(f"{args.prefix}*.material"):
        legacy_material.unlink(missing_ok=True)

    for chunk in chunks:
        name = chunk["name"]
        xml_path = Path(chunk["mesh_xml_path"])
        mesh_path = objects_dir / f"{name}.mesh"

        if mesh_path.exists() and not args.overwrite:
            results.append({"name": name, "status": "skipped_exists", "mesh_path": str(mesh_path)})
            continue

        step = run_cmd(
            [
                str(mesh_tool),
                "-v2",
                "-e",
                "-t",
                "-ts",
                "4",
                "-O",
                "puqs",
                str(xml_path),
                str(mesh_path),
            ]
        )
        if step.returncode != 0:
            failures += 1
            results.append(
                {
                    "name": name,
                    "status": "xml_to_mesh_failed",
                    "xml_path": str(xml_path),
                    "mesh_path": str(mesh_path),
                    "stdout": step.stdout[-4000:],
                    "stderr": step.stderr[-4000:],
                }
            )
            continue

        if not mesh_path.exists():
            failures += 1
            results.append(
                {
                    "name": name,
                    "status": "mesh_missing_after_success",
                    "xml_path": str(xml_path),
                    "mesh_path": str(mesh_path),
                }
            )
            continue

        results.append(
            {
                "name": name,
                "status": "ok",
                "xml_path": str(xml_path),
                "mesh_path": str(mesh_path),
            }
        )

    copied_materials = 0
    copied_textures = 0
    if materials_dir.exists():
        material_files = sorted(materials_dir.glob("*.material.json"))
        if material_files:
            merged_material_path = objects_dir / f"{args.prefix.rstrip('_')}_all.material.json"
            copied_materials = merge_material_jsons(material_files, merged_material_path)
    if textures_dir.exists():
        for texture_file in textures_dir.iterdir():
            if texture_file.is_file():
                shutil.copy2(texture_file, objects_dir / texture_file.name)
                copied_textures += 1

    report = {
        "track_root": str(track_root),
        "mesh_tool": str(mesh_tool),
        "objects_dir": str(objects_dir),
        "requested_chunks": len(chunks),
        "failures": failures,
        "copied_materials": copied_materials,
        "copied_textures": copied_textures,
        "results": results,
    }
    report_path = scaffold_root / "mesh_conversion_report.json"
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(f"Requested chunks: {len(chunks)}")
    print(f"Failures: {failures}")
    print(f"Report: {report_path}")
    print(f"Objects dir: {objects_dir}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
