#!/usr/bin/env python3
"""
Unified Carbon Track Toolkit

Builds one normalized bundle from the current Carbon reverse-engineering
artifacts instead of forcing manual cross-referencing across multiple tools.

Inputs this script can merge:
  - Binarius raw streamed sections (L5RA_raw style)
  - modelulator export (ScenerySections / Solids / CollisionPacks / RoadNetworks)
  - AssetDumper section export (section-level DAE + DDS textures)
  - optional PNG textures copied under a nested STREAM*.BUN_PNG tree

Outputs:
  - unified_manifest.json
  - raw_inventory/<track>.section_manifest.json            (optional)
  - asset_sections_textured/<section>.dae                  (patched to local textures)
  - textures/<hash_name>.png or .dds                       (linked or copied)
  - README.txt

Example:
    python CarbonUnifiedToolkit.py ^
        --track-id L5RA ^
        --out "D:\\Repos\\Games\\NFS-ModTools\\AssetDumper\\output\\L5RA_unified" ^
        --raw-input "D:\\Repos\\Games\\Binarius\\Binary\\output\\L5RA_raw" ^
        --bun "D:\\Games\\NFSC Redux\\TRACKS\\L5RA.BUN" ^
        --modelulator "D:\\Repos\\Games\\NFS-ModTools\\AssetDumper\\output\\modelulator" ^
        --assetdumper "D:\\Repos\\Games\\NFS-ModTools\\AssetDumper\\output\\STREAML5RA_test" ^
        --png-textures "D:\\Repos\\Games\\NFS-ModTools\\AssetDumper\\output\\modelulator\\Textures\\STREAML5RA.BUN_PNG"
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import re
import shutil
from collections import Counter, defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple
from urllib.parse import unquote

from CarbonTrackConverter import CarbonTrackConverter


INIT_FROM_RE = re.compile(r"<init_from>(.*?)</init_from>", re.IGNORECASE)
INSTANCE_GEOMETRY_RE = re.compile(r'<instance_geometry[^>]*url="#([^"]+)"', re.IGNORECASE)
INSTANCE_MATERIAL_RE = re.compile(r'<instance_material[^>]*target="#([^"]+)"', re.IGNORECASE)
MODELULATOR_INSTANCE_NODE_RE = re.compile(r'<instance_node[^>]*url="([^"]+)"', re.IGNORECASE)
MODELULATOR_SECTION_RE = re.compile(r"^SECTION_S(?P<number>\d+)_(?P<alias>[^.]+)\.dae$", re.IGNORECASE)
MODELULATOR_SOLID_RE = re.compile(
    r"^(?P<hash>0x[0-9A-Fa-f]{8})\.(?P<section>[^.]+)(?:\.(?P<label>.*))?\.dae$"
)
MODELULATOR_PALETTE_RE = re.compile(r"^eSolidPalette\.(?P<section>[^.]+)\.dae$", re.IGNORECASE)
RAW_HASH_PREFIX_RE = re.compile(r"^0x[0-9A-Fa-f]{8}_")
SCENERY_SECTION_NUMBER_RE = re.compile(r'S(?P<number>\d+)_0x[0-9A-Fa-f]+', re.IGNORECASE)
INTERNAL_IMAGE_ID_RE = re.compile(r"^texture-0x[0-9A-Fa-f]+-img$", re.IGNORECASE)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--track-id", required=True, help="Track id, for example L5RA")
    parser.add_argument("--out", required=True, help="Unified toolkit output directory")
    parser.add_argument("--raw-input", help="Binarius raw section directory, for example L5RA_raw")
    parser.add_argument("--bun", help="Optional BUN path for CarbonTrackConverter inventory")
    parser.add_argument("--modelulator", help="modelulator output root")
    parser.add_argument("--assetdumper", help="AssetDumper section export root")
    parser.add_argument(
        "--png-textures",
        help="Root of copied PNG textures, usually modelulator\\Textures\\STREAM*.BUN_PNG",
    )
    parser.add_argument(
        "--link-mode",
        choices=("hardlink", "copy", "skip"),
        default="hardlink",
        help="How to materialize resolved textures into the unified output",
    )
    parser.add_argument(
        "--full-geometry-details",
        action="store_true",
        help="Pass through to CarbonTrackConverter raw inventory generation",
    )
    return parser.parse_args()


def ensure_dir(path: Path) -> Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def write_json(path: Path, payload: object) -> None:
    with path.open("w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2)


def relative_to(base: Path, path: Optional[Path]) -> Optional[str]:
    if path is None:
        return None
    try:
        return str(path.resolve().relative_to(base.resolve()))
    except ValueError:
        return str(path.resolve())


def link_or_copy(src: Path, dst: Path, mode: str) -> bool:
    if mode == "skip":
        return False

    ensure_dir(dst.parent)
    if dst.exists():
        return False

    try:
        if mode == "hardlink":
            os.link(src, dst)
        else:
            shutil.copy2(src, dst)
    except OSError:
        shutil.copy2(src, dst)

    return True


def dedupe_sorted(items: Iterable[str]) -> List[str]:
    return sorted({item for item in items if item})


def limit_list(items: Sequence[str], count: int = 16) -> List[str]:
    return list(items[:count])


def normalized_texture_name(texture_name: str) -> str:
    stem = Path(texture_name).stem
    return RAW_HASH_PREFIX_RE.sub("", stem).lower()


def decode_modelulator_url(raw_url: str) -> Tuple[str, str]:
    decoded = unquote(raw_url)
    if "#" in decoded:
        path_part, anchor = decoded.split("#", 1)
    else:
        path_part, anchor = decoded, ""
    return path_part.replace("/", "\\"), anchor


class UnifiedCarbonToolkit:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.track_id = args.track_id
        self.output_root = Path(args.out)
        self.raw_input = Path(args.raw_input) if args.raw_input else None
        self.bun_path = Path(args.bun) if args.bun else None
        self.modelulator_root = Path(args.modelulator) if args.modelulator else None
        self.assetdumper_root = Path(args.assetdumper) if args.assetdumper else None
        self.png_texture_root = self._resolve_png_root(args.png_textures)

        self.raw_inventory_dir = self.output_root / "raw_inventory"
        self.textures_out_dir = self.output_root / "textures"
        self.asset_sections_out_dir = self.output_root / "asset_sections_textured"
        self.section_solid_bridge_json = self.output_root / "section_solid_bridge.json"
        self.section_solid_bridge_csv = self.output_root / "section_solid_bridge.csv"

    def _resolve_png_root(self, explicit: Optional[str]) -> Optional[Path]:
        if explicit:
            return Path(explicit)
        if self.modelulator_root is None:
            return None

        candidate = self.modelulator_root / "Textures" / f"STREAM{self.track_id}.BUN_PNG"
        return candidate if candidate.exists() else None

    def run(self) -> int:
        ensure_dir(self.output_root)
        ensure_dir(self.textures_out_dir)
        ensure_dir(self.asset_sections_out_dir)

        raw_manifest = self._build_or_load_raw_manifest()
        modelulator_index = self._scan_modelulator()
        assetdumper_index = self._scan_assetdumper()
        png_index = self._scan_png_textures()
        materialization = self._materialize_assetdumper_sections(assetdumper_index, png_index)
        section_solid_bridge = self._build_section_solid_bridge(raw_manifest, modelulator_index)
        unified_manifest = self._build_unified_manifest(
            raw_manifest,
            modelulator_index,
            assetdumper_index,
            png_index,
            materialization,
            section_solid_bridge,
        )

        manifest_path = self.output_root / "unified_manifest.json"
        write_json(self.section_solid_bridge_json, section_solid_bridge)
        self._write_section_solid_bridge_csv(section_solid_bridge)
        write_json(manifest_path, unified_manifest)
        self._write_readme(unified_manifest, manifest_path)

        print(f"Wrote {manifest_path}")
        print(
            "Summary: "
            f"{unified_manifest['counts']['unified_sections']} unified sections, "
            f"{unified_manifest['counts']['patched_asset_sections']} patched AssetDumper DAEs, "
            f"{unified_manifest['counts']['materialized_textures']} materialized textures"
        )
        return 0

    def _build_or_load_raw_manifest(self) -> Optional[Dict[str, object]]:
        if self.raw_input is None:
            return None

        ensure_dir(self.raw_inventory_dir)
        converter = CarbonTrackConverter(
            str(self.raw_input),
            str(self.raw_inventory_dir),
            self.track_id,
            str(self.bun_path) if self.bun_path else None,
            self.args.full_geometry_details,
        )

        if not converter.convert():
            raise RuntimeError(f"CarbonTrackConverter failed for {self.raw_input}")

        manifest_path = self.raw_inventory_dir / f"{self.track_id}.section_manifest.json"
        return json.loads(read_text(manifest_path))

    def _scan_modelulator(self) -> Dict[str, object]:
        if self.modelulator_root is None or not self.modelulator_root.exists():
            return {"root": None, "sections": {}, "counts": {}}

        scenery_root = self.modelulator_root / "ScenerySections"
        solids_root = self.modelulator_root / "Solids"
        collision_root = self.modelulator_root / "CollisionPacks"
        roads_root = self.modelulator_root / "RoadNetworks"

        sections: Dict[str, Dict[str, object]] = {}
        solid_files: Dict[str, Dict[str, object]] = {}
        if scenery_root.exists():
            for path in sorted(scenery_root.glob("*.dae")):
                section = self._parse_modelulator_section(path)
                if section is None:
                    continue
                sections[str(section["section_number"])] = section

        solid_counts_by_section: Counter[str] = Counter()
        palette_counts_by_section: Counter[str] = Counter()
        if solids_root.exists():
            for path in sorted(solids_root.glob("*.dae")):
                match = MODELULATOR_SOLID_RE.match(path.name)
                if match:
                    solid_section = match.group("section")
                    solid_counts_by_section[solid_section] += 1
                    solid_files[path.name.lower()] = {
                        "file": path.name,
                        "path": str(path),
                        "hash": match.group("hash").lower(),
                        "source_section": solid_section,
                        "label": match.group("label") or "",
                    }
                    continue
                palette_match = MODELULATOR_PALETTE_RE.match(path.name)
                if palette_match:
                    palette_counts_by_section[palette_match.group("section")] += 1

        for section_info in sections.values():
            resolved_solids = 0
            source_sections = Counter()
            for solid_ref in section_info.get("solid_refs", []):
                solid_meta = solid_files.get(solid_ref.lower())
                if solid_meta is None:
                    continue
                resolved_solids += 1
                source_sections[solid_meta["source_section"]] += 1

            section_info["resolved_solid_file_count"] = resolved_solids
            section_info["missing_solid_file_count"] = max(
                0, section_info["unique_solid_reference_count"] - resolved_solids
            )
            section_info["solid_source_section_count"] = len(source_sections)
            section_info["sample_solid_source_sections"] = limit_list(
                [section for section, _count in source_sections.most_common(16)]
            )

            alias = section_info.get("alias")
            if alias:
                section_info["palette_file_count"] = palette_counts_by_section.get(alias, 0)

        counts = {
            "scenery_sections": len(list(scenery_root.glob("*.dae"))) if scenery_root.exists() else 0,
            "solid_files": len(list(solids_root.glob("*.dae"))) if solids_root.exists() else 0,
            "collision_files": len(list(collision_root.glob("*.obj"))) if collision_root.exists() else 0,
            "road_files": len(list(roads_root.glob("*.obj"))) if roads_root.exists() else 0,
        }

        return {
            "root": str(self.modelulator_root),
            "sections": sections,
            "solid_files": solid_files,
            "counts": counts,
        }

    def _parse_modelulator_section(self, path: Path) -> Optional[Dict[str, object]]:
        match = MODELULATOR_SECTION_RE.match(path.name)
        section_number: Optional[int] = None
        alias: Optional[str] = None

        if match:
            section_number = int(match.group("number"))
            alias = match.group("alias")
        else:
            text = read_text(path)
            number_match = SCENERY_SECTION_NUMBER_RE.search(text)
            if number_match:
                section_number = int(number_match.group("number"))
            alias = path.stem.replace("SECTION_", "", 1)
            if section_number is None:
                return None

        text = read_text(path)
        solid_refs: List[str] = []
        for url in MODELULATOR_INSTANCE_NODE_RE.findall(text):
            decoded_path, _anchor = decode_modelulator_url(url)
            if "Solids\\" in decoded_path:
                solid_refs.append(Path(decoded_path).name)

        return {
            "section_number": section_number,
            "alias": alias,
            "path": str(path),
            "instance_count": len(MODELULATOR_INSTANCE_NODE_RE.findall(text)),
            "solid_reference_count": len(solid_refs),
            "unique_solid_reference_count": len(set(solid_refs)),
            "solid_refs": sorted(set(solid_refs)),
            "sample_solid_files": limit_list(sorted(set(solid_refs))),
        }

    def _scan_assetdumper(self) -> Dict[str, object]:
        if self.assetdumper_root is None or not self.assetdumper_root.exists():
            return {
                "root": None,
                "textures_root": None,
                "sections": {},
                "texture_files": {},
                "counts": {},
            }

        sections: Dict[str, Dict[str, object]] = {}
        for path in sorted(self.assetdumper_root.glob("*.dae")):
            if not path.stem.isdigit():
                continue
            section_number = int(path.stem)
            text = read_text(path)
            texture_refs = dedupe_sorted(Path(raw_ref.replace("/", "\\")).name for raw_ref in INIT_FROM_RE.findall(text))
            geometry_refs = dedupe_sorted(INSTANCE_GEOMETRY_RE.findall(text))
            material_refs = dedupe_sorted(INSTANCE_MATERIAL_RE.findall(text))
            sections[str(section_number)] = {
                "section_number": section_number,
                "path": str(path),
                "texture_refs": texture_refs,
                "texture_ref_count": len(texture_refs),
                "geometry_ref_count": len(geometry_refs),
                "material_ref_count": len(material_refs),
                "sample_textures": limit_list(texture_refs),
            }

        textures_root = self.assetdumper_root / "textures"
        texture_files: Dict[str, str] = {}
        if textures_root.exists():
            for path in sorted(textures_root.glob("*")):
                if path.is_file():
                    texture_files[path.name.lower()] = str(path)

        return {
            "root": str(self.assetdumper_root),
            "textures_root": str(textures_root) if textures_root.exists() else None,
            "sections": sections,
            "texture_files": texture_files,
            "counts": {
                "section_daes": len(sections),
                "dds_textures": len(texture_files),
            },
        }

    def _scan_png_textures(self) -> Dict[str, object]:
        if self.png_texture_root is None or not self.png_texture_root.exists():
            return {"root": None, "by_name": {}, "by_section_name": {}, "count": 0}

        by_name: Dict[str, List[str]] = defaultdict(list)
        by_section_name: Dict[str, List[str]] = defaultdict(list)

        png_files = sorted(self.png_texture_root.rglob("*.png"))
        for path in png_files:
            by_name[path.name.lower()].append(str(path))

            try:
                rel = path.relative_to(self.png_texture_root)
            except ValueError:
                rel = path

            parts = rel.parts
            section_hint = parts[0] if len(parts) > 1 else ""
            key = f"{section_hint.lower()}::{path.name.lower()}"
            by_section_name[key].append(str(path))

        return {
            "root": str(self.png_texture_root),
            "by_name": dict(by_name),
            "by_section_name": dict(by_section_name),
            "count": len(png_files),
        }

    def _resolve_png_texture(
        self,
        texture_name: str,
        section_number: Optional[int],
        png_index: Dict[str, object],
    ) -> Optional[Path]:
        if not png_index.get("root"):
            return None

        section_key = str(section_number) if section_number is not None else ""
        basename_candidates = []

        hashed_png = Path(texture_name).with_suffix(".png").name
        stripped_png = f"{RAW_HASH_PREFIX_RE.sub('', Path(texture_name).stem)}.png"
        basename_candidates.extend([hashed_png, stripped_png])

        for candidate in basename_candidates:
            scoped_key = f"{section_key.lower()}::{candidate.lower()}"
            scoped_matches = png_index["by_section_name"].get(scoped_key, [])
            if len(scoped_matches) == 1:
                return Path(scoped_matches[0])

        for candidate in basename_candidates:
            matches = png_index["by_name"].get(candidate.lower(), [])
            if len(matches) == 1:
                return Path(matches[0])

        return None

    def _materialize_assetdumper_sections(
        self,
        assetdumper_index: Dict[str, object],
        png_index: Dict[str, object],
    ) -> Dict[str, object]:
        if not assetdumper_index.get("root"):
            return {"sections": {}, "materialized_textures": 0}

        result_sections: Dict[str, Dict[str, object]] = {}
        materialized_destinations: set[str] = set()
        asset_texture_files: Dict[str, str] = assetdumper_index.get("texture_files", {})

        for section_key, section in assetdumper_index["sections"].items():
            source_path = Path(section["path"])
            section_number = section["section_number"]
            text = read_text(source_path)

            resolved_png = 0
            fallback_dds = 0
            unresolved: List[str] = []
            replacement_map: Dict[str, str] = {}

            for raw_ref in INIT_FROM_RE.findall(text):
                original_name = Path(raw_ref.replace("/", "\\")).name
                if not Path(original_name).suffix and INTERNAL_IMAGE_ID_RE.match(original_name):
                    continue
                replacement = None

                png_source = self._resolve_png_texture(original_name, section_number, png_index)
                if png_source is not None:
                    output_name = f"{Path(original_name).stem}.png"
                    dest = self.textures_out_dir / output_name
                    link_or_copy(png_source, dest, self.args.link_mode)
                    materialized_destinations.add(str(dest))
                    replacement = f"..\\textures\\{output_name}"
                    resolved_png += 1
                else:
                    dds_source = asset_texture_files.get(original_name.lower())
                    if dds_source:
                        dest = self.textures_out_dir / original_name
                        link_or_copy(Path(dds_source), dest, self.args.link_mode)
                        materialized_destinations.add(str(dest))
                        replacement = f"..\\textures\\{original_name}"
                        fallback_dds += 1

                if replacement is None:
                    unresolved.append(original_name)
                    continue

                replacement_map[raw_ref] = replacement

            patched_text = text
            for raw_ref, replacement in replacement_map.items():
                patched_text = patched_text.replace(raw_ref, replacement)

            patched_path = self.asset_sections_out_dir / source_path.name
            patched_path.write_text(patched_text, encoding="utf-8")

            result_sections[section_key] = {
                "patched_path": str(patched_path),
                "resolved_png_count": resolved_png,
                "fallback_dds_count": fallback_dds,
                "unresolved_texture_count": len(unresolved),
                "sample_unresolved_textures": limit_list(sorted(set(unresolved))),
            }

        return {
            "sections": result_sections,
            "materialized_textures": len(materialized_destinations),
        }

    def _build_unified_manifest(
        self,
        raw_manifest: Optional[Dict[str, object]],
        modelulator_index: Dict[str, object],
        assetdumper_index: Dict[str, object],
        png_index: Dict[str, object],
        materialization: Dict[str, object],
        section_solid_bridge: Dict[str, object],
    ) -> Dict[str, object]:
        raw_sections_by_number: Dict[str, List[Dict[str, object]]] = defaultdict(list)
        raw_section_count = 0
        if raw_manifest:
            raw_section_count = raw_manifest.get("section_count", 0)
            for section in raw_manifest.get("sections", []):
                geometry = section.get("geometry_pack")
                section_number = geometry.get("section_number") if isinstance(geometry, dict) else None
                if section_number is None:
                    continue
                raw_sections_by_number[str(section_number)].append(
                    {
                        "raw_section_name": section.get("name"),
                        "kind": section.get("kind"),
                        "size": section.get("size"),
                        "solid_count": geometry.get("solid_count"),
                        "sample_local_solid_keys": geometry.get("sample_local_solid_keys", []),
                    }
                )

        section_keys = sorted(
            {
                *raw_sections_by_number.keys(),
                *modelulator_index.get("sections", {}).keys(),
                *assetdumper_index.get("sections", {}).keys(),
            },
            key=lambda item: int(item),
        )

        unified_sections: List[Dict[str, object]] = []
        for section_key in section_keys:
            modelulator_section = modelulator_index.get("sections", {}).get(section_key)
            assetdumper_section = assetdumper_index.get("sections", {}).get(section_key)
            patched_section = materialization.get("sections", {}).get(section_key)
            bridge_section = section_solid_bridge.get("sections", {}).get(section_key)

            aliases = []
            if modelulator_section and modelulator_section.get("alias"):
                aliases.append(modelulator_section["alias"])
            aliases = dedupe_sorted(aliases)

            unified_sections.append(
                {
                    "section_number": int(section_key),
                    "aliases": aliases,
                    "raw_sections": raw_sections_by_number.get(section_key, []),
                    "modelulator": None
                    if modelulator_section is None
                    else {
                        "path": modelulator_section["path"],
                        "alias": modelulator_section.get("alias"),
                        "instance_count": modelulator_section.get("instance_count"),
                        "solid_reference_count": modelulator_section.get("solid_reference_count"),
                        "unique_solid_reference_count": modelulator_section.get("unique_solid_reference_count"),
                        "resolved_solid_file_count": modelulator_section.get("resolved_solid_file_count"),
                        "missing_solid_file_count": modelulator_section.get("missing_solid_file_count"),
                        "solid_source_section_count": modelulator_section.get("solid_source_section_count"),
                        "palette_file_count": modelulator_section.get("palette_file_count"),
                        "sample_solid_source_sections": modelulator_section.get("sample_solid_source_sections", []),
                        "sample_solid_files": modelulator_section.get("sample_solid_files", []),
                    },
                    "assetdumper": None
                    if assetdumper_section is None
                    else {
                        "path": assetdumper_section["path"],
                        "texture_ref_count": assetdumper_section.get("texture_ref_count"),
                        "geometry_ref_count": assetdumper_section.get("geometry_ref_count"),
                        "material_ref_count": assetdumper_section.get("material_ref_count"),
                        "sample_textures": assetdumper_section.get("sample_textures", []),
                    },
                    "patched_assetdumper": patched_section,
                    "solid_bridge": None
                    if bridge_section is None
                    else {
                        "bridge_path": bridge_section.get("bridge_path"),
                        "resolved_solid_count": bridge_section.get("resolved_solid_count"),
                        "missing_solid_count": bridge_section.get("missing_solid_count"),
                        "raw_local_match_count": bridge_section.get("raw_local_match_count"),
                        "external_match_count": bridge_section.get("external_match_count"),
                        "sample_local_solid_files": bridge_section.get("sample_local_solid_files", []),
                        "sample_external_solid_files": bridge_section.get("sample_external_solid_files", []),
                    },
                }
            )

        return {
            "track_id": self.track_id,
            "roots": {
                "output": str(self.output_root),
                "raw_input": str(self.raw_input) if self.raw_input else None,
                "bun": str(self.bun_path) if self.bun_path else None,
                "modelulator": str(self.modelulator_root) if self.modelulator_root else None,
                "assetdumper": str(self.assetdumper_root) if self.assetdumper_root else None,
                "png_textures": str(self.png_texture_root) if self.png_texture_root else None,
            },
            "counts": {
                "raw_sections": raw_section_count,
                "modelulator_scenery_sections": modelulator_index.get("counts", {}).get("scenery_sections", 0),
                "modelulator_solid_files": modelulator_index.get("counts", {}).get("solid_files", 0),
                "assetdumper_sections": assetdumper_index.get("counts", {}).get("section_daes", 0),
                "assetdumper_dds_textures": assetdumper_index.get("counts", {}).get("dds_textures", 0),
                "png_textures": png_index.get("count", 0),
                "patched_asset_sections": len(materialization.get("sections", {})),
                "materialized_textures": materialization.get("materialized_textures", 0),
                "solid_bridge_sections": section_solid_bridge.get("counts", {}).get("sections", 0),
                "solid_bridge_resolved_solids": section_solid_bridge.get("counts", {}).get("resolved_solids", 0),
                "unified_sections": len(unified_sections),
            },
            "outputs": {
                "raw_inventory": relative_to(self.output_root, self.raw_inventory_dir) if self.raw_input else None,
                "patched_asset_sections": relative_to(self.output_root, self.asset_sections_out_dir),
                "textures": relative_to(self.output_root, self.textures_out_dir),
                "section_solid_bridge_json": relative_to(self.output_root, self.section_solid_bridge_json),
                "section_solid_bridge_csv": relative_to(self.output_root, self.section_solid_bridge_csv),
            },
            "sections": unified_sections,
        }

    def _build_section_solid_bridge(
        self,
        raw_manifest: Optional[Dict[str, object]],
        modelulator_index: Dict[str, object],
    ) -> Dict[str, object]:
        raw_local_keys_by_number: Dict[str, set[str]] = {}
        raw_names_by_number: Dict[str, List[str]] = defaultdict(list)
        if raw_manifest:
            for section in raw_manifest.get("sections", []):
                geometry = section.get("geometry_pack")
                if not isinstance(geometry, dict):
                    continue
                section_number = geometry.get("section_number")
                if section_number is None:
                    continue
                section_key = str(section_number)
                raw_names_by_number[section_key].append(section.get("name"))
                local_keys = geometry.get("local_solid_keys") or []
                if local_keys:
                    raw_local_keys_by_number.setdefault(section_key, set()).update(
                        key.lower() for key in local_keys
                    )

        solid_files = modelulator_index.get("solid_files", {})
        bridge_sections: Dict[str, Dict[str, object]] = {}
        resolved_solids = 0
        missing_solids = 0

        for section_key, section_info in modelulator_index.get("sections", {}).items():
            solid_refs = section_info.get("solid_refs", [])
            raw_local_keys = raw_local_keys_by_number.get(section_key, set())

            resolved_entries: List[Dict[str, object]] = []
            missing_entries: List[str] = []
            local_files: List[str] = []
            external_files: List[str] = []

            for solid_ref in solid_refs:
                solid_meta = solid_files.get(solid_ref.lower())
                if solid_meta is None:
                    missing_entries.append(solid_ref)
                    continue

                solid_hash = solid_meta["hash"]
                is_local = solid_hash in raw_local_keys
                if is_local:
                    local_files.append(solid_meta["file"])
                else:
                    external_files.append(solid_meta["file"])

                resolved_entries.append(
                    {
                        "file": solid_meta["file"],
                        "path": solid_meta["path"],
                        "hash": solid_hash,
                        "source_section": solid_meta["source_section"],
                        "label": solid_meta["label"],
                        "matches_raw_local": is_local,
                    }
                )

            resolved_entries.sort(key=lambda item: item["file"].lower())
            missing_entries = sorted(set(missing_entries))
            local_files = sorted(set(local_files))
            external_files = sorted(set(external_files))

            resolved_solids += len(resolved_entries)
            missing_solids += len(missing_entries)

            bridge_sections[section_key] = {
                "section_number": section_info["section_number"],
                "alias": section_info.get("alias"),
                "modelulator_section_path": section_info.get("path"),
                "raw_section_names": sorted(set(raw_names_by_number.get(section_key, []))),
                "raw_local_solid_key_count": len(raw_local_keys),
                "referenced_solid_count": len(solid_refs),
                "resolved_solid_count": len(resolved_entries),
                "missing_solid_count": len(missing_entries),
                "raw_local_match_count": len(local_files),
                "external_match_count": len(external_files),
                "sample_local_solid_files": limit_list(local_files),
                "sample_external_solid_files": limit_list(external_files),
                "sample_missing_solid_files": limit_list(missing_entries),
                "resolved_solids": resolved_entries,
                "bridge_path": str(self.section_solid_bridge_json),
            }

        return {
            "track_id": self.track_id,
            "output_json": str(self.section_solid_bridge_json),
            "output_csv": str(self.section_solid_bridge_csv),
            "counts": {
                "sections": len(bridge_sections),
                "resolved_solids": resolved_solids,
                "missing_solids": missing_solids,
            },
            "sections": bridge_sections,
        }

    def _write_section_solid_bridge_csv(self, bridge: Dict[str, object]) -> None:
        fieldnames = [
            "section_number",
            "alias",
            "raw_section_names",
            "raw_local_solid_key_count",
            "referenced_solid_count",
            "resolved_solid_count",
            "missing_solid_count",
            "raw_local_match_count",
            "external_match_count",
        ]
        with self.section_solid_bridge_csv.open("w", encoding="utf-8", newline="") as handle:
            writer = csv.DictWriter(handle, fieldnames=fieldnames)
            writer.writeheader()
            for section in sorted(
                bridge.get("sections", {}).values(),
                key=lambda item: int(item["section_number"]),
            ):
                writer.writerow(
                    {
                        "section_number": section["section_number"],
                        "alias": section.get("alias") or "",
                        "raw_section_names": ",".join(section.get("raw_section_names", [])),
                        "raw_local_solid_key_count": section.get("raw_local_solid_key_count", 0),
                        "referenced_solid_count": section.get("referenced_solid_count", 0),
                        "resolved_solid_count": section.get("resolved_solid_count", 0),
                        "missing_solid_count": section.get("missing_solid_count", 0),
                        "raw_local_match_count": section.get("raw_local_match_count", 0),
                        "external_match_count": section.get("external_match_count", 0),
                    }
                )

    def _write_readme(self, manifest: Dict[str, object], manifest_path: Path) -> None:
        counts = manifest["counts"]
        lines = [
            "Unified Carbon Track Toolkit Output",
            "===================================",
            "",
            f"Track: {self.track_id}",
            f"Manifest: {manifest_path.name}",
            "",
            "What this bundle contains:",
            f"- unified sections: {counts['unified_sections']}",
            f"- raw inventoried sections: {counts['raw_sections']}",
            f"- modelulator scenery sections: {counts['modelulator_scenery_sections']}",
            f"- modelulator solid files: {counts['modelulator_solid_files']}",
            f"- AssetDumper section DAEs: {counts['assetdumper_sections']}",
            f"- patched AssetDumper DAEs: {counts['patched_asset_sections']}",
            f"- materialized local textures: {counts['materialized_textures']}",
            f"- section-to-solid bridge sections: {counts['solid_bridge_sections']}",
            f"- section-to-solid resolved solids: {counts['solid_bridge_resolved_solids']}",
            "",
            "How to use it:",
            "- `asset_sections_textured/` contains the AssetDumper section DAEs rewritten to local textures",
            "- `textures/` contains the linked or copied texture files that those patched DAEs reference",
            "- `unified_manifest.json` is the cross-tool index tying together raw sections, modelulator sections, and AssetDumper sections",
            "- `section_solid_bridge.json` ties each Carbon section to the exact modelulator solid DAEs it instantiates",
            "- `section_solid_bridge.csv` is the same bridge in scan-friendly summary form",
            "",
            "Important notes:",
            "- modelulator solid DAEs are still mostly geometry-only in the current export; the textured DAEs come from AssetDumper",
            "- the unified toolkit keeps original heavy exports in place and writes a normalized overlay instead of duplicating everything",
            "",
        ]
        (self.output_root / "README.txt").write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    args = parse_args()
    toolkit = UnifiedCarbonToolkit(args)
    return toolkit.run()


if __name__ == "__main__":
    raise SystemExit(main())
