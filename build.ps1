# Build script for StuntRally3
# Ensures executables go to bin/Release directly

Write-Host "Building StuntRally3..." -ForegroundColor Green

# Clean previous build directory
if (Test-Path "build") {
    Write-Host "Removing build directory..." -ForegroundColor Yellow
    # Use a retry loop to handle file locking issues
    $maxRetries = 5
    $retryCount = 0
    while ($retryCount -lt $maxRetries) {
        try {
            Remove-Item -Recurse -Force "build"
            break
        } catch {
            $retryCount++
            Write-Host "File in use, retrying ($retryCount/$maxRetries)..." -ForegroundColor Yellow
            Start-Sleep -Seconds 2
        }
    }
}

# Configure with CMake
Write-Host "Configuring..." -ForegroundColor Yellow
cmake -S . -B build -A "x64" -DCMAKE_TOOLCHAIN_FILE="../generators/conan_toolchain.cmake"

# Build
Write-Host "Building..." -ForegroundColor Yellow
cmake --build build --config Release

# DLLs are placed in bin/Release by the build system via fixup_bundle

Write-Host "Build complete!" -ForegroundColor Green
