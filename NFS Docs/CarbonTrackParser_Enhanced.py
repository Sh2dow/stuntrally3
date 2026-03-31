#!/usr/bin/env python3
"""
Enhanced Carbon track parser.

This tool is the baseline parser for Carbon track research in this repo.

What it does well:
- discovers FE track assets under TRACKS/<REGION>/
- inventories real region bundle chunks from TRACKS/<REGION>.BUN
- inventories real stream bundle metadata from TRACKS/STREAM<REGION>.BUN
- parses TrackStreamingSections records from the region bundle
- exports SR3-facing XML plus a detailed JSON research report

What it does not pretend to do:
- it does not fully decode TrackMaps.bin
- it does not fully decode TroughBoundary.bin
- it does not invent a fake TrackPath.bin sidecar
- it does not convert Carbon geometry/material/collision content into SR3

Usage:
    python CarbonTrackParser_Enhanced.py <carbon_tracks_dir> <output_dir> [region]

Examples:
    python CarbonTrackParser_Enhanced.py "D:\\Games\\NFSC Redux\\TRACKS" "data\\tracks" L5RA
    python CarbonTrackParser_Enhanced.py "D:\\Games\\NFSC Redux\\TRACKS" "data\\tracks"
"""

from __future__ import annotations

import json
import struct
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# Carbon track_path::zone::type from hyperlinked/track_path.hpp
CARBON_ZONE_TYPES = {
    -1: "none",
    0: "reset",
    1: "reset_to_point",
    2: "guided_reset",
    3: "tunnel",
    4: "overpass",
    5: "overpass_small",
    6: "streamer_prediction",
    7: "garage",
    8: "hidden",
    9: "traffic_pattern",
    10: "dynamic",
    11: "neighborhood",
    12: "jump_camera",
    13: "no_cop_spawn",
    14: "pursuit_start",
    15: "highway",
    16: "canyon_drop",
    17: "vertigo_camera",
}

CARBON_CHUNK_IDS = {
    0x00034110: "track_streaming_sections",
    0x00034111: "track_streaming_infos",
    0x00034112: "track_streaming_barriers",
    0x00034113: "track_streaming_discs",
    0x80034147: "track_path_manager",
    0x00034148: "track_path_points",
    0x00034149: "track_path_lanes",
    0x0003414A: "track_path_zones",
    0x0003414D: "track_path_barriers",
    0x0003B800: "world_road_network",
}

TRACK_STREAMING_SECTIONS_CHUNK = 0x00034110
STREAMING_SECTION_SIZE = 0x5C
STREAMING_SECTION_STRUCT = struct.Struct("<QhBBiiIiiiifffIiIiiiiiii")


@dataclass
class CarbonTrackZone:
    zone_type: int = 0
    position: Tuple[float, float, float] = (0.0, 0.0, 0.0)
    radius: float = 10.0
    length: float = 0.0
    group_key: int = 0
    enabled: bool = True
    params: Dict[str, str] = field(default_factory=dict)


@dataclass
class CarbonTrackBarrier:
    start: Tuple[float, float, float] = (0.0, 0.0, 0.0)
    end: Tuple[float, float, float] = (0.0, 0.0, 0.0)
    height: float = 3.0
    group_key: int = 0
    player_only: bool = False
    handedness: int = 0
    enabled: bool = True


@dataclass
class CarbonBundleChunk:
    source_file: str
    chunk_id: int
    chunk_name: str
    offset: int
    size: int


@dataclass
class CarbonStreamingSection:
    name: str
    section_number: int
    was_rendered: int
    currently_visible: int
    status: int
    file_type: int
    file_offset: int
    size: int
    compressed_size: int
    permanent_size: int
    section_priority: int
    center_x: float
    center_y: float
    radius: float
    checksum: int
    last_needed_timestamp: int
    unactivated_frame_count: int
    loaded_time: int
    base_loading_priority: int
    loading_priority: int
    memory_pointer: int
    disc_bundle_pointer: int
    loaded_size: int
    boundary_pointer: int
    stream_data_in_bounds: Optional[bool] = None


