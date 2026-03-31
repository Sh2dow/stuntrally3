#!/usr/bin/env python3
"""
Carbon Track Section Inventory

Inventories Binarius-extracted NFS Carbon track sections and writes a JSON
report describing what is actually present in the streamed files.

This tool does not decode Carbon world geometry into SR3 meshes.
The current confirmed section types are:
  - A*  -> GeometryPack / ScenerySection
  - Y*  -> TPKBlocks / texture pack data
  - Z0  -> WCollisionPack

Usage:
    python CarbonTrackConverter.py <input_dir> <output_dir> [track_id] [--bun <bun_file>]

Examples:
    python CarbonTrackConverter.py "output\\L5RA_raw" "data\\tracks\\L5RA" L5RA
    python CarbonTrackConverter.py "output\\L5RA_raw" "data\\tracks\\L5RA" L5RA --bun "D:\\Games\\NFSC Redux\\TRACKS\\L5RA.BUN"
    python CarbonTrackConverter.py "output\\L5RA_raw" "data\\tracks\\L5RA" L5RA --full-geometry-details
"""

from __future__ import annotations

import json
import struct
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


CHUNK_NAMES: Dict[int, str] = {
    0x00000000: "padding",
    0x00134002: "solid_list_header",
    0x00134003: "solid_index_entries",
    0x00134004: "solid_stream_entries",
    0x00134011: "solid",
    0x00134012: "solid_texture_entries",
    0x00134013: "solid_light_material_entries",
    0x00134017: "solid_normal_smoother",
    0x00134018: "solid_smooth_vertices",
    0x00134019: "solid_smooth_vertex_infos",
    0x0013401A: "solid_position_markers",
    0x0013401D: "solid_morph_targets",
    0x0013401F: "solid_selection_sets",
    0x00134020: "solid_selection_edges",
    0x00134021: "solid_selection_edge_members",
    0x00134900: "solid_platform_info",
    0x00134B01: "solid_vertex_buffer",
    0x00134B02: "solid_mesh_entries",
    0x00134B03: "solid_index_buffer",
    0x00134C02: "solid_material_name",
    0x00134C04: "solid_platform_extra",
    0x00034101: "scenery_header",
    0x00034102: "scenery_infos",
    0x00034103: "scenery_instances",
    0x00034105: "scenery_tree_nodes",
    0x00034106: "scenery_override_hooks",
    0x00034107: "scenery_preculler_infos",
    0x00034108: "scenery_override_infos",
    0x00034109: "scenery_groups",
    0x0003410A: "scenery_ngbbs",
    0x0003410C: "model_hierarchy_instance",
    0x0003410D: "light_texture_collections",
    0x00034110: "track_streaming_sections",
    0x00034111: "track_streaming_infos",
    0x00034112: "track_streaming_barriers",
    0x00034113: "track_streaming_discs",
    0x00034146: "track_position_markers",
    0x00034148: "track_path_points",
    0x00034149: "track_path_lanes",
    0x0003414A: "track_path_zones",
    0x0003414D: "track_path_barriers",
    0x00034151: "visible_section_pack_header",
    0x00034152: "visible_section_boundaries",
    0x00034153: "visible_section_drivables",
    0x00034154: "visible_section_specifics",
    0x00034155: "visible_section_loadings",
    0x00034156: "visible_section_elev_polies",
    0x00034157: "visible_section_remap_table",
    0x00034158: "visible_section_overlay",
    0x00034201: "track_infos",
    0x0003B800: "world_road_network",
    0x0003B801: "world_collision_assets",
    0x0003B802: "world_grid_maker",
    0x0003B901: "collision_object",
    0x0003BC00: "emitter_system",
    0x33310001: "texture_pack_info_header",
    0x33310002: "texture_pack_info_entries",
    0x33310003: "texture_pack_info_stream",
    0x33310004: "texture_pack_info_textures",
    0x33310005: "texture_pack_info_plats",
    0x33312001: "texture_pack_anim_header",
    0x33312002: "texture_pack_anim_frames",
    0x33320001: "texture_pack_vram_header",
    0x33320002: "texture_pack_vram_data",
    0x80034100: "scenery_section",
    0x8003410B: "model_hierarchy_tree",
    0x80034147: "track_path_manager",
    0x80034150: "visible_section_manager",
    0x80036000: "event_trigger_pack",
    0x80134000: "geometry_pack",
    0x80134001: "solid_list_container",
    0x80134008: "solid_list_nested_chunks",
    0x80134010: "solid_container",
    0x80134100: "solid_platform_container",
    0x80135000: "light_source_pack",
    0xB3300000: "texture_pack",
    0xB3310000: "texture_pack_info",
    0xB3312000: "texture_pack_anim_pack",
    0xB3312004: "texture_pack_anim_inst",
    0xB3320000: "texture_pack_vram",
}

