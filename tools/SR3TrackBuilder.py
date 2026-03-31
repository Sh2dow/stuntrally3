#!/usr/bin/env python3
"""
SR3 Track Builder

Builds SR3 track files from an already-decoded SR3 mesh:
- Generates heightmap.f32 from mesh vertices
- Extracts road centerline for road.xml
- Creates scene.xml with track objects

Usage:
    python SR3TrackBuilder.py <carbon_track_dir> <sr3_output_dir> [track_name]

Example:
    python SR3TrackBuilder.py "output/L5RA_raw" "data/tracks/CasinoTower" "CasinoTower"
"""

import os
import sys
import struct
import math
import xml.etree.ElementTree as ET
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Tuple, Optional, Dict
import numpy as np


# ============================================================================
# Data Structures
# ============================================================================

@dataclass
class SR3Vertex:
    """SR3 vertex format (48 bytes)"""
    position: Tuple[float, float, float]
    normal: Tuple[float, float, float]
    uv: Tuple[float, float]
    tangent: Tuple[float, float, float, float]


@dataclass
class TrackMesh:
    """Loaded track mesh"""
    vertices: List[SR3Vertex] = field(default_factory=list)
    magic: str = ''
    header_size: int = 8


@dataclass
class BoundingBox:
    """3D bounding box"""
    min_x: float = 0.0
    min_y: float = 0.0
    min_z: float = 0.0
    max_x: float = 0.0
    max_y: float = 0.0
    max_z: float = 0.0
    
    @property
    def width(self) -> float:
        return self.max_x - self.min_x
    
    @property
    def height(self) -> float:
        return self.max_y - self.min_y
    
    @property
    def depth(self) -> float:
        return self.max_z - self.min_z


# ============================================================================
# Mesh Loader
# ============================================================================

class MeshLoader:
    """Load decoded SR3 mesh files"""
    
    @staticmethod
    def load(mesh_path: Path) -> Optional[TrackMesh]:
        """Load mesh from file"""
        if not mesh_path.exists():
            print(f"  Mesh not found: {mesh_path}")
            return None
        
        mesh = TrackMesh()
        
        try:
            with open(mesh_path, 'rb') as f:
                # Read header
                magic = f.read(4).decode('ascii', errors='ignore')
                vertex_count = struct.unpack('<I', f.read(4))[0]
                
                mesh.magic = magic
                mesh.header_size = 8
                
                if magic != 'SR3M':
                    print(f"  Unsupported mesh format '{magic}'")
                    print("  tools/CarbonTrackConverter.py does not decode Carbon GeometryPack data into SR3 meshes.")
                    return None

                expected_size = 8 + vertex_count * 48
                actual_size = mesh_path.stat().st_size
                if actual_size < expected_size:
                    print(f"  Mesh is truncated: expected at least {expected_size} bytes, got {actual_size}")
                    return None
                
                print(f"  Loading {vertex_count:,} vertices...")
                
                # Read vertices (48 bytes each)
                for _ in range(vertex_count):
                    data = f.read(48)
                    if len(data) < 48:
                        break
                    
                    vertex = SR3Vertex(
                        position=struct.unpack_from('<fff', data, 0),
                        normal=struct.unpack_from('<fff', data, 12),
                        uv=struct.unpack_from('<ff', data, 24),
                        tangent=struct.unpack_from('<ffff', data, 32)
                    )
                    mesh.vertices.append(vertex)
                
                print(f"  Loaded {len(mesh.vertices):,} vertices")

                if len(mesh.vertices) != vertex_count:
                    print(f"  Mesh vertex count mismatch: header={vertex_count}, read={len(mesh.vertices)}")
                    return None
                
        except Exception as e:
            print(f"  ERROR loading mesh: {e}")
            return None
        
        return mesh


# ============================================================================
# Heightmap Generator
# ============================================================================

