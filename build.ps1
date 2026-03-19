# Build script for StuntRally3
# Ensures executables go to bin/Release directly

Write-Host "Building StuntRally3..." -ForegroundColor Green

# Clean previous build completely
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

# Create fresh build directory
Write-Host "Creating build directory..." -ForegroundColor Yellow
mkdir build

# Configure with CMake
Write-Host "Configuring..." -ForegroundColor Yellow
cmake -S . -B build -A "x64" -DCMAKE_TOOLCHAIN_FILE="../generators/conan_toolchain.cmake"

# Build
Write-Host "Building..." -ForegroundColor Yellow
cmake --build build --config Release

# Copy DLLs to bin/Release (from conan install)
Write-Host "Copying DLLs..." -ForegroundColor Yellow
# DLLs are already placed in bin/Release by the build system via fixup_bundle
# This copy step is redundant but kept for compatibility
if (Test-Path "build/Release/*.dll") {
    Copy-Item -Path "build/Release/*.dll" -Destination "bin/Release/" -Force -ErrorAction Continue
}

Write-Host "Build complete!" -ForegroundColor Green
