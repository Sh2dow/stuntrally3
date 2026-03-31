# Carbon W2C Track Porting Pipeline

## Status: Binarius Extraction COMPLETE ✅

All Carbon track data has been successfully extracted to:
`D:\Repos\Games\Binarius\Binary\output\L5RA_raw\`

## Extracted Data Structure

### Main Files:
- **Y0** (98MB) - Main track geometry/vertices
- **X*** - Streaming sections and LOD data
- **A*** - Geometry chunks (meshes, materials)
- **B*** - Barriers and boundaries
- **C*** - Collision data
- **Z*** - Additional track data

### File Organization:
```
L5RA_raw/
├── Y0          # Main geometry (98MB)
├── X0-X900     # Streaming sections
├── A1-A900     # Geometry chunks
├── B1-B400     # Barriers
├── C1-C900     # Collision
└── Z0          # Track metadata
```

---

## Phase 1: Carbon Track Format Analysis

### Step 1.1: Analyze Y0 (Main Geometry)

**Action:** Examine Y0 file structure to understand:
- Vertex format (position, normal, UV, etc.)
- Index buffer format
- Material assignments
- Section boundaries

**Tools Needed:**
- Hex editor
- Carbon W2C format documentation (if available)
- Reverse engineering from extracted data

### Step 1.2: Analyze Track Metadata

**Action:** Parse track metadata to understand:
- Track name/ID
- Track length
- Checkpoint positions
- Racing line data
- Zone definitions (jump, canyon, vertigo, etc.)

**Source Files:**
- `Z0` - Likely track metadata
- `A*` chunks - May contain zone data
- `B*` chunks - Barrier definitions

### Step 1.3: Map Carbon Chunks to SR3 Format

**Carbon Format → SR3 Format:**
```
Carbon Y0 vertices  →  SR3 track vertices
Carbon X* sections  →  SR3 track sections
Carbon B* barriers  →  SR3 barrier objects
Carbon C* collision →  SR3 collision mesh
Carbon zones        →  SR3 zone triggers
```

---

## Phase 2: Carbon Track Converter Tool

### Step 2.1: Create CarbonFormatParser.cs

**Location:** `tools\CarbonTrackConverter\CarbonFormatParser.cs`

**Purpose:** Parse raw Carbon binary files

**Structure:**
```csharp
public class CarbonTrackFile
{
    public string FileName { get; set; }
    public byte[] Data { get; set; }
    public CarbonTrackHeader Header { get; set; }
}

public class CarbonTrackHeader
{
    public uint Magic { get; set; }
    public uint Version { get; set; }
    public uint ChunkCount { get; set; }
    // ...
}

public class CarbonTrackParser
{
    public static CarbonTrackFile Load(string filePath)
    {
        // Read binary file
        // Parse header
        // Extract chunks
    }
    
    public static List<CarbonVertex> ParseVertices(byte[] data)
    {
        // Parse vertex buffer
        // Extract position, normal, UV, etc.
    }
    
    public static List<CarbonIndex> ParseIndices(byte[] data)
    {
        // Parse index buffer
    }
}
```

### Step 2.2: Create SR3TrackExporter.cs

**Location:** `tools\CarbonTrackConverter\SR3TrackExporter.cs`

**Purpose:** Convert parsed Carbon data to SR3 format

**Structure:**
```csharp
public class SR3TrackExporter
{
    public static void Export(CarbonTrackData carbonData, string outputPath)
    {
        // Convert vertices
        var vertices = ConvertVertices(carbonData.Vertices);
        
        // Convert indices
        var indices = ConvertIndices(carbonData.Indices);
        
        // Convert materials
        var materials = ConvertMaterials(carbonData.Materials);
        
        // Write SR3 track file
        WriteSR3Track(outputPath, vertices, indices, materials);
    }
    
    private static SR3Vertex ConvertVertex(CarbonVertex carbonVertex)
    {
        return new SR3Vertex
        {
            Position = carbonVertex.Position,
            Normal = carbonVertex.Normal,
            UV = carbonVertex.UV,
            // Map Carbon format to SR3 format
        };
    }
}
```

### Step 2.3: Create Main Converter Program

**Location:** `tools\CarbonTrackConverter\Program.cs`

**Purpose:** Command-line tool for batch conversion

**Usage:**
```bash
CarbonTrackConverter.exe <input_dir> <output_dir> [track_id]

