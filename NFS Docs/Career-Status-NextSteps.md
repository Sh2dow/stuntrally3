# SR3 Career Mode: Current Status & Next Steps

## ✅ What's Complete

### Phase 1: Career Foundation
- **Career GUI Window** - Full UI with 10 district buttons
- **Career Progression System** - Rep, cash, levels, boss tracking
- **District Unlock Logic** - Based on reputation and bosses defeated
- **Career Save/Load** - XML-based career state persistence
- **Career Track Assignments** - `career_tracks.xml` with events per district

### Phase 2: Track Metadata Extraction
- **CarbonTrackParser_Enhanced.py** - Extracts Carbon track metadata
- **Track Zone Extraction** - Jump camera, canyon drop, vertigo zones
- **Barrier Extraction** - Track barriers from `TroughBoundary.bin`
- **Streaming Section Inventory** - Lists all sections from `TRACKS\*.BUN`

### Phase 3: Tools & Documentation
- **NFSC-Track-Port-Pipeline.md** - Complete porting guide
- **CarbonTrackExtractor.py** - Section/zone extraction tool
- **hyperlinked integration** - All Carbon chunk IDs documented
- **ida-pro-mcp active** - Live Carbon.exe reverse engineering

---

## ❌ What's Missing

### Critical Gap: No Playable Career Tracks

**Current State:**
```xml
<!-- career_tracks.xml -->
<event id="downtown_1" type="sprint" track="Isl2-Sandy" .../>
```

**Problem:** `Isl2-Sandy` is a **stunt track**, not a proper racing circuit.

**Career needs:**
- Proper racing circuits (2-5 km length)
- Multiple laps possible
- Clear start/finish lines
- Defined racing line for AI
- Checkpoints for position tracking

---

## 🔧 Solutions (In Priority Order)

### Option A: Port Carbon Tracks (Authentic but Complex)

**Use your existing tools:**
1. **Binarius** - Extract `.BUN` files
2. **CarbonTrackExtractor.py** - Parse sections/zones
3. **hyperlinked** - Chunk structure reference
4. **ida-pro-mcp** - Verify structures

**Pipeline:**
```
Carbon BUN → Extract sections → Convert models → Generate collision → SR3 format
```

**Time:** 2-3 weeks for first track, then 1 week per track

**Pros:**
- Authentic Carbon experience
- All 10 districts have proper tracks
- Zone triggers work (jump, canyon, vertigo)

**Cons:**
- Complex model/texture conversion
- Need to reverse BUN format details
- Collision mesh generation required

---

### Option B: Create Tracks in SR3 Editor (Easier but Manual)

**Use SR3's built-in Track Editor:**
1. Create 10 circuits (one per district)
2. Varying lengths: 2km (easy) to 5km (hard)
3. Add proper start/finish, checkpoints
4. Define racing line for AI

**Time:** 1-2 days per track = 2-3 weeks total

**Pros:**
- Native SR3 format (no conversion)
- Full editor support
- Immediate testing

**Cons:**
- Manual work
- Not authentic Carbon tracks
- Requires track design skills

---

### Option C: Hybrid (Recommended)

**Phase 1 (Immediate - Week 1):**
- Use **best existing SR3 tracks** for career
- Update `career_tracks.xml` with real circuits
- Make career **playable now**

**Suitable SR3 tracks:**
- `Uni7-GlassStairs` - Long circuit
- `Mos10-City` - Urban track
- `Grc13-Acropolis` - Mountain circuit
- `Isl14-Ocean` - Coastal track
- `Mos5-Factory` - Industrial

**Phase 2 (Short-term - Weeks 2-4):**
- Port **1-2 Carbon tracks** as proof-of-concept
- Use Binarius + CarbonTrackExtractor
- Test full pipeline

**Phase 3 (Medium-term - Months 2-3):**
- Port remaining Carbon tracks
- Replace SR3 placeholder tracks

---

## 📋 Immediate Next Steps

### This Week: Make Career Playable

1. **Update `career_tracks.xml`** with existing SR3 circuits:
   ```xml
   <district id="1" name="Downtown">
       <event type="sprint" track="Uni7-GlassStairs" laps="1"/>
       <event type="circuit" track="Mos10-City" laps="2"/>
   </district>
   ```

2. **Test career flow:**
   - Open career window
   - Select district
   - Start event
   - Complete race
   - Earn rep/cash
   - Unlock next district

3. **Fix any GUI issues:**
   - District button clicks
   - Event start integration
   - Career progress updates

### Next Week: Start Carbon Porting

1. **Extract L5RA (Casino Tower) with Binarius:**
   ```bash
   cd d:\Repos\Games\Binarius
   CarbonStream.exe extract "D:\Games\NFSC Redux\TRACKS\L5RA.BUN" "output\L5RA\"
   ```

2. **Run CarbonTrackExtractor:**
   ```bash
   python tools/CarbonTrackExtractor.py "output\L5RA" "data\tracks\CasinoTower\"
   ```

3. **Analyze output:**
   - Check `sections.json` - section count/bounds
   - Check `zones.xml` - zone types/positions
   - Check `inventory.json` - chunk inventory

4. **Plan model conversion:**
   - Identify model format
   - Write converter to SR3 `.mesh`
   - Test in SR3 viewer

---

## 🎯 Success Criteria

### Immediate (Week 1):
- ✅ Career window opens
- ✅ Can select districts
- ✅ Can start races with existing SR3 tracks
- ✅ Career progress saves/loads
- ✅ Rep/cash earned from races

### Short-term (Month 1):
- ✅ 1 Carbon track ported and working
- ✅ Model conversion pipeline established
- ✅ Collision generation working
- ✅ Zone triggers functional

### Medium-term (Month 2-3):
- ✅ All 10 districts have Carbon tracks
- ✅ Career mode fully playable
- ✅ All zone types working (jump, canyon, vertigo)
- ✅ Boss battles on Carbon tracks

---

## 📁 Key Files

### Career System:
- `src/game/Career.h` - Career data structures
- `src/game/Career.cpp` - Career manager implementation
- `src/game/CGui_Menu.cpp` - Career GUI (`ShowCareerWnd()`)
- `data/career/career_tracks.xml` - Track assignments
- `data/gui/Game_Main.layout` - Career window layout

### Track Extraction:
- `NFS Docs/CarbonTrackParser_Enhanced.py` - Metadata extractor
- `tools/CarbonTrackExtractor.py` - Section/zone extractor
- `NFS Docs/NFSC-Track-Port-Pipeline.md` - Porting guide
- `NFS Docs/NFSC-Track-Usage-Research.md` - Carbon research

### External Tools:
- `d:\Repos\Games\Binarius\CarbonStream.cs` - BUN extractor
- `d:\Repos\Games\NFSC\hyperlinked\` - Carbon chunk docs
- `ida-pro-mcp` - Live Carbon.exe analysis

---

## 🤔 Decision Needed

**Which approach do you want to pursue first?**

**A)** Quick win: Use existing SR3 tracks for career (1-2 days)
**B)** Full port: Start Carbon track extraction pipeline (2-3 weeks)
**C)** Hybrid: Both in parallel (SR3 tracks now, Carbon tracks later)

**My recommendation: Option C**
- Get career playable immediately with SR3 tracks
- Start Carbon porting in background
- Replace tracks as they're completed