SCENERY_INFO_SIZE = 0x48
SCENERY_INSTANCE_SIZE = 0x60
SCENERY_TREE_NODE_SIZE = 0x30
LIGHT_TEXTURE_COLLECTION_SIZE = 0x20
INDEX_ENTRY_SIZE = 0x08
SOLID_LIST_HEADER_SIZE = 0x90
SOLID_SIZE = 0xE0
SOLID_KEY_OFFSET = 0x10
SOLID_POLY_COUNT_OFFSET = 0x14
SOLID_VERT_COUNT_OFFSET = 0x16
SOLID_BONE_COUNT_OFFSET = 0x18
SOLID_TEXTURE_COUNT_OFFSET = 0x19
SOLID_LIGHT_MATERIAL_COUNT_OFFSET = 0x1A
SOLID_POSITION_MARKER_COUNT_OFFSET = 0x1B
SOLID_BBOX_MIN_OFFSET = 0x20
SOLID_BBOX_MAX_OFFSET = 0x30
SOLID_NAME_OFFSET = 0xA0
SOLID_NAME_SIZE = 0x40
PLATFORM_INFO_SIZE = 0x34
PLATFORM_INFO_VERSION_OFFSET = 0x08
PLATFORM_INFO_CHUNKS_LOADED_OFFSET = 0x0A
PLATFORM_INFO_MESH_FLAGS_OFFSET = 0x0C
PLATFORM_INFO_SUBMESH_COUNT_OFFSET = 0x10
PLATFORM_INFO_POLYGON_COUNT_OFFSET = 0x2C
PLATFORM_INFO_VERTEX_COUNT_OFFSET = 0x30
MESH_ENTRY_SIZE = 0x90
MESH_ENTRY_BBOX_MIN_OFFSET = 0x00
MESH_ENTRY_BBOX_MAX_OFFSET = 0x0C
MESH_ENTRY_SHADER_TYPE_OFFSET = 0x30
MESH_ENTRY_FLAGS_OFFSET = 0x38
MESH_ENTRY_MATERIAL_KEY_OFFSET = 0x3C
MESH_ENTRY_VERTEX_COUNT_OFFSET = 0x40
MESH_ENTRY_TRIANGLE_COUNT_OFFSET = 0x60
MESH_ENTRY_INDEX_START_OFFSET = 0x64
MESH_ENTRY_INDEX_COUNT_OFFSET = 0x7C
MESH_ENTRY_VERTEX_OFFSET_OFFSET = 0x8C


def chunk_name(chunk_id: int) -> str:
    return CHUNK_NAMES.get(chunk_id, f"0x{chunk_id:08X}")


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


@dataclass
class ChunkNode:
    chunk_id: int
    offset: int
    size: int
    children: List["ChunkNode"]
    error: Optional[str] = None

    @property
    def name(self) -> str:
        return chunk_name(self.chunk_id)

    @property
    def end(self) -> int:
        return self.offset + 8 + self.size

    @property
    def is_container(self) -> bool:
        return (self.chunk_id & 0x80000000) != 0

    def to_dict(self) -> Dict[str, object]:
        result: Dict[str, object] = {
            "id": f"0x{self.chunk_id:08X}",
            "name": self.name,
            "offset": self.offset,
            "size": self.size,
        }
        if self.error:
            result["error"] = self.error
        if self.children:
            result["children"] = [child.to_dict() for child in self.children]
        return result


class ChunkParser:
    def parse(self, data: bytes) -> List[ChunkNode]:
        return self._parse_range(data, 0, len(data), 0)

    def _parse_range(self, data: bytes, start: int, end: int, depth: int) -> List[ChunkNode]:
        if depth > 16:
            return [ChunkNode(0, start, max(0, end - start), [], "max recursion depth exceeded")]

        nodes: List[ChunkNode] = []
        offset = start

        while offset + 8 <= end:
            chunk_id, size = struct.unpack_from("<II", data, offset)
            next_offset = offset + 8 + size

            if next_offset < offset + 8 or next_offset > end:
                nodes.append(
                    ChunkNode(
                        chunk_id=chunk_id,
                        offset=offset,
                        size=size,
                        children=[],
                        error=f"chunk exceeds parent range 0x{start:08X}-0x{end:08X}",
                    )
                )
                break

            children: List[ChunkNode] = []
            if (chunk_id & 0x80000000) != 0 and size != 0:
                children = self._parse_range(data, offset + 8, next_offset, depth + 1)

            nodes.append(ChunkNode(chunk_id=chunk_id, offset=offset, size=size, children=children))
            offset = next_offset

        if offset != end:
            nodes.append(
                ChunkNode(
                    chunk_id=0,
                    offset=offset,
                    size=max(0, end - offset - 8),
                    children=[],
                    error=f"trailing bytes: {end - offset}",
                )
            )

        return nodes


