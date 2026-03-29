#!/usr/bin/env python3
"""
Carbon Track Metadata Parser

Parses Carbon track data files and exports to SR3-compatible XML format.

Supported Carbon files:
- TRACKS\<REGION>\TrackMaps.bin - FE preview maps
- TRACKS\<REGION>\MINI_MAP_*_LOCKED.bin - Locked minimaps
- TRACKS\<REGION>\MINI_MAP_*_UNLOCKED.bin - Unlocked minimaps
- TRACKS\<REGION>\TroughBoundary.bin - Track boundaries

This tool extracts metadata and converts it to SR3's TrackInfo XML format.
It does NOT modify or load Carbon bundles at runtime.

Usage:
    python CarbonTrackParser.py <carbon_tracks_dir> <output_dir> [region]
    
Example:
    python CarbonTrackParser.py "D:\Games\NFSC Redux\TRACKS" "data\tracks" L5RA
"""

import os
import sys
import struct
import xml.etree.ElementTree as ET
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple


# Carbon Zone Types (from track_path::zone::type)
CARBON_ZONE_TYPES = {
    0: "reset",
    1: "guided_reset",
    2: "tunnel",
    3: "overpass",
    4: "streamer_prediction",
    5: "garage",
    6: "traffic_pattern",
    7: "dynamic",
    8: "neighborhood",
    9: "jump_camera",
    10: "no_cop_spawn",
    11: "pursuit_start",
    12: "highway",
    13: "canyon_drop",
    14: "vertigo_camera",
}

# Carbon chunk IDs (from hyperlinked)
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


@dataclass
class CarbonTrackZone:
    """Represents a Carbon track zone"""
    zone_type: int = 0
    position: Tuple[float, float, float] = (0, 0, 0)
    radius: float = 10.0
    length: float = 0.0
    group_key: int = 0
    enabled: bool = True
    params: Dict[str, str] = field(default_factory=dict)


@dataclass
class CarbonTrackBarrier:
    """Represents a Carbon track barrier"""
    start: Tuple[float, float, float] = (0, 0, 0)
    end: Tuple[float, float, float] = (0, 0, 0)
    height: float = 3.0
    group_key: int = 0
    player_only: bool = False
    handedness: int = 0
    enabled: bool = True


@dataclass
class CarbonTrackInfo:
    """Represents Carbon track metadata"""
    track_id: str = ""
    display_name: str = ""
    art_name: str = ""
    region: str = ""
    
    engage_pos: Tuple[float, float, float] = (0, 0, 0)
    engage_yaw: float = 0.0
    
    minimap_locked: str = ""
    minimap_unlocked: str = ""
    track_map: str = ""
    
    length: float = 0.0
    difficulty: int = 1
    event_types: int = 0
    
    is_unlocked: bool = True
    district_id: int = 0
    
    zones: List[CarbonTrackZone] = field(default_factory=list)
    barriers: List[CarbonTrackBarrier] = field(default_factory=list)