class HeightmapGenerator:
    """Generate SR3 heightmap from mesh"""
    
    def __init__(self, mesh: TrackMesh):
        self.mesh = mesh
        self.bbox = self._compute_bbox()
    
    def _compute_bbox(self) -> BoundingBox:
        """Compute bounding box from vertices"""
        if not self.mesh.vertices:
            return BoundingBox()
        
        # Filter out invalid vertices
        valid_positions = []
        for v in self.mesh.vertices:
            x, y, z = v.position
            if math.isfinite(x) and math.isfinite(y) and math.isfinite(z):
                valid_positions.append((x, y, z))
        
        if not valid_positions:
            print("  WARNING: No valid vertex positions found")
            return BoundingBox()
        
        positions = np.array(valid_positions)
        
        bbox = BoundingBox(
            min_x=float(np.min(positions[:, 0])),
            min_y=float(np.min(positions[:, 1])),
            min_z=float(np.min(positions[:, 2])),
            max_x=float(np.max(positions[:, 0])),
            max_y=float(np.max(positions[:, 1])),
            max_z=float(np.max(positions[:, 2]))
        )
        
        print(f"  Track bounds: {bbox.width:.1f} x {bbox.height:.1f} x {bbox.depth:.1f}")
        return bbox
    
    def generate(self, output_path: Path, resolution: int = 512, 
                 height_scale: float = 1.0) -> bool:
        """
        Generate heightmap.f32 file
        
        Args:
            output_path: Output file path
            resolution: Heightmap resolution (512x512 default)
            height_scale: Vertical scale factor
        """
        print(f"  Generating {resolution}x{resolution} heightmap...")
        
        # Create heightmap array
        heightmap = np.zeros((resolution, resolution), dtype=np.float32)
        
        # Project vertices to XZ plane, use Y as height
        x_scale = resolution / max(self.bbox.width, 0.001)
        z_scale = resolution / max(self.bbox.depth, 0.001)
        
        for vertex in self.mesh.vertices:
            x, y, z = vertex.position
            
            # Skip invalid vertices
            if not (math.isfinite(x) and math.isfinite(y) and math.isfinite(z)):
                continue
            
            # Map to heightmap coordinates
            hx = int((x - self.bbox.min_x) * x_scale)
            hz = int((z - self.bbox.min_z) * z_scale)
            
            # Clamp to bounds
            hx = max(0, min(resolution - 1, hx))
            hz = max(0, min(resolution - 1, hz))
            
            # Store height (use maximum for overlapping vertices)
            heightmap[hz, hx] = max(heightmap[hz, hx], y * height_scale)
        
        # Apply smoothing
        heightmap = self._smooth_heightmap(heightmap, iterations=2)
        
        # Write to file
        try:
            with open(output_path, 'wb') as f:
                f.write(heightmap.tobytes())
            
            file_size = output_path.stat().st_size
            print(f"  Wrote {output_path.name} ({file_size:,} bytes)")
            return True
            
        except Exception as e:
            print(f"  ERROR writing heightmap: {e}")
            return False
    
    def _smooth_heightmap(self, heightmap: np.ndarray, iterations: int = 1) -> np.ndarray:
        """Apply Gaussian-like smoothing"""
        from scipy.ndimage import gaussian_filter
        return gaussian_filter(heightmap, sigma=1.0)


# ============================================================================
# Road XML Generator
# ============================================================================