@dataclass
class CarbonTrackInfo:
    track_id: str = ""
    display_name: str = ""
    art_name: str = ""
    region: str = ""

    engage_pos: Tuple[float, float, float] = (0.0, 0.0, 0.0)
    engage_yaw: float = 0.0

    minimap_locked: str = ""
    minimap_unlocked: str = ""
    track_map: str = ""
    track_map_size: int = 0
    track_map_header: str = ""

    length: float = 0.0
    difficulty: int = 1
    event_types: int = 0

    is_unlocked: bool = True
    district_id: int = 0

    zones: List[CarbonTrackZone] = field(default_factory=list)
    barriers: List[CarbonTrackBarrier] = field(default_factory=list)

    boundary_file: str = ""
    boundary_file_size: int = 0
    boundary_header: str = ""

    region_bundle: str = ""
    region_bundle_size: int = 0
    stream_bundle: str = ""
    stream_bundle_size: int = 0

    total_bundle_chunks_scanned: int = 0
    known_chunks: List[CarbonBundleChunk] = field(default_factory=list)
    streaming_sections: List[CarbonStreamingSection] = field(default_factory=list)

    warnings: List[str] = field(default_factory=list)


class CarbonTrackParser:
    """Parser for Carbon FE track assets and validated bundle metadata."""

    def __init__(self, carbon_tracks_dir: str):
        self.carbon_dir = Path(carbon_tracks_dir)
        self.tracks: Dict[str, CarbonTrackInfo] = {}

    def parse_region(self, region: str) -> Optional[CarbonTrackInfo]:
        region_path = self.carbon_dir / region

        if not region_path.exists():
            print(f"  Region not found: {region}")
            return None

        track = CarbonTrackInfo(track_id=region, region=region)
        track.display_name = self._region_to_name(region)
        track.art_name = region.lower()

        self._discover_fe_assets(region_path, track)
        self._discover_bundle_assets(region, track)

        self.tracks[region] = track
        return track

    def _discover_fe_assets(self, region_path: Path, track: CarbonTrackInfo) -> None:
        region = track.region

        track_maps = region_path / "TrackMaps.bin"
        if track_maps.exists():
            track.track_map = f"TRACKS/{region}/TrackMaps.bin"
            track.track_map_size = track_maps.stat().st_size
            track.track_map_header = self._read_header_hex(track_maps)
            print(f"  Found TrackMaps.bin ({track.track_map_size} bytes)")

        for file in sorted(region_path.glob("MINI_MAP_*.bin")):
            rel = f"TRACKS/{region}/{file.name}"
            if "UNLOCKED" in file.name:
                track.minimap_unlocked = rel
                print(f"  Found unlocked minimap: {file.name}")
            elif "LOCKED" in file.name:
                track.minimap_locked = rel
                print(f"  Found locked minimap: {file.name}")

        boundary_file = region_path / "TroughBoundary.bin"
        if boundary_file.exists():
            track.boundary_file = f"TRACKS/{region}/TroughBoundary.bin"
            track.boundary_file_size = boundary_file.stat().st_size
            track.boundary_header = self._read_header_hex(boundary_file)
            print(f"  Found TroughBoundary.bin ({track.boundary_file_size} bytes)")
            track.warnings.append(
                "TroughBoundary.bin was only inventoried. Its binary layout is not yet validated in this parser."
            )

    def _discover_bundle_assets(self, region: str, track: CarbonTrackInfo) -> None:
        region_bundle = self.carbon_dir / f"{region}.BUN"
        stream_bundle = self.carbon_dir / f"STREAM{region}.BUN"

        if region_bundle.exists():
            track.region_bundle = f"TRACKS/{region}.BUN"
            track.region_bundle_size = region_bundle.stat().st_size
            print(f"  Found region bundle: {region_bundle.name} ({track.region_bundle_size} bytes)")
        if stream_bundle.exists():
            track.stream_bundle = f"TRACKS/STREAM{region}.BUN"
            track.stream_bundle_size = stream_bundle.stat().st_size
            print(f"  Found stream bundle: {stream_bundle.name} ({track.stream_bundle_size} bytes)")

        if region_bundle.exists():
            self._scan_region_bundle(region_bundle, track)
        elif stream_bundle.exists():
            track.warnings.append(
                f"Found {stream_bundle.name} but no matching {region}.BUN region bundle to describe its section table."
            )

    def _scan_region_bundle(self, region_bundle: Path, track: CarbonTrackInfo) -> None:
        file_size = region_bundle.stat().st_size
        scanned_chunks = 0

        with open(region_bundle, "rb") as f:
            while f.tell() + 8 <= file_size:
                offset = f.tell()
                header = f.read(8)
                if len(header) < 8:
                    break

                chunk_id, chunk_size = struct.unpack("<II", header)
                data_offset = f.tell()

                if data_offset + chunk_size > file_size:
                    track.warnings.append(
                        f"Stopped bundle scan at 0x{offset:08X}: chunk 0x{chunk_id:08X} size 0x{chunk_size:X} exceeds file bounds."
                    )
                    break

                scanned_chunks += 1

                if chunk_id in CARBON_CHUNK_IDS:
                    chunk_name = CARBON_CHUNK_IDS[chunk_id]
                    track.known_chunks.append(
                        CarbonBundleChunk(
                            source_file=region_bundle.name,
                            chunk_id=chunk_id,
                            chunk_name=chunk_name,
                            offset=offset,
                            size=chunk_size,
                        )
                    )

                    if chunk_id == TRACK_STREAMING_SECTIONS_CHUNK:
                        chunk_data = f.read(chunk_size)
                        self._parse_streaming_sections_chunk(chunk_data, track)
                        continue

                f.seek(chunk_size, 1)

        track.total_bundle_chunks_scanned = scanned_chunks

        if not track.known_chunks:
            track.warnings.append(
                f"No known Carbon track chunks were discovered while scanning {region_bundle.name}."
            )

    def _parse_streaming_sections_chunk(self, chunk_data: bytes, track: CarbonTrackInfo) -> None:
        if len(chunk_data) % STREAMING_SECTION_SIZE != 0:
            track.warnings.append(
                f"TrackStreamingSections chunk size 0x{len(chunk_data):X} is not aligned to 0x{STREAMING_SECTION_SIZE:X}."
            )
            return

        stream_size = track.stream_bundle_size if track.stream_bundle_size > 0 else None
        count = len(chunk_data) // STREAMING_SECTION_SIZE
        print(f"    Parsed {count} streaming section records")

        for index in range(count):
            offset = index * STREAMING_SECTION_SIZE
            entry = chunk_data[offset:offset + STREAMING_SECTION_SIZE]
            values = STREAMING_SECTION_STRUCT.unpack(entry)

            raw_name = entry[:8].split(b"\x00", 1)[0].decode("ascii", errors="ignore")
            section = CarbonStreamingSection(
                name=raw_name,
                section_number=values[1],
                was_rendered=values[2],
                currently_visible=values[3],
                status=values[4],
                file_type=values[5],
                file_offset=values[6],
                size=values[7],
                compressed_size=values[8],
                permanent_size=values[9],
                section_priority=values[10],
                center_x=values[11],
                center_y=values[12],
                radius=values[13],
                checksum=values[14],
                last_needed_timestamp=values[15],
                unactivated_frame_count=values[16],
                loaded_time=values[17],
                base_loading_priority=values[18],
                loading_priority=values[19],
                memory_pointer=values[20],
                disc_bundle_pointer=values[21],
                loaded_size=values[22],
                boundary_pointer=values[23],
            )

            if stream_size is not None:
                section.stream_data_in_bounds = section.file_offset + section.size <= stream_size

            track.streaming_sections.append(section)

    def export_to_sr3(self, output_dir: str, track: CarbonTrackInfo) -> Path:
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)

        root = ET.Element("track")
        root.set("id", track.track_id)
        root.set("displayName", track.display_name)
        root.set("artName", track.art_name)
        root.set("region", track.region)

        engage_elem = ET.SubElement(root, "engagePos")
        engage_elem.set("pos", f"{track.engage_pos[0]}, {track.engage_pos[1]}, {track.engage_pos[2]}")
        engage_elem.set("yaw", str(track.engage_yaw))

        minimap_elem = ET.SubElement(root, "minimap")
        minimap_elem.set("locked", track.minimap_locked)
        minimap_elem.set("unlocked", track.minimap_unlocked)

        if track.track_map:
            root.set("trackMap", track.track_map)

        stats_elem = ET.SubElement(root, "stats")
        stats_elem.set("length", str(track.length))
        stats_elem.set("difficulty", str(track.difficulty))
        stats_elem.set("eventTypes", str(track.event_types))

        prog_elem = ET.SubElement(root, "progression")
        prog_elem.set("isUnlocked", str(track.is_unlocked).lower())
        prog_elem.set("districtId", str(track.district_id))

        source_elem = ET.SubElement(root, "carbonSource")
        source_elem.set("trackMapsPresent", str(bool(track.track_map)).lower())
        source_elem.set("boundaryPresent", str(bool(track.boundary_file)).lower())
        source_elem.set("regionBundlePresent", str(bool(track.region_bundle)).lower())
        source_elem.set("streamBundlePresent", str(bool(track.stream_bundle)).lower())
        source_elem.set("knownTrackChunks", str(len(track.known_chunks)))
        source_elem.set("streamingSections", str(len(track.streaming_sections)))

        if track.zones:
            zones_elem = ET.SubElement(root, "zones")
            for zone in track.zones:
                zone_elem = ET.SubElement(zones_elem, "zone")
                zone_elem.set("type", self._zone_type_name(zone.zone_type))
                zone_elem.set("pos", f"{zone.position[0]}, {zone.position[1]}, {zone.position[2]}")
                zone_elem.set("radius", str(zone.radius))
                zone_elem.set("length", str(zone.length))
                zone_elem.set("group", str(zone.group_key))
                zone_elem.set("enabled", str(zone.enabled).lower())

                for key, value in zone.params.items():
                    param_elem = ET.SubElement(zone_elem, "param")
                    param_elem.set("key", key)
                    param_elem.set("value", value)

        if track.barriers:
            barriers_elem = ET.SubElement(root, "barriers")
            for barrier in track.barriers:
                barrier_elem = ET.SubElement(barriers_elem, "barrier")
                barrier_elem.set("start", f"{barrier.start[0]}, {barrier.start[1]}, {barrier.start[2]}")
                barrier_elem.set("end", f"{barrier.end[0]}, {barrier.end[1]}, {barrier.end[2]}")
                barrier_elem.set("height", str(barrier.height))
                barrier_elem.set("group", str(barrier.group_key))
                barrier_elem.set("playerOnly", str(barrier.player_only).lower())
                barrier_elem.set("handedness", str(barrier.handedness))
                barrier_elem.set("enabled", str(barrier.enabled).lower())

        tree = ET.ElementTree(root)
        ET.indent(tree, space="\t", level=0)

        output_file = output_path / f"{track.track_id}.xml"
        tree.write(output_file, encoding="utf-8", xml_declaration=True)
        print(f"  Exported XML: {output_file}")
        return output_file

    def export_to_s3(self, output_dir: str, track: CarbonTrackInfo) -> Path:
        """Compatibility alias for older docs/scripts."""
        return self.export_to_sr3(output_dir, track)

    def export_report(self, output_dir: str, track: CarbonTrackInfo) -> Path:
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)

        report = {
            "track_id": track.track_id,
            "display_name": track.display_name,
            "art_name": track.art_name,
            "region": track.region,
            "files": {
                "track_map": track.track_map,
                "track_map_size": track.track_map_size,
                "track_map_header": track.track_map_header,
                "minimap_locked": track.minimap_locked,
                "minimap_unlocked": track.minimap_unlocked,
                "boundary_file": track.boundary_file,
                "boundary_file_size": track.boundary_file_size,
                "boundary_header": track.boundary_header,
                "region_bundle": track.region_bundle,
                "region_bundle_size": track.region_bundle_size,
                "stream_bundle": track.stream_bundle,
                "stream_bundle_size": track.stream_bundle_size,
            },
            "bundle_scan": {
                "total_chunks_scanned": track.total_bundle_chunks_scanned,
                "known_chunks": [
                    {
                        "source_file": chunk.source_file,
                        "chunk_id": f"0x{chunk.chunk_id:08X}",
                        "chunk_name": chunk.chunk_name,
                        "offset": f"0x{chunk.offset:08X}",
                        "size": chunk.size,
                    }
                    for chunk in track.known_chunks
                ],
            },
            "streaming_sections": [
                {
                    "name": section.name,
                    "section_number": section.section_number,
                    "status": section.status,
                    "file_type": section.file_type,
                    "file_offset": section.file_offset,
                    "size": section.size,
                    "compressed_size": section.compressed_size,
                    "permanent_size": section.permanent_size,
                    "section_priority": section.section_priority,
                    "center_x": section.center_x,
                    "center_y": section.center_y,
                    "radius": section.radius,
                    "checksum": f"0x{section.checksum:08X}",
                    "stream_data_in_bounds": section.stream_data_in_bounds,
                }
                for section in track.streaming_sections
            ],
            "parsed_semantics": {
                "zones": len(track.zones),
                "barriers": len(track.barriers),
            },
            "warnings": track.warnings,
        }

        output_file = output_path / f"{track.track_id}.report.json"
        with open(output_file, "w", encoding="utf-8") as f:
            json.dump(report, f, indent=2)
        print(f"  Exported report: {output_file}")
        return output_file

    def create_track_database(self, output_dir: str) -> Path:
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)

        root = ET.Element("trackDatabase")
        root.set("comment", "Generated by CarbonTrackParser_Enhanced.py")
        root.set("generated", str(output_path.absolute()))

        for track_id in sorted(self.tracks.keys()):
            track = self.tracks[track_id]
            track_elem = ET.SubElement(root, "track")
            track_elem.set("id", track.track_id)
            track_elem.set("file", f"{track.track_id}.xml")
            track_elem.set("region", track.region)

        tree = ET.ElementTree(root)
        ET.indent(tree, space="\t", level=0)

        output_file = output_path / "TrackDatabase.xml"
        tree.write(output_file, encoding="utf-8", xml_declaration=True)
        print(f"Created track database: {output_file}")
        return output_file

    def _region_to_name(self, region: str) -> str:
        region_names = {
            "L5RA": "Casino Tower",
            "L5RB": "South Junction",
            "L4RA": "Ocean Hills",
            "L4RB": "Eagle Drive",
            "L3RA": "Fortuna",
            "L3RB": "Beacon Hill",
            "L2RA": "Heritage Heights",
            "L2RB": "Downtown",
            "L1RA": "Rosewood",
            "L1RB": "Palmont",
        }
        return region_names.get(region, region)

    def _read_header_hex(self, file_path: Path, byte_count: int = 16) -> str:
        with open(file_path, "rb") as f:
            return f.read(byte_count).hex()

    def _zone_type_name(self, zone_type: int) -> str:
        return CARBON_ZONE_TYPES.get(zone_type, f"unknown_{zone_type}")


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__)
        print("\nUsage: python CarbonTrackParser_Enhanced.py <carbon_tracks_dir> <output_dir> [region]")
        print("\nExamples:")
        print('  python CarbonTrackParser_Enhanced.py "D:\\Games\\NFSC Redux\\TRACKS" "data\\tracks" L5RA')
        print('  python CarbonTrackParser_Enhanced.py "D:\\Games\\NFSC Redux\\TRACKS" "data\\tracks"')
        return 1

    carbon_dir = sys.argv[1]
    output_dir = sys.argv[2]
    region = sys.argv[3] if len(sys.argv) > 3 else None

    print("Carbon Track Parser (Enhanced)")
    print("==============================")
    print(f"Carbon tracks: {carbon_dir}")
    print(f"Output dir: {output_dir}")

    parser = CarbonTrackParser(carbon_dir)

    if region:
        print(f"\nParsing region: {region}")
        track = parser.parse_region(region)
        if track:
            parser.export_to_sr3(output_dir, track)
            parser.export_report(output_dir, track)
    else:
        print("\nParsing all regions...")
        for region_dir in sorted(Path(carbon_dir).iterdir()):
            if region_dir.is_dir():
                print(f"\nParsing region: {region_dir.name}")
                track = parser.parse_region(region_dir.name)
                if track:
                    parser.export_to_sr3(output_dir, track)
                    parser.export_report(output_dir, track)

    if parser.tracks:
        parser.create_track_database(output_dir)
        print(f"\nParsed {len(parser.tracks)} track(s)")
        print(f"  - Known track chunks: {sum(len(t.known_chunks) for t in parser.tracks.values())}")
        print(f"  - Streaming sections: {sum(len(t.streaming_sections) for t in parser.tracks.values())}")
    else:
        print("\nNo tracks found to parse")

    return 0


if __name__ == "__main__":
    sys.exit(main())