class CarbonTrackParser:
    """Parser for Carbon track data files"""
    
    def __init__(self, carbon_tracks_dir: str):
        self.carbon_dir = Path(carbon_tracks_dir)
        self.tracks: Dict[str, CarbonTrackInfo] = {}
    
    def parse_region(self, region: str) -> Optional[CarbonTrackInfo]:
        """Parse track data for a specific Carbon region"""
        region_path = self.carbon_dir / region
        
        if not region_path.exists():
            print(f"Region not found: {region}")
            return None
        
        track = CarbonTrackInfo()
        track.track_id = region
        track.region = region
        
        # Find TrackMaps.bin
        track_maps = region_path / "TrackMaps.bin"
        if track_maps.exists():
            track.track_map = f"TRACKS/{region}/TrackMaps.bin"
            print(f"  Found TrackMaps.bin")
        
        # Find minimap files
        for file in region_path.glob("MINI_MAP_*.bin"):
            filename = file.name
            if "LOCKED" in filename:
                track.minimap_locked = f"TRACKS/{region}/{filename}"
                print(f"  Found locked minimap: {filename}")
            elif "UNLOCKED" in filename:
                track.minimap_unlocked = f"TRACKS/{region}/{filename}"
                print(f"  Found unlocked minimap: {filename}")
        
        # Find boundary file (contains zones/barriers)
        boundary_file = region_path / "TroughBoundary.bin"
        if boundary_file.exists():
            print(f"  Found TroughBoundary.bin - parsing zones/barriers")
            self._parse_boundary_file(boundary_file, track)
        
        # Set display name from region code
        track.display_name = self._region_to_name(region)
        track.art_name = region.lower()
        
        self.tracks[region] = track
        return track
    
    def _region_to_name(self, region: str) -> str:
        """Convert region code to display name"""
        # Simple conversion - can be expanded with actual Carbon names
        region_names = {
            "L5RA": "Casino Tower",
            "L5RB": "South Junction",
            "L3RA": "Fortuna",
            "L3RB": "Beacon Hill",
            "L1RA": "Rosewood",
            "L1RB": "Palmont",
        }
        return region_names.get(region, region)
    
    def _parse_boundary_file(self, file: Path, track: CarbonTrackInfo):
        """Parse Carbon boundary file for zones and barriers"""
        try:
            with open(file, 'rb') as f:
                # Read header
                header = f.read(4)
                if len(header) < 4:
                    return
                
                # Try to detect file format
                # Carbon boundary files have various formats
                # This is a simplified parser - full implementation would need
                # to match Carbon's exact binary structure
                
                print(f"    Boundary file size: {file.stat().st_size} bytes")
                # For now, we'll create placeholder zones
                # A full implementation would parse the actual binary structure
                
        except Exception as e:
            print(f"    Error parsing boundary file: {e}")
    
    def export_to_s3(self, output_dir: str, track: CarbonTrackInfo):
        """Export Carbon track data to SR3 XML format"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        # Create XML structure
        root = ET.Element("track")
        root.set("id", track.track_id)
        root.set("displayName", track.display_name)
        root.set("artName", track.art_name)
        root.set("region", track.region)
        
        # Engage position
        engage_elem = ET.SubElement(root, "engagePos")
        engage_elem.set("pos", f"{track.engage_pos[0]}, {track.engage_pos[1]}, {track.engage_pos[2]}")
        engage_elem.set("yaw", str(track.engage_yaw))
        
        # Minimap
        minimap_elem = ET.SubElement(root, "minimap")
        minimap_elem.set("locked", track.minimap_locked)
        minimap_elem.set("unlocked", track.minimap_unlocked)
        
        # Track map
        if track.track_map:
            # Commented out - would need actual TrackMaps.bin importer
            # root.set("trackMap", track.track_map)
            pass
        
        # Stats
        stats_elem = ET.SubElement(root, "stats")
        stats_elem.set("length", str(track.length))
        stats_elem.set("difficulty", str(track.difficulty))
        stats_elem.set("eventTypes", str(track.event_types))
        
        # Progression
        prog_elem = ET.SubElement(root, "progression")
        prog_elem.set("isUnlocked", str(track.is_unlocked).lower())
        prog_elem.set("districtId", str(track.district_id))
        
        # Zones
        if track.zones:
            zones_elem = ET.SubElement(root, "zones")
            for zone in track.zones:
                zone_elem = ET.SubElement(zones_elem, "zone")
                zone_elem.set("type", CARBON_ZONE_TYPES.get(zone.zone_type, "reset"))
                zone_elem.set("pos", f"{zone.position[0]}, {zone.position[1]}, {zone.position[2]}")
                zone_elem.set("radius", str(zone.radius))
                zone_elem.set("length", str(zone.length))
                zone_elem.set("group", str(zone.group_key))
                zone_elem.set("enabled", str(zone.enabled).lower())
                
                # Custom parameters
                for key, value in zone.params.items():
                    param_elem = ET.SubElement(zone_elem, "param")
                    param_elem.set("key", key)
                    param_elem.set("value", value)
        
        # Barriers
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
        
        # Write XML file
        tree = ET.ElementTree(root)
        ET.indent(tree, space="\t", level=0)
        
        output_file = output_path / f"{track.track_id}.xml"
        tree.write(output_file, encoding="utf-8", xml_declaration=True)
        print(f"Exported: {output_file}")
        
        return output_file
    
    def create_track_database(self, output_dir: str):
        """Create SR3 TrackDatabase.xml from parsed tracks"""
        output_path = Path(output_dir)
        
        root = ET.Element("trackDatabase")
        root.set("comment", "Generated by CarbonTrackParser.py")
        
        for track_id in sorted(self.tracks.keys()):
            track = self.tracks[track_id]
            track_elem = ET.SubElement(root, "track")
            track_elem.set("id", track.track_id)
            track_elem.set("file", f"{track.track_id}.xml")
        
        # Write database file
        tree = ET.ElementTree(root)
        ET.indent(tree, space="\t", level=0)
        
        output_file = output_path / "TrackDatabase.xml"
        tree.write(output_file, encoding="utf-8", xml_declaration=True)
        print(f"Created track database: {output_file}")


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        print("\nUsage: python CarbonTrackParser.py <carbon_tracks_dir> <output_dir> [region]")
        print("\nExamples:")
        print("  python CarbonTrackParser.py \"D:\\Games\\NFSC Redux\\TRACKS\" \"data\\tracks\" L5RA")
        print("  python CarbonTrackParser.py \"D:\\Games\\NFSC Redux\\TRACKS\" \"data\\tracks\"")
        return 1
    
    carbon_dir = sys.argv[1]
    output_dir = sys.argv[2]
    region = sys.argv[3] if len(sys.argv) > 3 else None
    
    print(f"Carbon Track Parser")
    print(f"==================")
    print(f"Carbon tracks: {carbon_dir}")
    print(f"Output dir: {output_dir}")
    
    parser = CarbonTrackParser(carbon_dir)
    
    if region:
        # Parse specific region
        print(f"\nParsing region: {region}")
        track = parser.parse_region(region)
        if track:
            parser.export_to_s3(output_dir, track)
    else:
        # Parse all regions
        print(f"\nParsing all regions...")
        for region_dir in Path(carbon_dir).iterdir():
            if region_dir.is_dir():
                region = region_dir.name
                print(f"\nParsing region: {region}")
                parser.parse_region(region)
    
    # Create track database
    if parser.tracks:
        parser.create_track_database(output_dir)
        print(f"\nParsed {len(parser.tracks)} track(s)")
    else:
        print("\nNo tracks found to parse")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