class RoadXMLGenerator:
    """Generate SR3 road.xml from track mesh"""
    
    def __init__(self, mesh: TrackMesh, bbox: BoundingBox):
        self.mesh = mesh
        self.bbox = bbox
    
    def generate(self, output_path: Path, track_width: float = 10.0) -> bool:
        """
        Generate road.xml with track centerline
        
        Args:
            output_path: Output file path
            track_width: Approximate track width in meters
        """
        print(f"  Generating road.xml (track width: {track_width}m)...")
        
        # Extract approximate centerline from mesh
        # This is a simplified approach - full implementation would need
        # proper road edge detection
        
        centerline = self._extract_centerline(track_width)
        
        if not centerline:
            print("  WARNING: Could not extract centerline")
            return False
        
        # Build XML
        root = ET.Element('road')
        root.set('version', '3')
        
        # Add track points
        points_elem = ET.SubElement(root, 'points')
        
        for i, point in enumerate(centerline):
            pt_elem = ET.SubElement(points_elem, 'point')
            pt_elem.set('id', str(i))
            pt_elem.set('x', f'{point[0]:.3f}')
            pt_elem.set('y', f'{point[1]:.3f}')
            pt_elem.set('z', f'{point[2]:.3f}')
            pt_elem.set('width', f'{track_width:.2f}')
            
            # Add spline flags
            if i == 0:
                pt_elem.set('start', '1')
            if i == len(centerline) - 1:
                pt_elem.set('end', '1')
        
        # Write XML
        tree = ET.ElementTree(root)
        ET.indent(tree, space='  ')
        
        try:
            tree.write(output_path, encoding='utf-8', xml_declaration=True)
            print(f"  Wrote {output_path.name} ({len(centerline)} points)")
            return True
        except Exception as e:
            print(f"  ERROR writing road.xml: {e}")
            return False
    
    def _extract_centerline(self, track_width: float) -> List[Tuple[float, float, float]]:
        """
        Extract approximate centerline from track mesh
        
        This is a simplified implementation. A full implementation would:
        1. Identify road surface by texture/material
        2. Find road edges
        3. Compute centerline between edges
        4. Smooth and optimize the spline
        """
        # For now, use a simple approach:
        # 1. Sort vertices by Z (track direction)
        # 2. Average X positions in slices
        
        if not self.mesh.vertices:
            return []
        
        # Group vertices by Z slices
        z_slices = {}
        slice_size = track_width / 2
        
        for vertex in self.mesh.vertices:
            x, y, z = vertex.position
            slice_key = int(z / slice_size)
            
            if slice_key not in z_slices:
                z_slices[slice_key] = []
            z_slices[slice_key].append((x, y, z))
        
        # Compute centerline points
        centerline = []
        for slice_key in sorted(z_slices.keys()):
            points = z_slices[slice_key]
            if points:
                avg_x = sum(p[0] for p in points) / len(points)
                avg_y = sum(p[1] for p in points) / len(points)
                avg_z = sum(p[2] for p in points) / len(points)
                centerline.append((avg_x, avg_y, avg_z))
        
        # Smooth centerline
        if len(centerline) > 3:
            centerline = self._smooth_spline(centerline)
        
        return centerline
    
    def _smooth_spline(self, points: List[Tuple[float, float, float]], 
                       iterations: int = 2) -> List[Tuple[float, float, float]]:
        """Apply moving average smoothing"""
        if len(points) < 3:
            return points
        
        smoothed = points[:]
        for _ in range(iterations):
            new_points = [smoothed[0]]
            for i in range(1, len(smoothed) - 1):
                prev = smoothed[i - 1]
                curr = smoothed[i]
                next_p = smoothed[i + 1]
                new_points.append((
                    (prev[0] + curr[0] * 2 + next_p[0]) / 4,
                    (prev[1] + curr[1] * 2 + next_p[1]) / 4,
                    (prev[2] + curr[2] * 2 + next_p[2]) / 4
                ))
            new_points.append(smoothed[-1])
            smoothed = new_points
        
        return smoothed


# ============================================================================
# Scene XML Generator
# ============================================================================

class SceneXMLGenerator:
    """Generate SR3 scene.xml"""
    
    def __init__(self, track_name: str, bbox: BoundingBox):
        self.track_name = track_name
        self.bbox = bbox
    
    def generate(self, output_path: Path) -> bool:
        """Generate scene.xml with track objects"""
        print(f"  Generating scene.xml...")
        
        root = ET.Element('scene')
        root.set('version', '3')
        
        # Track info
        info_elem = ET.SubElement(root, 'info')
        ET.SubElement(info_elem, 'name').text = self.track_name
        ET.SubElement(info_elem, 'author').text = 'NFS Carbon'
        ET.SubElement(info_elem, 'description').text = f'{self.track_name} ported from NFS Carbon'
        
        # Track bounds
        bounds_elem = ET.SubElement(root, 'bounds')
        bounds_elem.set('min', f'{self.bbox.min_x:.1f} {self.bbox.min_y:.1f} {self.bbox.min_z:.1f}')
        bounds_elem.set('max', f'{self.bbox.max_x:.1f} {self.bbox.max_y:.1f} {self.bbox.max_z:.1f}')
        
        # Objects section (empty for now - would contain track props)
        objects_elem = ET.SubElement(root, 'objects')
        # TODO: Add track objects from Carbon data
        
        # Lighting
        lighting_elem = ET.SubElement(root, 'lighting')
        ET.SubElement(lighting_elem, 'ambient').text = '0.4 0.4 0.45'
        ET.SubElement(lighting_elem, 'sun').text = '0.8 0.75 0.6'
        ET.SubElement(lighting_elem, 'sun_dir').text = '-0.5 0.8 -0.3'
        
        # Write XML
        tree = ET.ElementTree(root)
        ET.indent(tree, space='  ')
        
        try:
            tree.write(output_path, encoding='utf-8', xml_declaration=True)
            print(f"  Wrote {output_path.name}")
            return True
        except Exception as e:
            print(f"  ERROR writing scene.xml: {e}")
            return False


# ============================================================================
# Main Track Builder
# ============================================================================

