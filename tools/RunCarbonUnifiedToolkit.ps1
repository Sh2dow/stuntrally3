[CmdletBinding()]
param(
    [string]$TrackId = "L5RA",
    [string]$RawInput,
    [string]$Bun,
    [string]$ModelulatorRoot,
    [string]$AssetDumperRoot,
    [string]$PngTexturesRoot,
    [string]$OutputRoot,
    [ValidateSet("hardlink", "copy", "skip")]
    [string]$LinkMode = "hardlink",
    [switch]$FullGeometryDetails
)

$repoRoot = Split-Path -Parent $PSScriptRoot
$pythonScript = Join-Path $PSScriptRoot "CarbonUnifiedToolkit.py"

if (-not (Test-Path $pythonScript)) {
    throw "Missing toolkit script: $pythonScript"
}

if (-not $RawInput) {
    $RawInput = "D:\Repos\Games\Binarius\Binary\output\${TrackId}_raw"
}

if (-not $Bun) {
    $Bun = "D:\Games\NFSC Redux\TRACKS\${TrackId}.BUN"
}

if (-not $ModelulatorRoot) {
    $ModelulatorRoot = "D:\Repos\Games\NFS-ModTools\AssetDumper\output\modelulator"
}

if (-not $AssetDumperRoot) {
    $AssetDumperRoot = "D:\Repos\Games\NFS-ModTools\AssetDumper\output\STREAM${TrackId}_test"
}

if (-not $PngTexturesRoot) {
    $PngTexturesRoot = "D:\Repos\Games\NFS-ModTools\AssetDumper\output\modelulator\Textures\STREAM${TrackId}.BUN_PNG"
}

if (-not $OutputRoot) {
    $OutputRoot = "D:\Repos\Games\NFS-ModTools\AssetDumper\output\${TrackId}_unified"
}

$args = @(
    $pythonScript,
    "--track-id", $TrackId,
    "--out", $OutputRoot,
    "--raw-input", $RawInput,
    "--bun", $Bun,
    "--modelulator", $ModelulatorRoot,
    "--assetdumper", $AssetDumperRoot,
    "--png-textures", $PngTexturesRoot,
    "--link-mode", $LinkMode
)

if ($FullGeometryDetails) {
    $args += "--full-geometry-details"
}

Write-Host "Running unified Carbon toolkit for $TrackId"
Write-Host "Output: $OutputRoot"

& python @args
$exitCode = $LASTEXITCODE

if ($exitCode -ne 0) {
    throw "CarbonUnifiedToolkit.py failed with exit code $exitCode"
}

Write-Host ""
Write-Host "Unified bundle ready:"
Write-Host "  $OutputRoot"
Write-Host "  $(Join-Path $OutputRoot 'unified_manifest.json')"
