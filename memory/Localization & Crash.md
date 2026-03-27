## Crash Issue Analysis

### Crash Details
- **Application**: SR-Editor3.exe
- **Faulting Module**: RenderSystem_Vulkan.dll
- **Exception Code**: 0xc0000005 (access violation)
- **Fault Offset**: 0x0000000000035370

### Potential Causes
1. **Vulkan Rendering Issues**: The game uses Vulkan rendering subsystem
2. **DLL Loading**: Some DLLs might be in wrong location
3. **Resource Loading**: Access violation during resource initialization
4. **Car Model Creation**: Access violation when creating car models with Vulkan rendering

### DLL File Locations
- Some executables are built in `build/Release/`
- Some executables are built in `bin/Release/`
- DLLs should be copied properly to executable directory

### Vulkan Rendering Configuration
- The game uses Vulkan rendering subsystem by default
- Settings file: `D:\Users\sh2dow\AppData\Roaming\stuntrally3\ogre.cfg`
- Render System: `Vulkan Rendering Subsystem`
- Video Mode: 1920 x 1080

### Possible Solutions
1. Try switching to OpenGL rendering subsystem
2. Update graphics drivers
3. Verify Vulkan installation
4. Check if car models are being loaded correctly
5. Ensure all required DLLs are in the executable directory

## Language Configuration

### Files Updated
- `config/game-default.cfg`: `language = en`
- `config/custom-config.cfg`: `language = en`
- `D:\Users\sh2dow\AppData\Roaming\stuntrally3\game.cfg`: `language = en`
- `D:\Users\sh2dow\AppData\Roaming\stuntrally3\editor.cfg`: `language = en`

### Language Loading
The game loads language from settings files:
1. User-specific settings in `%APPDATA%\stuntrally3\`
2. Default settings in `config/` directory

## Files Modified/Created
- `D:\Repos\Games\OTHER GAMES\stuntrally3\CMake\FixupBundle.cmake`
- `D:\Repos\Games\OTHER GAMES\stuntrally3\build.ps1`
- `D:\Repos\Games\OTHER GAMES\stuntrally3\config\game-default.cfg`
- `D:\Repos\Games\OTHER GAMES\stuntrally3\config\custom-config.cfg`
- `D:\Repos\Games\OTHER GAMES\stuntrally3\src\vdrift\game.cpp` - Modified `UpdateCarInputs()` to enable performance test mode for bot cars
- `D:\Users\sh2dow\AppData\Roaming\stuntrally3\game.cfg`
- `D:\Users\sh2dow\AppData\Roaming\stuntrally3\editor.cfg`
