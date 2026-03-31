#!/usr/bin/env python3
"""
Carbon Track Section Extractor

Uses Binarius extraction + hyperlinked chunk definitions to extract
Carbon track sections and convert to SR3-readable format.

Prerequisites:
- Binarius extracted files in output/
- hyperlinked chunk definitions
- IDA for structure verification

Usage:
    python CarbonTrackExtractor.py "D:\Games\NFSC Redux\TRACKS\L5RA" "data/tracks/CasinoTower/"
"""

import os
import sys
import json
import struct
from pathlib import Path

# Carbon chunk IDs from hyperlinked
CHUNK_IDS = {
    0x5C: 'track_streaming_sections',
    0x5D: 'track_streaming_infos',
    0x5E: 'track_streaming_barriers',
    0x5F: 'track_streaming_discs',
    0x80034147: 'track_path_manager',
    0x00034148: 'track_path_points',
    0x00034149: 'track_path_lanes',
    0x0003414A: 'track_path_zones',
    0x0003414D: 'track_path_barriers',
    0x0E: 'texture_dict',
    0x13: 'model_dict',
    0x1A: 'collision_pack',
}

# Zone types from track_path.hpp
ZONE_TYPES = {
    0: 'reset',
    1: 'guided_reset',
    2: 'tunnel',
    3: 'overpass',
    4: 'streamer_prediction',
    5: 'garage',
    6: 'traffic_pattern',
    7: 'dynamic',
    8: 'neighborhood',
    9: 'jump_camera',
    10: 'no_cop_spawn',
    11: 'pursuit_start',
    12: 'highway',
    13: 'canyon_drop',
    14: 'vertigo_camera',
}

# Section record structure (from IDA - 92 bytes)
SECTION_RECORD_SIZE = 92
SECTION_RECORD_FORMAT = '<IIIIIIIIIIIIIIIIIIII'  # Adjust based on actual structure