class SR3TrackBuilder:
    """Main track builder class"""
    
    def __init__(self, carbon_dir: str, sr3_output: str, track_name: str):
        self.carbon_dir = Path(carbon_dir)
        self.sr3_output = Path(sr3_output)
        self.track_name = track_name
        
        self.mesh: Optional[TrackMesh] = None
        self.bbox: Optional[BoundingBox] = None
    
    def build(self) -> bool:
        """Run full track building pipeline"""
        print(f"Building SR3 track: {self.track_name}")
        print(f"  Input: {self.carbon_dir}")
        print(f"  Output: {self.sr3_output}")
        print()
        
        # Create output directory
        self.sr3_output.mkdir(parents=True, exist_ok=True)
        
        # Step 1: Load mesh
        print("=== Step 1: Load Track Mesh ===")
        mesh_path = self.carbon_dir / 'track.mesh'
        self.mesh = MeshLoader.load(mesh_path)
        
        if not self.mesh or not self.mesh.vertices:
            print("ERROR: No mesh loaded")
            return False

        if not self._validate_mesh(self.mesh):
            print("ERROR: Mesh content is not suitable for heightmap/spline generation")
            return False
        print()
        
        # Step 2: Generate heightmap
        print("=== Step 2: Generate Heightmap ===")
        heightmap_gen = HeightmapGenerator(self.mesh)
        self.bbox = heightmap_gen.bbox
        
        heightmap_path = self.sr3_output / 'heightmap.f32'
        heightmap_gen.generate(heightmap_path, resolution=512)
        
        # Also generate heightmap2.f32 (higher resolution)
        heightmap2_path = self.sr3_output / 'heightmap2.f32'
        heightmap_gen.generate(heightmap2_path, resolution=1024)
        print()
        
        # Step 3: Generate road.xml
        print("=== Step 3: Generate Road XML ===")
        road_gen = RoadXMLGenerator(self.mesh, self.bbox)
        road_path = self.sr3_output / 'road.xml'
        road_gen.generate(road_path, track_width=12.0)
        print()
        
        # Step 4: Generate scene.xml
        print("=== Step 4: Generate Scene XML ===")
        scene_gen = SceneXMLGenerator(self.track_name, self.bbox)
        scene_path = self.sr3_output / 'scene.xml'
        scene_gen.generate(scene_path)
        print()
        
        # Step 5: Create directories
        print("=== Step 5: Create Directory Structure ===")
        (self.sr3_output / 'objects').mkdir(exist_ok=True)
        (self.sr3_output / 'preview').mkdir(exist_ok=True)
        print("  Created: objects/")
        print("  Created: preview/")
        print()
        
        print("=== Build Complete ===")
        print(f"Output: {self.sr3_output}")
        print()
        print("Next steps:")
        print("  1. Copy textures to: data/tracks/<track>/textures/")
        print("  2. Create materials.ini")
        print("  3. Add track to career_tracks.xml")
        print("  4. Generate preview image")
        
        return True

    def _validate_mesh(self, mesh: TrackMesh) -> bool:
        finite_vertices = []
        for vertex in mesh.vertices:
            x, y, z = vertex.position
            if math.isfinite(x) and math.isfinite(y) and math.isfinite(z):
                finite_vertices.append((x, y, z))

        if len(finite_vertices) < 3:
            print("  Mesh does not contain enough finite positions")
            return False

        xs = [value[0] for value in finite_vertices]
        ys = [value[1] for value in finite_vertices]
        zs = [value[2] for value in finite_vertices]

        width = max(xs) - min(xs)
        height = max(ys) - min(ys)
        depth = max(zs) - min(zs)

        if width < 1.0 or depth < 1.0:
            print(f"  Mesh bounds are implausibly small: width={width:.6f}, depth={depth:.6f}, height={height:.6f}")
            return False

        return True


# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        print("\nUsage: python SR3TrackBuilder.py <carbon_track_dir> <sr3_output_dir> [track_name]")
        print("\nExamples:")
        print('  python SR3TrackBuilder.py "output/L5RA_raw" "data/tracks/CasinoTower" "CasinoTower"')
        print('  python SR3TrackBuilder.py "output/L3RA_raw" "data/tracks/Fortuna" "Fortuna"')
        sys.exit(1)
    
    carbon_dir = sys.argv[1]
    sr3_output = sys.argv[2]
    track_name = sys.argv[3] if len(sys.argv) > 3 else 'CarbonTrack'
    
    builder = SR3TrackBuilder(carbon_dir, sr3_output, track_name)
    success = builder.build()
    
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