Examples:
CarbonTrackConverter.exe "output\L5RA_raw" "data\tracks" L5RA
CarbonTrackConverter.exe "output\L3RA_raw" "data\tracks" L3RA
```

**Code:**
```csharp
class Program
{
    static void Main(string[] args)
    {
        if (args.Length < 2)
        {
            Console.WriteLine("Usage: CarbonTrackConverter <input_dir> <output_dir> [track_id]");
            return;
        }
        
        string inputDir = args[0];
        string outputDir = args[1];
        string trackId = args.Length > 2 ? args[2] : "Unknown";
        
        Console.WriteLine($"Converting Carbon track from {inputDir}...");
        
        // Parse Carbon files
        var carbonData = CarbonTrackParser.LoadDirectory(inputDir);
        
        // Convert to SR3 format
        SR3TrackExporter.Export(carbonData, Path.Combine(outputDir, trackId));
        
        Console.WriteLine($"Conversion complete! Output: {outputDir}/{trackId}");
    }
}
```

---

## Phase 3: Integration with SR3

### Step 3.1: Add Carbon Track Loader to SR3

**Location:** `src\game\CarbonTrackLoader.cpp` (new file)

**Purpose:** Load converted Carbon tracks in SR3

**Structure:**
```cpp
class CarbonTrackLoader
{
public:
    static bool LoadTrack(const std::string& trackPath, TrackData& outData);
    
private:
    static bool LoadVertices(const std::string& filePath, std::vector<Vertex>& vertices);
    static bool LoadIndices(const std::string& filePath, std::vector<uint32_t>& indices);
    static bool LoadMaterials(const std::string& filePath, std::vector<Material>& materials);
    static bool LoadZones(const std::string& filePath, std::vector<Zone>& zones);
};
```

### Step 3.2: Update Career Event System

**Location:** `src\game\EventMode.cpp`

**Changes:**
- Add Carbon track loading support
- Map Carbon track IDs to events
- Update event configuration to use Carbon tracks

---

## Immediate Next Steps

### TODAY: Analyze Extracted Data

1. **Open Y0 in hex editor**
   - Identify header structure
   - Find vertex data patterns
   - Identify index data patterns

2. **Compare with known Carbon format**
   - Check hyperlinked documentation
   - Look for vertex format definitions
   - Find chunk type identifiers

3. **Document Carbon track format**
   - Create format specification
   - Document vertex structure
   - Document chunk organization

### TOMORROW: Create Parser

1. **Create CarbonFormatParser.cs**
   - Implement binary file reading
   - Parse Carbon headers
   - Extract vertex/index data

2. **Test parser on Y0**
   - Verify vertex extraction
   - Verify index extraction
   - Check data integrity

### DAY 3: Create Exporter

1. **Create SR3TrackExporter.cs**
   - Implement format conversion
   - Map Carbon → SR3 formats
   - Write SR3 track files

2. **Test conversion**
   - Convert L5RA track
   - Load in SR3
   - Verify geometry displays correctly

---

## Tools & Resources

### Available:
- ✅ Binarius (extraction working)
- ✅ Extracted Carbon data (L5RA_raw)
- ✅ hyperlinked documentation
- ✅ ida-pro-mcp (for reverse engineering)

### Need to Create:
- ⏳ CarbonFormatParser.cs
- ⏳ SR3TrackExporter.cs
- ⏳ CarbonTrackConverter.exe
- ⏳ CarbonTrackLoader.cpp (for SR3)

---

## Success Criteria

### Phase 1 Complete When:
- [ ] Carbon track format documented
- [ ] Parser can extract vertices/indices
- [ ] Converter creates valid SR3 track files
- [ ] SR3 can load Carbon track
- [ ] Track displays correctly in-game

### Estimated Timeline:
- **Format Analysis:** 1-2 days
- **Parser Development:** 2-3 days
- **Exporter Development:** 2-3 days
- **SR3 Integration:** 2-3 days
- **Testing & Debugging:** 2-3 days

**Total: 9-14 days for first working Carbon track**

---

## Questions to Answer

1. **What is the Carbon vertex format?**
   - Position (x,y,z)?
   - Normal (x,y,z)?
   - UV (u,v)?
   - Additional data?

2. **How are chunks organized?**
   - What does each chunk type contain?
   - How to reassemble Y0 + X* chunks?

3. **What is the SR3 track format?**
   - Existing SR3 track file structure
   - Vertex format SR3 expects
   - How SR3 loads tracks

---

## Let's Start: Analyze Y0 File

**First Action:** Open Y0 in hex editor and document the structure.

**What to look for:**
- File header (magic number, version, etc.)
- Vertex count
- Index count
- Data sections
- Chunk boundaries

**Tools:**
- HxD or similar hex editor
- Note-taking for structure documentation
- Compare with known Carbon format specs if available

Shall I create the Carbon format parser now, or do you want to analyze the Y0 file structure first?