class CarbonTrackExtractor:
    def __init__(self, carbon_track_path, output_path):
        self.carbon_path = Path(carbon_track_path)
        self.output_path = Path(output_path)
        self.sections = []
        self.zones = []
        self.barriers = []
        
    def extract(self):
        """Main extraction pipeline"""
        print(f"Extracting Carbon track: {self.carbon_path}")
        print(f"Output to: {self.output_path}")
        
        # Step 1: Inventory BUN contents
        self.inventory_bundles()
        
        # Step 2: Extract streaming sections
        self.extract_sections()
        
        # Step 3: Extract track path (zones, barriers)
        self.extract_track_path()
        
        # Step 4: Generate SR3 format
        self.generate_sr3_format()
        
        print("Extraction complete!")
        
    def inventory_bundles(self):
        """Step 1: List all chunks in BUN files"""
        print("\n=== Step 1: Inventory BUN Contents ===")
        
        bun_files = list(self.carbon_path.glob('*.BUN'))
        
        inventory = {}
        for bun_file in bun_files:
            print(f"\nProcessing: {bun_file.name}")
            inventory[bun_file.name] = self.parse_bun_header(bun_file)
        
        # Save inventory
        inventory_file = self.output_path / 'inventory.json'
        with open(inventory_file, 'w') as f:
            json.dump(inventory, f, indent=2)
        
        print(f"Saved inventory to: {inventory_file}")
        
    def parse_bun_header(self, bun_file):
        """Parse BUN file header and list chunks"""
        chunks = []
        
        try:
            with open(bun_file, 'rb') as f:
                # Read BUN header (implementation depends on Binarius format)
                # This is a placeholder - actual implementation needs Binarius structure
                
                # Example structure (adjust based on actual BUN format):
                # uint32_t magic
                # uint32_t version
                # uint32_t num_chunks
                # ChunkEntry[num_chunks]
                
                magic = struct.unpack('<I', f.read(4))[0]
                version = struct.unpack('<I', f.read(4))[0]
                num_chunks = struct.unpack('<I', f.read(4))[0]
                
                print(f"  Magic: 0x{magic:08X}")
                print(f"  Version: {version}")
                print(f"  Chunks: {num_chunks}")
                
                for i in range(num_chunks):
                    chunk_id = struct.unpack('<I', f.read(4))[0]
                    chunk_offset = struct.unpack('<I', f.read(4))[0]
                    chunk_size = struct.unpack('<I', f.read(4))[4])
                    
                    chunk_name = CHUNK_IDS.get(chunk_id, f'unknown_0x{chunk_id:08X}')
                    chunks.append({
                        'id': chunk_id,
                        'name': chunk_name,
                        'offset': chunk_offset,
                        'size': chunk_size,
                    })
                    
                    print(f"    Chunk {i}: {chunk_name} (0x{chunk_id:08X}) @ {chunk_offset} ({chunk_size} bytes)")
                    
        except Exception as e:
            print(f"  Error parsing BUN: {e}")
            
        return chunks
        
    def extract_sections(self):
        """Step 2: Extract track streaming sections"""
        print("\n=== Step 2: Extract Streaming Sections ===")
        
        # Find section chunk (0x5C)
        section_file = self.find_chunk_file(0x5C)
        
        if not section_file:
            print("  No section chunk found, skipping")
            return
            
        print(f"  Found sections in: {section_file}")
        
        try:
            with open(section_file, 'rb') as f:
                # Parse section records (92 bytes each per IDA)
                while True:
                    data = f.read(SECTION_RECORD_SIZE)
                    if len(data) < SECTION_RECORD_SIZE:
                        break
                        
                    section = self.parse_section_record(data)
                    if section:
                        self.sections.append(section)
                        print(f"  Section {section['id']}: bounds={section['bounds']}")
                        
        except Exception as e:
            print(f"  Error extracting sections: {e}")
            
    def parse_section_record(self, data):
        """Parse a single section record"""
        # Structure from IDA (adjust as needed):
        # uint32_t section_id
        # float bounds_min[3]
        # float bounds_max[3]
        # uint32_t num_models
        # uint32_t model_offset
        # ... (rest of 92 bytes)
        
        try:
            section_id = struct.unpack('<I', data[0:4])[0]
            bounds_min = struct.unpack('<fff', data[4:16])
            bounds_max = struct.unpack('<fff', data[16:28])
            
            return {
                'id': section_id,
                'bounds': {
                    'min': bounds_min,
                    'max': bounds_max,
                },
                'raw_data': data,
            }
        except Exception as e:
            print(f"  Error parsing section: {e}")
            return None
            
    def extract_track_path(self):
        """Step 3: Extract track path data (zones, barriers)"""
        print("\n=== Step 3: Extract Track Path ===")
        
        # Extract zones (0x0003414A)
        zone_file = self.find_chunk_file(0x0003414A)
        if zone_file:
            print(f"  Found zones in: {zone_file}")
            self.zones = self.parse_zones(zone_file)
        else:
            print("  No zone chunk found")
            
        # Extract barriers (0x0003414D)
        barrier_file = self.find_chunk_file(0x0003414D)
        if barrier_file:
            print(f"  Found barriers in: {barrier_file}")
            self.barriers = self.parse_barriers(barrier_file)
        else:
            print("  No barrier chunk found")
            
    def find_chunk_file(self, chunk_id):
        """Find extracted chunk file by ID"""
        # Binarius extracts chunks with naming like: 0x5C_section_records.bin
        chunk_name = CHUNK_IDS.get(chunk_id, f'0x{chunk_id:08X}')
        
        # Search in output directory
        for file in self.carbon_path.glob(f'*{chunk_name}*'):
            return file
            
        # Also check parent directory
        for file in self.carbon_path.parent.glob(f'*{chunk_name}*'):
            return file
            
        return None
        
    def parse_zones(self, zone_file):
        """Parse track path zones"""
        zones = []
        
        try:
            with open(zone_file, 'rb') as f:
                # Zone structure (from track_path.hpp):
                # uint32_t zone_type
                # float position[3]
                # float radius
                # uint32_t flags
                # ...
                
                while True:
                    data = f.read(32)  # Adjust size as needed
                    if len(data) < 32:
                        break
                        
                    zone_type = struct.unpack('<I', data[0:4])[0]
                    position = struct.unpack('<fff', data[4:16])
                    radius = struct.unpack('<f', data[16:20])[0]
                    
                    zone_name = ZONE_TYPES.get(zone_type, f'unknown_{zone_type}')
                    
                    zones.append({
                        'type': zone_type,
                        'type_name': zone_name,
                        'position': position,
                        'radius': radius,
                    })
                    
                    print(f"    Zone: {zone_name} @ {position} r={radius}")
                    
        except Exception as e:
            print(f"  Error parsing zones: {e}")
            
        return zones
        
    def parse_barriers(self, barrier_file):
        """Parse track path barriers"""
        barriers = []
        
        try:
            with open(barrier_file, 'rb') as f:
                # Barrier structure:
                # float start[3]
                # float end[3]
                # float height
                # uint32_t flags
                # ...
                
                while True:
                    data = f.read(32)  # Adjust size as needed
                    if len(data) < 32:
                        break
                        
                    start = struct.unpack('<fff', data[0:12])
                    end = struct.unpack('<fff', data[12:24])
                    height = struct.unpack('<f', data[24:28])[0]
                    
                    barriers.append({
                        'start': start,
                        'end': end,
                        'height': height,
                    })
                    
                    print(f"    Barrier: {start} -> {end} h={height}")
                    
        except Exception as e:
            print(f"  Error parsing barriers: {e}")
            
        return barriers
        
    def generate_sr3_format(self):
        """Step 4: Generate SR3-readable format"""
        print("\n=== Step 4: Generate SR3 Format ===")
        
        self.output_path.mkdir(parents=True, exist_ok=True)
        
        # Generate track.ini
        self.generate_track_ini()
        
        # Generate zones.xml
        self.generate_zones_xml()
        
        # Generate barriers.xml
        self.generate_barriers_xml()
        
        # Generate section manifest
        self.generate_section_manifest()
        
    def generate_track_ini(self):
        """Generate SR3 track.ini"""
        ini_path = self.output_path / 'track.ini'
        
        with open(ini_path, 'w') as f:
            f.write("[track]\n")
            f.write(f"name = {self.carbon_path.name}\n")
            f.write(f"length = {len(self.sections) * 100}\n")  # Placeholder
            f.write(f"difficulty = 3\n")
            f.write("\n")
            f.write("[events]\n")
            f.write(f"sprint = 1\n")
            f.write(f"circuit = 1\n")
            
        print(f"  Generated: {ini_path}")
        
    def generate_zones_xml(self):
        """Generate SR3 zones.xml from Carbon zones"""
        if not self.zones:
            print("  No zones to export")
            return
            
        xml_path = self.output_path / 'zones.xml'
        
        with open(xml_path, 'w') as f:
            f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
            f.write('<zones>\n')
            
            for zone in self.zones:
                f.write(f'  <zone type="{zone["type_name"]}" ')
                f.write(f'pos="{zone["position"][0]},{zone["position"][1]},{zone["position"][2]}" ')
                f.write(f'radius="{zone["radius"]}"/>\n')
                
            f.write('</zones>\n')
            
        print(f"  Generated: {xml_path}")
        
    def generate_barriers_xml(self):
        """Generate SR3 barriers.xml from Carbon barriers"""
        if not self.barriers:
            print("  No barriers to export")
            return
            
        xml_path = self.output_path / 'barriers.xml'
        
        with open(xml_path, 'w') as f:
            f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
            f.write('<barriers>\n')
            
            for barrier in self.barriers:
                f.write(f'  <barrier ')
                f.write(f'start="{barrier["start"][0]},{barrier["start"][1]},{barrier["start"][2]}" ')
                f.write(f'end="{barrier["end"][0]},{barrier["end"][1]},{barrier["end"][2]}" ')
                f.write(f'height="{barrier["height"]}"/>\n')
                
            f.write('</barriers>\n')
            
        print(f"  Generated: {xml_path}")
        
    def generate_section_manifest(self):
        """Generate section manifest for SR3 loader"""
        manifest_path = self.output_path / 'sections.json'
        
        with open(manifest_path, 'w') as f:
            json.dump({
                'num_sections': len(self.sections),
                'sections': self.sections,
                'num_zones': len(self.zones),
                'num_barriers': len(self.barriers),
            }, f, indent=2)
            
        print(f"  Generated: {manifest_path}")


def main():
    if len(sys.argv) < 3:
        print("Usage: python CarbonTrackExtractor.py <carbon_track_path> <output_path>")
        print("Example: python CarbonTrackExtractor.py \"D:\\Games\\NFSC Redux\\TRACKS\\L5RA\" \"data\\tracks\\CasinoTower\\\"")
        sys.exit(1)
        
    carbon_path = sys.argv[1]
    output_path = sys.argv[2]
    
    extractor = CarbonTrackExtractor(carbon_path, output_path)
    extractor.extract()


if __name__ == '__main__':
    main()
