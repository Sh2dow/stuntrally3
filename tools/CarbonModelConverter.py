#!/usr/bin/env python3
"""
Carbon Model & Texture Converter

Converts extracted Carbon models/textures to SR3 format.

Prerequisites:
- Extracted Carbon files from Binarius
- PIL/Pillow for texture conversion
- numpy for vertex processing

Usage:
    python CarbonModelConverter.py "data/tracks/CasinoTower/raw/" "data/tracks/CasinoTower/"
"""

import os
import sys
import struct
import json
import numpy as np
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("ERROR: PIL/Pillow not installed. Run: pip install Pillow")
    sys.exit(1)


class CarbonModelConverter:
    def __init__(self, input_path, output_path):
        self.input_path = Path(input_path)
        self.output_path = Path(output_path)
        self.models = []
        self.textures = []
        
    def convert(self):
        """Main conversion pipeline"""
        print(f"Converting Carbon models: {self.input_path}")
        print(f"Output to: {self.output_path}")
        
        # Create output directories
        (self.output_path / 'textures').mkdir(parents=True, exist_ok=True)
        
        # Step 1: Load manifest from Binarius extraction
        if not self.load_manifest():
            return False
            
        # Step 2: Convert textures
        self.convert_textures()
        
        # Step 3: Convert models
        self.convert_models()
        
        # Step 4: Generate SR3 mesh file
        self.generate_sr3_mesh()
        
        # Step 5: Generate material file
        self.generate_materials()
        
        print("Conversion complete!")
        return True
        
    def load_manifest(self):
        """Load extraction manifest from Binarius"""
        manifest_file = self.input_path / 'manifest.json'
        
        if not manifest_file.exists():
            print(f"ERROR: No manifest.json found in {self.input_path}")
            print("Run Binarius extraction first!")
            return False
            
        with open(manifest_file, 'r') as f:
            self.manifest = json.load(f)
            
        print(f"Loaded manifest: {len(self.manifest.get('models', []))} models, "
              f"{len(self.manifest.get('textures', []))} textures")
              
        self.models = self.manifest.get('models', [])
        self.textures = self.manifest.get('textures', [])
        
        return True
        
    def convert_textures(self):
        """Step 2: Convert DXT textures to PNG"""
        print("\n=== Step 2: Convert Textures ===")
        
        texture_file = self.input_path / '0x0E_texture_dict.bin'
        if not texture_file.exists():
            print("  No texture dictionary found, skipping")
            return
            
        with open(texture_file, 'rb') as f:
            # Read texture count
            num_textures = struct.unpack('<I', f.read(4))[0]
            print(f"  Found {num_textures} textures")
            
            for i in range(num_textures):
                # Read texture record (24 bytes)
                tex_data = f.read(24)
                if len(tex_data) < 24:
                    break
                    
                tex_id, width, height, format, data_offset, data_size = \
                    struct.unpack('<IIIIII', tex_data)
                    
                # Read texture data
                f.seek(data_offset)
                tex_data = f.read(data_size)
                
                # Decompress based on format
                # Format: 0=DXT1, 1=DXT3, 2=DXT5
                try:
                    if format == 0:
                        img = self.decompress_dxt1(tex_data, width, height)
                    elif format == 1:
                        img = self.decompress_dxt3(tex_data, width, height)
                    elif format == 2:
                        img = self.decompress_dxt5(tex_data, width, height)
                    else:
                        print(f"  Unknown texture format {format}, skipping")
                        continue
                        
                    # Save as PNG
                    output_file = self.output_path / 'textures' / f'tex_{tex_id:08d}.png'
                    img.save(output_file)
                    print(f"  Converted texture {i}/{num_textures}: {tex_id:08X} ({width}x{height})")
                    
                except Exception as e:
                    print(f"  Error converting texture {tex_id}: {e}")
                    
    def decompress_dxt1(self, data, width, height):
        """Decompress DXT1 texture"""
        # DXT1: 8 bytes per 4x4 block
        img = Image.new('RGBA', (width, height))
        pixels = img.load()
        
        block_index = 0
        for by in range(0, height, 4):
            for bx in range(0, width, 4):
                # Read block
                block = data[block_index*8:(block_index+1)*8]
                color0, color1, indices = struct.unpack('<HHI', block[:8])
                
                # Decode colors
                r0, g0, b0 = self.decode_565(color0)
                r1, g1, b1 = self.decode_565(color1)
                
                # Generate palette
                if color0 > color1:
                    colors = [
                        (r0, g0, b0, 255),
                        (r1, g1, b1, 255),
                        ((2*r0 + r1)//3, (2*g0 + g1)//3, (2*b0 + b1)//3, 255),
                        ((r0 + 2*r1)//3, (g0 + 2*g1)//3, (b0 + 2*b1)//3, 255),
                    ]
                else:
                    colors = [
                        (r0, g0, b0, 255),
                        (r1, g1, b1, 255),
                        ((r0 + r1)//2, (g0 + g1)//2, (b0 + b1)//2, 255),
                        (0, 0, 0, 0),
                    ]
                
                # Decode indices
                for py in range(4):
                    for px in range(4):
                        index = (indices >> (2 * (py * 4 + px))) & 0x03
                        if bx + px < width and by + py < height:
                            pixels[bx + px, by + py] = colors[index]
                            
                block_index += 1
                
        return img
        
    def decode_565(self, color):
        """Decode 16-bit 565 color to RGB"""
        r = (color >> 11) & 0x1F
        g = (color >> 5) & 0x3F
        b = color & 0x1F
        return (r * 8 // 31, g * 4 // 63, b * 8 // 31)
        
    def decompress_dxt3(self, data, width, height):
        """Decompress DXT3 texture"""
        # DXT3: 16 bytes per 4x4 block (8 bytes alpha + 8 bytes color)
        img = Image.new('RGBA', (width, height))
        pixels = img.load()
        
        block_index = 0
        for by in range(0, height, 4):
            for bx in range(0, width, 4):
                # Read alpha
                alpha_data = struct.unpack('<Q', data[block_index*16:block_index*16+8])[0]
                
                # Read color
                block = data[block_index*16+8:block_index*16+16]
                color0, color1, indices = struct.unpack('<HHI', block[:8])
                
                # Decode colors (DXT3 always uses 4-color interpolation)
                r0, g0, b0 = self.decode_565(color0)
                r1, g1, b1 = self.decode_565(color1)
                
                colors = [
                    (r0, g0, b0),
                    (r1, g1, b1),
                    ((2*r0 + r1)//3, (2*g0 + g1)//3, (2*b0 + b1)//3),
                    ((r0 + 2*r1)//3, (g0 + 2*g1)//3, (b0 + 2*b1)//3),
                ]
                
                # Decode pixels
                for py in range(4):
                    for px in range(4):
                        alpha_idx = py * 4 + px
                        alpha = (alpha_data >> (4 * alpha_idx)) & 0x0F
                        alpha = alpha * 17  # 4-bit to 8-bit
                        
                        color_idx = (indices >> (2 * alpha_idx)) & 0x03
                        r, g, b = colors[color_idx]
                        
                        if bx + px < width and by + py < height:
                            pixels[bx + px, by + py] = (r, g, b, alpha)
                            
                block_index += 1
                
        return img
        
    def decompress_dxt5(self, data, width, height):
        """Decompress DXT5 texture"""
        # DXT5: 16 bytes per 4x4 block (8 bytes alpha + 8 bytes color)
        img = Image.new('RGBA', (width, height))
        pixels = img.load()
        
        block_index = 0
        for by in range(0, height, 4):
            for bx in range(0, width, 4):
                # Read alpha
                alpha_data = data[block_index*16:block_index*16+8]
                alpha0, alpha1 = struct.unpack('<BB', alpha_data[:2])
                
                # Generate alpha palette
                if alpha0 > alpha1:
                    alphas = [
                        alpha0, alpha1,
                        (6*alpha0 + 1*alpha1)//7,
                        (5*alpha0 + 2*alpha1)//7,
                        (4*alpha0 + 3*alpha1)//7,
                        (3*alpha0 + 4*alpha1)//7,
                        (2*alpha0 + 5*alpha1)//7,
                        (1*alpha0 + 6*alpha1)//7,
                    ]
                else:
                    alphas = [
                        alpha0, alpha1,
                        (4*alpha0 + 1*alpha1)//5,
                        (3*alpha0 + 2*alpha1)//5,
                        (2*alpha0 + 3*alpha1)//5,
                        (1*alpha0 + 4*alpha1)//5,
                        0, 255
                    ]
                
                # Read color
                block = data[block_index*16+8:block_index*16+16]
                color0, color1, indices = struct.unpack('<HHI', block[:8])
                
                # Decode colors
                r0, g0, b0 = self.decode_565(color0)
                r1, g1, b1 = self.decode_565(color1)
                
                colors = [
                    (r0, g0, b0),
                    (r1, g1, b1),
                    ((2*r0 + r1)//3, (2*g0 + g1)//3, (2*b0 + b1)//3),
                    ((r0 + 2*r1)//3, (g0 + 2*g1)//3, (b0 + 2*b1)//3),
                ]
                
                # Decode pixels
                alpha_indices = struct.unpack('<Q', alpha_data[2:10])[0]
                color_indices = struct.unpack('<I', alpha_data[10:14])[0]
                
                for py in range(4):
                    for px in range(4):
                        idx = py * 4 + px
                        
                        alpha_idx = (alpha_indices >> (3 * idx)) & 0x07
                        alpha = alphas[alpha_idx]
                        
                        color_idx = (color_indices >> (2 * idx)) & 0x03
                        r, g, b = colors[color_idx]
                        
                        if bx + px < width and by + py < height:
                            pixels[bx + px, by + py] = (r, g, b, alpha)
                            
                block_index += 1
                
        return img
        
    def convert_models(self):
        """Step 3: Convert Carbon models to SR3 format"""
        print("\n=== Step 3: Convert Models ===")
        
        model_file = self.input_path / '0x13_model_dict.bin'
        if not model_file.exists():
            print("  No model dictionary found, skipping")
            return
            
        with open(model_file, 'rb') as f:
            # Read model count
            num_models = struct.unpack('<I', f.read(4))[0]
            print(f"  Found {num_models} models")
            
            for i in range(num_models):
                # Read model record (32 bytes)
                model_data = f.read(32)
                if len(model_data) < 32:
                    break
                    
                model_id, vert_offset, vert_count, index_offset, index_count, \
                    texture_id, material_flags, bounding_sphere = \
                    struct.unpack('<IIIIIIIf', model_data)
                    
                # Store model info for later
                self.models.append({
                    'id': model_id,
                    'vertex_offset': vert_offset,
                    'vertex_count': vert_count,
                    'index_offset': index_offset,
                    'index_count': index_count,
                    'texture_id': texture_id,
                    'material_flags': material_flags,
                    'bounding_sphere': bounding_sphere,
                })
                
                print(f"  Model {i}/{num_models}: {model_id:08X} "
                      f"(verts={vert_count}, indices={index_count})")
                      
    def generate_sr3_mesh(self):
        """Step 4: Generate SR3 mesh file"""
        print("\n=== Step 4: Generate SR3 Mesh ===")
        
        if not self.models:
            print("  No models to convert")
            return
            
        mesh_file = self.output_path / 'track.mesh'
        
        with open(mesh_file, 'wb') as f:
            # SR3 mesh header
            f.write(b'SR3M')  # Magic
            f.write(struct.pack('<I', 1))  # Version
            f.write(struct.pack('<I', len(self.models)))  # Model count
            
            # Write model data
            for model in self.models:
                # Model header
                f.write(struct.pack('<I', model['id']))
                f.write(struct.pack('<I', model['vertex_count']))
                f.write(struct.pack('<I', model['index_count']))
                f.write(struct.pack('<I', model['texture_id']))
                f.write(struct.pack('<I', model['material_flags']))
                f.write(struct.pack('<f', model['bounding_sphere']))
                
                # Note: Actual vertex/index data would come from
                # the extracted BUN data at the specified offsets
                # This is a placeholder for the full implementation
                
        print(f"  Generated: {mesh_file}")
        
    def generate_materials(self):
        """Step 5: Generate SR3 material file"""
        print("\n=== Step 5: Generate Materials ===")
        
        mat_file = self.output_path / 'materials.ini'
        
        with open(mat_file, 'w') as f:
            f.write("[materials]\n")
            
            for model in self.models:
                tex_id = model['texture_id']
                f.write(f"\n[mat_{model['id']:08X}]\n")
                f.write(f"texture = textures/tex_{tex_id:08X}.png\n")
                f.write(f"flags = {model['material_flags']}\n")
                
        print(f"  Generated: {mat_file}")


def main():
    if len(sys.argv) < 3:
        print("Usage: python CarbonModelConverter.py <input_path> <output_path>")
        print("Example: python CarbonModelConverter.py \"output\\L5RA\" \"data\\tracks\\CasinoTower\\\"")
        sys.exit(1)
        
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    
    converter = CarbonModelConverter(input_path, output_path)
    
    if converter.convert():
        print("\n✓ Conversion successful!")
        sys.exit(0)
    else:
        print("\n✗ Conversion failed!")
        sys.exit(1)


if __name__ == '__main__':
    main()
