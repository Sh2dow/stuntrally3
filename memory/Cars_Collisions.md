# Bot Cars Research - 2026-03-19

## Bot Car Implementation

### Car Types
The game supports several car types defined in `CarModel.h`:
```cpp
enum eCarType { CT_LOCAL=0, CT_REMOTE, CT_REPLAY, CT_GHOST, CT_GHOST2, CT_TRACK };
```

- `CT_LOCAL`: Local player (has physics + camera)
- `CT_REMOTE`: Networked car (has physics + camera)
- `CT_REPLAY`: Replay playback (has camera)
- `CT_GHOST`: Ghost car from file (no physics, visual only)
- `CT_GHOST2`: Another car's ghost file (no physics)
- `CT_TRACK`: Track's ghost file (no physics)

### Bot Car Creation
In `Game_SceneInit.cpp`, bot cars are created as follows:
```cpp
int numCars = mClient ? mClient->getPeerCount()+1 : pSet->game.local_players;
for (i = 0; i < numCars; ++i)
{
    CarModel::eCarType et = CarModel::CT_LOCAL;
    // ...
    CarModel* car = new CarModel(i, i, et, carName, &mCams[i], this);
    car->Load(startId, loop);
    carModels.push_back(car);
}
```

Bot cars should be created with `CT_LOCAL` type, which means they should have physics simulation.

### Key Function: isGhost()
```cpp
bool isGhost() const { return cType >= CT_GHOST; }
bool isGhostTrk() const { return cType == CT_TRACK; }
```

This means `CT_GHOST`, `CT_GHOST2`, and `CT_TRACK` are considered ghosts and don't have physics simulation.

### Ghost Car Creation (Replay Only)
Ghost cars for replay are created separately in `Game_SceneInit.cpp`:
```cpp
if (!bRplPlay && pSet->rpl_ghost && !mClient)
{
    CarModel* c = new CarModel(i, MAX_Players, CarModel::CT_GHOST, orgCar, 0, this);
    c->Load(-1, false);
    carModels.push_back(c);
}
```

### Render Queue Groups
In `RenderConst.h`, the `RQG_Ghost` render queue group is defined as `RQ_GlassV2 + 7` (value 87). This is used for "markers, debug" objects.

The `CreatePart` function in `CarModel_Create.cpp` has a commented-out line that would assign ghost cars to a special render queue:
```cpp
// if (ghost)  {  item->setRenderQueueGroup(RQG_CarGhost);  item->setCastShadows(false);  }
```

This suggests that ghost cars might have been assigned to a special render queue group that makes them appear differently, but this code is currently commented out.

### How Bot Cars Work
Bot cars in StuntRally3 are created as `CT_LOCAL` type in `Game_SceneInit.cpp`. This means they should:
1. Have physics simulation (CAR object created via `pGame->LoadCar()`)
2. Have camera view
3. Be fully functional cars with AI control

The issue with bot cars appearing as "ghosts" might be related to:
1. Car physics not being initialized properly
2. AI control not being enabled
3. Rendering issues with Vulkan rendering subsystem

### Potential Fixes for Bot Cars
1. Check if bot cars have proper AI control enabled in their `.car` config files
2. Verify that `pCar->setAI()` is being called for bot cars
3. Check if there are any issues with the car physics initialization
4. Verify that the cars are being rendered with correct visibility flags

## Implementation Plan for Bot Cars with Physics

### Current Issue
Bot cars are created as `CT_LOCAL` type and have physics simulation, but they appear as "ghosts" because they're not being controlled properly.

### Root Cause Analysis
1. **Car Creation**: Bot cars are created with `CT_LOCAL` type in `Game_SceneInit.cpp`
2. **Physics**: Each bot car has a `CAR` object created via `pGame->LoadCar()`
3. **Input Control**: The issue is in `UpdateCarInputs()` in `game.cpp`:
   - All cars use the same input state from `mPlayerInputState[id]`
   - For bot cars, this means they're using the player's input instead of AI control

### Implementation Strategy
The game has a performance test mode (`bPerfTest`) that provides automatic car inputs. To implement bot cars with physics:

1. **Enable Performance Test Mode for Bot Cars**: Modify `UpdateCarInputs()` to use performance test mode for bot cars
2. **Create AI Input Control**: Implement separate input controls for bot cars with AI logic

### Code Changes Required

#### In `Game_Update.cpp` or `Game_SceneInit.cpp`:
- Set `bPerfTest = true` for bot cars (non-player cars)
- This will enable automatic inputs via `ProcessInput()` with performance test stage

#### In `UpdateCarInputs()` in `game.cpp`:
```cpp
// Check if car is a bot (non-player)
bool isBotCar = (car.id != 0 && !mClient);  // Bot cars are non-zero IDs in local play

if (isBotCar && !bPerfTest) {
    // Enable perf test for bot cars
    bPerfTest = true;
    iPerfTestStage = PT_Accel;  // Or other appropriate stage
}
```

### Alternative Approach: Separate AI Controls
Create separate input controls for bot cars with AI logic:
1. Create `AIControl` class with AI driving logic
2. For each bot car, use AI to generate throttle, brake, steering inputs
3. Pass these inputs to `ProcessInput()` instead of player inputs

### Files to Modify
1. `D:\Repos\Games\OTHER GAMES\stuntrally3\src\vdrift\game.cpp` - `UpdateCarInputs()`
2. `D:\Repos\Games\OTHER GAMES\stuntrally3\src\game\Game_Update.cpp` - Bot car update logic
3. `D:\Repos\Games\OTHER GAMES\stuntrally3\src\game\CarModel_Update.cpp` - Car update logic

### Performance Test Stages
The performance test mode has these stages:
- `PT_StartWait`: Wait at start
- `PT_Accel`: Accelerate
- `PT_Brake`: Brake
- `PT_Circle`: Circle (testing)

## Implementation Results

### Code Changes Applied
Modified `UpdateCarInputs()` in `D:\Repos\Games\OTHER GAMES\stuntrally3\src\vdrift\game.cpp`:

```cpp
// Check if this is a bot car (non-zero ID in local play)
// Bot cars have car.id != 0 and are not networked (app->mClient == nullptr)
bool isBotCar = (car.id != 0 && app->mClient == nullptr);  // Bot cars are non-zero IDs in local play

// For bot cars, enable performance test mode to provide automatic inputs
bool bPerfTest = app->bPerfTest || isBotCar;
EPerfTest iPerfTestStage = app->iPerfTestStage;

// For bot cars, use Accel stage (drive forward)
if (isBotCar && !app->bPerfTest) {
    bPerfTest = true;
    iPerfTestStage = PT_Accel;
}
```

### Build Results
- **SR-Editor3.exe**: Built successfully
- **SR-Translator.exe**: Built successfully  
- **StuntRally3.exe**: Built successfully

### Executables Deployed
All executables copied to `D:\Repos\Games\OTHER GAMES\stuntrally3\bin\Release\`:
- SR-Editor3.exe
- SR-Translator.exe
- StuntRally3.exe

### How Bot Cars Work Now
Bot cars are now controlled using the performance test mode:
1. Bot cars are identified by `car.id != 0` and `app->mClient == nullptr`
2. Performance test mode is enabled with `PT_Accel` stage (drive forward)
3. Bot cars receive automatic throttle inputs and will drive forward
4. This provides physics-based AI control without needing separate AI logic

### Limitations
- Bot cars drive in a straight line (PT_Accel mode)
- No steering control for bot cars
- For more sophisticated AI, separate AI control logic would need to be implemented