class CarbonTrackConverter:
    def __init__(
        self,
        input_dir: str,
        output_dir: str,
        track_id: str = "Unknown",
        bun_path: Optional[str] = None,
        full_geometry_details: bool = False,
    ):
        self.input_dir = Path(input_dir)
        self.output_dir = Path(output_dir)
        self.track_id = track_id
        self.bun_path = Path(bun_path) if bun_path else None
        self.full_geometry_details = full_geometry_details
        self.parser = ChunkParser()

    def convert(self) -> bool:
        if not self.input_dir.exists():
            print(f"Input directory not found: {self.input_dir}")
            return False

        self.output_dir.mkdir(parents=True, exist_ok=True)

        print(f"Inventorying Carbon track sections: {self.input_dir}")
        print(f"Output: {self.output_dir}")
        print(f"Track ID: {self.track_id}")
        print()

        sections = self._inventory_sections()
        bundles = self._inventory_bundles()

        report = {
            "track_id": self.track_id,
            "input_dir": str(self.input_dir),
            "output_dir": str(self.output_dir),
            "supports_mesh_export": False,
            "full_geometry_details": self.full_geometry_details,
            "reason": (
                "Carbon streamed world files are chunked GeometryPack/TPK/WCollision payloads. "
                "A generic 32-byte vertex decode is not valid for these sections."
            ),
            "section_count": len(sections),
            "sections_by_kind": dict(Counter(section["kind"] for section in sections)),
            "sections": sections,
            "bundles": bundles,
        }

        report_path = self.output_dir / f"{self.track_id}.section_manifest.json"
        with report_path.open("w", encoding="utf-8") as handle:
            json.dump(report, handle, indent=2)

        self._write_status_note(report_path)

        print(f"Wrote {report_path.name}")
        print()
        print("Section counts:")
        for kind, count in sorted(report["sections_by_kind"].items()):
            print(f"  {kind}: {count}")
        print()
        print("No SR3 mesh was written. Use the manifest to drive further reverse engineering.")
        return True

    def _inventory_sections(self) -> List[Dict[str, object]]:
        sections: List[Dict[str, object]] = []

        for data_file in sorted(self.input_dir.glob("*/DATA.BIN")):
            section_dir = data_file.parent
            settings = self._read_settings(section_dir / "Settings.end")
            data = data_file.read_bytes()
            nodes = self.parser.parse(data)
            summary = self._summarize_section(section_dir.name, data, nodes, settings)
            sections.append(summary)

        return sections

    def _inventory_bundles(self) -> List[Dict[str, object]]:
        candidates: List[Path] = []

        if self.bun_path and self.bun_path.exists():
            candidates.append(self.bun_path)

        auto_candidates = [
            self.input_dir.parent / f"{self.track_id}.BUN",
            self.input_dir.parent / f"STREAM{self.track_id}.BUN",
            self.input_dir.parent.parent / "TRACKS" / f"{self.track_id}.BUN",
            self.input_dir.parent.parent / "TRACKS" / f"STREAM{self.track_id}.BUN",
        ]

        for path in auto_candidates:
            if path.exists() and path not in candidates:
                candidates.append(path)

        bundles: List[Dict[str, object]] = []
        for path in candidates:
            data = path.read_bytes()
            nodes = self.parser.parse(data)
            top_non_padding = next((node for node in nodes if node.chunk_id != 0), None)
            bundles.append(
                {
                    "path": str(path),
                    "size": len(data),
                    "first_chunk": None
                    if top_non_padding is None
                    else {
                        "id": f"0x{top_non_padding.chunk_id:08X}",
                        "name": top_non_padding.name,
                        "size": top_non_padding.size,
                    },
                    "top_level_chunks": self._chunk_counter(nodes),
                }
            )

        return bundles

    def _read_settings(self, settings_path: Path) -> Dict[str, object]:
        if not settings_path.exists():
            return {}

        try:
            return json.loads(settings_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            return {"error": f"could not parse {settings_path.name}"}

    def _summarize_section(
        self,
        section_name: str,
        data: bytes,
        nodes: List[ChunkNode],
        settings: Dict[str, object],
    ) -> Dict[str, object]:
        first_non_padding = next((node for node in nodes if node.chunk_id != 0), None)
        kind = self._classify_section(section_name, first_non_padding)
        summary: Dict[str, object] = {
            "name": section_name,
            "prefix": section_name[:1],
            "size": len(data),
            "kind": kind,
            "settings": settings,
            "first_chunk": None
            if first_non_padding is None
            else {
                "id": f"0x{first_non_padding.chunk_id:08X}",
                "name": first_non_padding.name,
                "size": first_non_padding.size,
                "offset": first_non_padding.offset,
            },
            "top_level_chunks": self._chunk_counter(nodes),
        }

        geometry_summary = self._summarize_geometry_pack(data, nodes)
        if geometry_summary:
            summary["geometry_pack"] = geometry_summary

        collision_summary = self._summarize_collision_pack(nodes)
        if collision_summary:
            summary["collision_pack"] = collision_summary

        texture_summary = self._summarize_texture_pack(nodes)
        if texture_summary:
            summary["texture_pack"] = texture_summary

        return summary

    def _classify_section(self, section_name: str, first_non_padding: Optional[ChunkNode]) -> str:
        if first_non_padding is None:
            return "empty"

        mapping = {
            0x80134000: "geometry_pack",
            0x80034100: "geometry_pack",
            0xB3300000: "texture_pack",
            0x0003B801: "collision_pack",
            0x80135000: "light_source_pack",
            0x80036000: "event_trigger_pack",
            0x0003BC00: "emitter_system",
            0x00034112: "track_streaming_barriers",
        }
        return mapping.get(first_non_padding.chunk_id, f"unknown:{chunk_name(first_non_padding.chunk_id)}")

    def _chunk_counter(self, nodes: Iterable[ChunkNode]) -> Dict[str, int]:
        counter = Counter(node.name for node in nodes)
        return dict(sorted(counter.items()))

    def _payload_offset(self, node: ChunkNode, aligned: bool = False, alignment: int = 0x10) -> Optional[int]:
        offset = node.offset + 8
        if aligned:
            offset = align_up(offset, alignment)
        if offset > node.end:
            return None
        return offset

    def _payload_size(self, node: ChunkNode, aligned: bool = False, alignment: int = 0x10) -> int:
        offset = self._payload_offset(node, aligned=aligned, alignment=alignment)
        if offset is None:
            return 0
        return max(0, node.end - offset)

    def _read_u8(self, data: bytes, offset: Optional[int]) -> Optional[int]:
        if offset is None or offset < 0 or offset + 1 > len(data):
            return None
        return data[offset]

    def _read_u16(self, data: bytes, offset: Optional[int]) -> Optional[int]:
        if offset is None or offset < 0 or offset + 2 > len(data):
            return None
        return struct.unpack_from("<H", data, offset)[0]

    def _read_u32(self, data: bytes, offset: Optional[int]) -> Optional[int]:
        if offset is None or offset < 0 or offset + 4 > len(data):
            return None
        return struct.unpack_from("<I", data, offset)[0]

    def _read_vec3(self, data: bytes, offset: Optional[int]) -> Optional[List[float]]:
        if offset is None or offset < 0 or offset + 12 > len(data):
            return None
        return [round(value, 6) for value in struct.unpack_from("<fff", data, offset)]

    def _read_ascii(self, data: bytes, offset: Optional[int], length: int) -> Optional[str]:
        if offset is None or offset < 0 or offset + length > len(data):
            return None
        raw = data[offset : offset + length].split(b"\x00", 1)[0]
        if not raw:
            return None
        return raw.decode("ascii", errors="replace")

    def _hex32(self, value: Optional[int]) -> Optional[str]:
        if value is None:
            return None
        return f"0x{value:08X}"

    def _hex16(self, value: Optional[int]) -> Optional[str]:
        if value is None:
            return None
        return f"0x{value:04X}"

    def _find_first_descendant(self, nodes: Iterable[ChunkNode], chunk_id: int) -> Optional[ChunkNode]:
        for node in nodes:
            if node.chunk_id == chunk_id:
                return node
            child = self._find_first_descendant(node.children, chunk_id)
            if child is not None:
                return child
        return None

    def _find_children(self, node: ChunkNode, chunk_id: int) -> List[ChunkNode]:
        return [child for child in node.children if child.chunk_id == chunk_id]

    def _summarize_geometry_pack(self, data: bytes, nodes: List[ChunkNode]) -> Optional[Dict[str, object]]:
        geometry_pack = self._find_first_descendant(nodes, 0x80134000)
        if geometry_pack is None:
            return None

        scenery_section = self._find_first_descendant(nodes, 0x80034100)
        result: Dict[str, object] = {
            "geometry_pack_chunk_counts": dict(sorted(Counter(node.name for node in geometry_pack.children).items())),
            "scenery_section_present": scenery_section is not None,
        }

        solid_list_containers = self._find_children(geometry_pack, 0x80134001)
        solid_containers = self._find_children(geometry_pack, 0x80134010)
        result["solid_list_container_count"] = len(solid_list_containers)
        result["solid_container_count"] = len(solid_containers)

        if solid_list_containers:
            result["solid_list_headers"] = [
                self._summarize_solid_list_container(data, container) for container in solid_list_containers
            ]

        solids = [self._summarize_solid_container(data, container) for container in solid_containers]
        if solids:
            unique_solid_keys = sorted({solid["key"] for solid in solids if solid.get("key")})
            result["solid_count"] = len(solids)
            result["unique_solid_key_count"] = len(unique_solid_keys)
            result["mesh_entry_count_total"] = sum(
                solid.get("platform", {}).get("submesh_count", 0)
                for solid in solids
                if isinstance(solid.get("platform"), dict)
            )
            result["vertex_buffer_chunk_count_total"] = sum(
                solid.get("platform", {}).get("vertex_buffer_chunk_count", 0)
                for solid in solids
                if isinstance(solid.get("platform"), dict)
            )
            result["sample_local_solid_keys"] = unique_solid_keys[:16]
            material_names = sorted(
                {
                    material_name
                    for solid in solids
                    for material_name in solid.get("platform", {}).get("material_names", [])
                }
            )
            if material_names:
                result["sample_material_names"] = material_names[:16]

            if self.full_geometry_details:
                result["solids"] = solids
            else:
                result["solid_samples"] = solids[:12]
                result["solid_sample_count"] = min(12, len(solids))
                if len(solids) > 12:
                    result["solid_sample_truncated"] = True

        local_solid_keys = {solid["key"] for solid in solids if solid.get("key")}
        if local_solid_keys:
            result["local_solid_keys"] = sorted(local_solid_keys)

        if scenery_section is None:
            return result

        chunk_counts = Counter(node.name for node in scenery_section.children if node.chunk_id != 0)
        result["chunk_counts"] = dict(sorted(chunk_counts.items()))

        header = next((node for node in scenery_section.children if node.chunk_id == 0x00034101), None)
        if header is not None:
            data_start = header.offset + 8
            aligned_start = align_up(data_start, 0x10)
            if aligned_start + 16 <= len(data):
                candidates = []
                for offset in (12, 0, 4):
                    candidates.append(struct.unpack_from("<H", data, aligned_start + offset)[0])

                section_number = next((value for value in candidates if value != 0), 0)
                result["section_number"] = section_number

        infos = next((node for node in scenery_section.children if node.chunk_id == 0x00034102), None)
        if infos is not None:
            result.update(self._summarize_scenery_infos(data, infos, local_solid_keys))

        instances = next((node for node in scenery_section.children if node.chunk_id == 0x00034103), None)
        if instances is not None:
            data_start = instances.offset + 8
            aligned_start = align_up(data_start, 0x10)
            aligned_size = max(0, instances.size - (aligned_start - data_start))
            result["scenery_instance_count"] = aligned_size // SCENERY_INSTANCE_SIZE

        tree_nodes = next((node for node in scenery_section.children if node.chunk_id == 0x00034105), None)
        if tree_nodes is not None:
            data_start = tree_nodes.offset + 8
            aligned_start = align_up(data_start, 0x10)
            aligned_size = max(0, tree_nodes.size - (aligned_start - data_start))
            result["scenery_tree_node_count"] = aligned_size // SCENERY_TREE_NODE_SIZE

        light_tex = next((node for node in scenery_section.children if node.chunk_id == 0x0003410D), None)
        if light_tex is not None:
            result["light_texture_collection_count"] = light_tex.size // LIGHT_TEXTURE_COLLECTION_SIZE

        return result

    def _summarize_solid_list_container(self, data: bytes, container: ChunkNode) -> Dict[str, object]:
        result: Dict[str, object] = {
            "chunk_counts": self._chunk_counter(container.children),
        }

        header = next((child for child in container.children if child.chunk_id == 0x00134002), None)
        if header is not None:
            payload_offset = self._payload_offset(header)
            result["version"] = self._read_u32(data, payload_offset + 0x08 if payload_offset is not None else None)
            result["solid_count"] = self._read_u32(data, payload_offset + 0x0C if payload_offset is not None else None)
            result["filename"] = self._read_ascii(data, payload_offset + 0x10 if payload_offset is not None else None, 0x38)
            result["group_name"] = self._read_ascii(data, payload_offset + 0x48 if payload_offset is not None else None, 0x20)
            result["perm_chunk_byte_offset"] = self._read_u32(
                data, payload_offset + 0x68 if payload_offset is not None else None
            )
            result["perm_chunk_byte_size"] = self._read_u32(
                data, payload_offset + 0x6C if payload_offset is not None else None
            )
            result["max_solid_chunk_alignment"] = self._read_u16(
                data, payload_offset + 0x70 if payload_offset is not None else None
            )
            result["texture_pack_count"] = self._read_u16(
                data, payload_offset + 0x7C if payload_offset is not None else None
            )
            result["default_texture_count"] = self._read_u16(
                data, payload_offset + 0x7E if payload_offset is not None else None
            )

        index_entries = next((child for child in container.children if child.chunk_id == 0x00134003), None)
        if index_entries is not None:
            payload_offset = self._payload_offset(index_entries)
            entry_count = self._payload_size(index_entries) // INDEX_ENTRY_SIZE
            result["solid_index_entry_count"] = entry_count
            sample_entries: List[Dict[str, Optional[str]]] = []
            for index in range(min(entry_count, 8)):
                entry_offset = payload_offset + index * INDEX_ENTRY_SIZE if payload_offset is not None else None
                sample_entries.append(
                    {
                        "key": self._hex32(self._read_u32(data, entry_offset)),
                        "value": self._hex32(self._read_u32(data, entry_offset + 4 if entry_offset is not None else None)),
                    }
                )
            if sample_entries:
                result["sample_index_entries"] = sample_entries

        stream_entries = next((child for child in container.children if child.chunk_id == 0x00134004), None)
        if stream_entries is not None:
            result["solid_stream_entry_bytes"] = self._payload_size(stream_entries)

        return result

    def _summarize_solid_container(self, data: bytes, container: ChunkNode) -> Dict[str, object]:
        result: Dict[str, object] = {
            "chunk_counts": self._chunk_counter(container.children),
        }

        solid_node = next((child for child in container.children if child.chunk_id == 0x00134011), None)
        if solid_node is not None:
            result.update(self._summarize_solid_record(data, solid_node))

        platform_container = next((child for child in container.children if child.chunk_id == 0x80134100), None)
        if platform_container is not None:
            result["platform"] = self._summarize_platform_container(data, platform_container)

        return result

    def _summarize_solid_record(self, data: bytes, solid_node: ChunkNode) -> Dict[str, object]:
        payload_offset = self._payload_offset(solid_node, aligned=True)
        result: Dict[str, object] = {
            "payload_offset": payload_offset,
            "payload_size": self._payload_size(solid_node, aligned=True),
        }

        if payload_offset is None:
            result["error"] = "aligned solid payload is out of bounds"
            return result

        result["version"] = self._read_u8(data, payload_offset + 0x0C)
        result["flags"] = self._hex16(self._read_u16(data, payload_offset + 0x0E))
        result["key"] = self._hex32(self._read_u32(data, payload_offset + SOLID_KEY_OFFSET))
        result["poly_count"] = self._read_u16(data, payload_offset + SOLID_POLY_COUNT_OFFSET)
        result["vert_count"] = self._read_u16(data, payload_offset + SOLID_VERT_COUNT_OFFSET)
        result["bone_count"] = self._read_u8(data, payload_offset + SOLID_BONE_COUNT_OFFSET)
        result["texture_count"] = self._read_u8(data, payload_offset + SOLID_TEXTURE_COUNT_OFFSET)
        result["light_material_count"] = self._read_u8(data, payload_offset + SOLID_LIGHT_MATERIAL_COUNT_OFFSET)
        result["position_marker_count"] = self._read_u8(data, payload_offset + SOLID_POSITION_MARKER_COUNT_OFFSET)
        result["bbox_min"] = self._read_vec3(data, payload_offset + SOLID_BBOX_MIN_OFFSET)
        result["bbox_max"] = self._read_vec3(data, payload_offset + SOLID_BBOX_MAX_OFFSET)
        result["name"] = self._read_ascii(data, payload_offset + SOLID_NAME_OFFSET, SOLID_NAME_SIZE)
        return result

    def _summarize_platform_container(self, data: bytes, container: ChunkNode) -> Dict[str, object]:
        result: Dict[str, object] = {
            "chunk_counts": self._chunk_counter(container.children),
        }

        platform_info_node = next((child for child in container.children if child.chunk_id == 0x00134900), None)
        mesh_entries_node = next((child for child in container.children if child.chunk_id == 0x00134B02), None)
        index_buffer_node = next((child for child in container.children if child.chunk_id == 0x00134B03), None)
        vertex_buffer_nodes = self._find_children(container, 0x00134B01)
        material_name_nodes = self._find_children(container, 0x00134C02)
        extra_nodes = self._find_children(container, 0x00134C04)

        submesh_count = 0
        if platform_info_node is not None:
            payload_offset = self._payload_offset(platform_info_node, aligned=True)
            result["payload_offset"] = payload_offset
            result["payload_size"] = self._payload_size(platform_info_node, aligned=True)
            result["version"] = self._read_u16(
                data, payload_offset + PLATFORM_INFO_VERSION_OFFSET if payload_offset is not None else None
            )
            result["chunks_loaded_count"] = self._read_u8(
                data, payload_offset + PLATFORM_INFO_CHUNKS_LOADED_OFFSET if payload_offset is not None else None
            )
            result["mesh_flags"] = self._hex32(
                self._read_u32(data, payload_offset + PLATFORM_INFO_MESH_FLAGS_OFFSET if payload_offset is not None else None)
            )
            submesh_count = self._read_u32(
                data, payload_offset + PLATFORM_INFO_SUBMESH_COUNT_OFFSET if payload_offset is not None else None
            ) or 0
            result["submesh_count"] = submesh_count
            result["polygon_count"] = self._read_u32(
                data, payload_offset + PLATFORM_INFO_POLYGON_COUNT_OFFSET if payload_offset is not None else None
            )
            result["vertex_count"] = self._read_u32(
                data, payload_offset + PLATFORM_INFO_VERTEX_COUNT_OFFSET if payload_offset is not None else None
            )

        result["vertex_buffer_chunk_count"] = len(vertex_buffer_nodes)
        result["vertex_buffer_bytes"] = sum(self._payload_size(node, aligned=True) for node in vertex_buffer_nodes)
        result["material_name_count"] = len(material_name_nodes)
        result["platform_extra_chunk_count"] = len(extra_nodes)

        if index_buffer_node is not None:
            result["index_buffer_bytes"] = self._payload_size(index_buffer_node, aligned=True)
            result["index_buffer_index_count"] = self._payload_size(index_buffer_node, aligned=True) // 2

        mesh_entries = self._summarize_mesh_entries(
            data,
            mesh_entries_node,
            submesh_count,
            material_name_nodes,
            vertex_buffer_nodes,
        )
        if mesh_entries:
            result["mesh_entries"] = mesh_entries
            result["submesh_count"] = len(mesh_entries)
            result["shader_type_histogram"] = dict(
                sorted(Counter(entry["shader_type"] for entry in mesh_entries if entry.get("shader_type")).items())
            )
            material_names = sorted(
                {
                    entry["material_name"]
                    for entry in mesh_entries
                    if entry.get("material_name")
                }
            )
            if material_names:
                result["material_names"] = material_names

        return result

    def _summarize_mesh_entries(
        self,
        data: bytes,
        mesh_entries_node: Optional[ChunkNode],
        submesh_count: int,
        material_name_nodes: List[ChunkNode],
        vertex_buffer_nodes: List[ChunkNode],
    ) -> List[Dict[str, object]]:
        if mesh_entries_node is None:
            return []

        payload_offset = self._payload_offset(mesh_entries_node, aligned=True)
        if payload_offset is None:
            return []

        payload_size = self._payload_size(mesh_entries_node, aligned=True)
        available_count = payload_size // MESH_ENTRY_SIZE
        parse_count = min(submesh_count, available_count) if submesh_count else available_count

        material_names = [
            self._read_ascii(data, self._payload_offset(node), self._payload_size(node))
            for node in material_name_nodes
        ]
        vertex_buffers = [
            {
                "index": index,
                "offset": self._payload_offset(node, aligned=True),
                "size": self._payload_size(node, aligned=True),
            }
            for index, node in enumerate(vertex_buffer_nodes)
        ]

        entries: List[Dict[str, object]] = []
        for index in range(parse_count):
            base = payload_offset + index * MESH_ENTRY_SIZE
            entry: Dict[str, object] = {
                "index": index,
                "bbox_min": self._read_vec3(data, base + MESH_ENTRY_BBOX_MIN_OFFSET),
                "bbox_max": self._read_vec3(data, base + MESH_ENTRY_BBOX_MAX_OFFSET),
                "shader_type": self._hex32(self._read_u32(data, base + MESH_ENTRY_SHADER_TYPE_OFFSET)),
                "flags": self._hex32(self._read_u32(data, base + MESH_ENTRY_FLAGS_OFFSET)),
                "material_key": self._hex32(self._read_u32(data, base + MESH_ENTRY_MATERIAL_KEY_OFFSET)),
                "vertex_count": self._read_u32(data, base + MESH_ENTRY_VERTEX_COUNT_OFFSET),
                "triangle_count": self._read_u32(data, base + MESH_ENTRY_TRIANGLE_COUNT_OFFSET),
                "index_start": self._read_u32(data, base + MESH_ENTRY_INDEX_START_OFFSET),
                "index_count": self._read_u32(data, base + MESH_ENTRY_INDEX_COUNT_OFFSET),
                "vertex_offset": self._read_u32(data, base + MESH_ENTRY_VERTEX_OFFSET_OFFSET),
            }
            if index < len(material_names) and material_names[index]:
                entry["material_name"] = material_names[index]
            entries.append(entry)

        vertex_group_index = -1
        last_shader_type: Optional[str] = None
        for entry in entries:
            shader_type = entry.get("shader_type")
            if shader_type != last_shader_type:
                vertex_group_index += 1
                last_shader_type = shader_type
            entry["vertex_buffer_group"] = vertex_group_index
            if 0 <= vertex_group_index < len(vertex_buffers):
                entry["vertex_buffer_bytes"] = vertex_buffers[vertex_group_index]["size"]
                entry["vertex_buffer_offset"] = vertex_buffers[vertex_group_index]["offset"]

        return entries

    def _summarize_scenery_infos(
        self,
        data: bytes,
        infos: ChunkNode,
        local_solid_keys: Iterable[str],
    ) -> Dict[str, object]:
        info_count = infos.size // SCENERY_INFO_SIZE
        result: Dict[str, object] = {
            "scenery_info_count": info_count,
        }

        local_solid_key_set = set(local_solid_keys)
        solid_key_counter: Counter[str] = Counter()
        sample_infos: List[Dict[str, object]] = []
        sample_names: List[str] = []

        for index in range(info_count):
            base = infos.offset + 8 + index * SCENERY_INFO_SIZE
            if base + SCENERY_INFO_SIZE > len(data):
                break

            debug_name = self._read_ascii(data, base, 0x18)
            if debug_name and len(sample_names) < 8:
                sample_names.append(debug_name)

            key_values = struct.unpack_from("<IIII", data, base + 0x18)
            solid_keys = [self._hex32(key) for key in key_values if key]
            for key in solid_keys:
                if key is not None:
                    solid_key_counter[key] += 1

            if len(sample_infos) < 12:
                sample_infos.append(
                    {
                        "debug_name": debug_name,
                        "solid_keys": solid_keys,
                        "resolved_in_section": [key in local_solid_key_set for key in solid_keys],
                    }
                )

        if sample_names:
            result["sample_debug_names"] = sample_names
        if sample_infos:
            result["sample_scenery_infos"] = sample_infos
        if solid_key_counter:
            result["scenery_info_total_solid_references"] = sum(solid_key_counter.values())
            result["scenery_info_unique_solid_key_count"] = len(solid_key_counter)
            result["sample_solid_keys"] = [
                {
                    "key": key,
                    "count": count,
                    "resolved_in_section": key in local_solid_key_set,
                }
                for key, count in solid_key_counter.most_common(16)
            ]

            resolved_unique = sorted(key for key in solid_key_counter if key in local_solid_key_set)
            unresolved_unique = sorted(key for key in solid_key_counter if key not in local_solid_key_set)
            result["solid_key_resolution"] = {
                "resolved_unique_key_count": len(resolved_unique),
                "unresolved_unique_key_count": len(unresolved_unique),
                "resolved_reference_count": sum(solid_key_counter[key] for key in resolved_unique),
                "unresolved_reference_count": sum(solid_key_counter[key] for key in unresolved_unique),
                "sample_resolved_keys": resolved_unique[:12],
                "sample_unresolved_keys": unresolved_unique[:12],
            }

        return result

    def _summarize_collision_pack(self, nodes: List[ChunkNode]) -> Optional[Dict[str, object]]:
        collision_nodes = [node for node in nodes if node.chunk_id == 0x0003B801]
        if not collision_nodes:
            return None

        return {
            "collision_block_count": len(collision_nodes),
            "sample_sizes": [node.size for node in collision_nodes[:16]],
        }

    def _summarize_texture_pack(self, nodes: List[ChunkNode]) -> Optional[Dict[str, object]]:
        texture_pack = self._find_first_descendant(nodes, 0xB3300000)
        if texture_pack is None:
            return None

        info_block = self._find_first_descendant([texture_pack], 0xB3310000)
        summary: Dict[str, object] = {
            "top_level_chunk_counts": dict(sorted(Counter(node.name for node in texture_pack.children).items())),
        }

        if info_block is not None:
            summary["info_block_size"] = info_block.size
            summary["info_chunk_counts"] = dict(
                sorted(Counter(node.name for node in info_block.children if node.chunk_id != 0).items())
            )

        return summary

    def _write_status_note(self, report_path: Path) -> None:
        note_path = self.output_dir / "README.txt"
        note = (
            "Carbon track export status\n"
            "==========================\n\n"
            "This output contains an inventory report, not decoded SR3 geometry.\n"
            "Use the JSON manifest to inspect GeometryPack, TPK, and WCollision sections.\n\n"
            f"Report: {report_path.name}\n"
        )
        note_path.write_text(note, encoding="utf-8")


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__)
        return 1

    input_dir = sys.argv[1]
    output_dir = sys.argv[2]
    track_id = "Unknown"
    bun_path = None
    full_geometry_details = False

    index = 3
    while index < len(sys.argv):
        arg = sys.argv[index]
        if arg == "--bun" and index + 1 < len(sys.argv):
            bun_path = sys.argv[index + 1]
            index += 2
        elif arg == "--full-geometry-details":
            full_geometry_details = True
            index += 1
        elif not arg.startswith("--"):
            track_id = arg
            index += 1
        else:
            index += 1

    converter = CarbonTrackConverter(input_dir, output_dir, track_id, bun_path, full_geometry_details)
    return 0 if converter.convert() else 1


if __name__ == "__main__":
    raise SystemExit(main())
