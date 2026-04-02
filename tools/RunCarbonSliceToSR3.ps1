[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BlendPath,
    [Parameter(Mandatory = $true)]
    [string]$TrackName,
    [string]$BlenderExe = "D:\Development\Blender K-Cycles\blender.exe",
    [string]$OutputRoot,
    [switch]$RunTrackBuilder
)

$builderScript = Join-Path $PSScriptRoot "CarbonSliceToSR3.py"
$trackBuilderScript = Join-Path $PSScriptRoot "SR3TrackBuilder.py"

if (-not (Test-Path $builderScript)) {
    throw "Missing Blender SR3 exporter: $builderScript"
}

if (-not (Test-Path $BlenderExe)) {
    throw "Missing Blender executable: $BlenderExe"
}

if (-not (Test-Path $BlendPath)) {
    throw "Missing input blend: $BlendPath"
}

if (-not $OutputRoot) {
    $OutputRoot = Join-Path (Split-Path -Parent $BlendPath) ("sr3_export_" + $TrackName)
}

$args = @(
    "--background",
    "--factory-startup",
    "--python", $builderScript,
    "--",
    "--in-blend", $BlendPath,
    "--out-dir", $OutputRoot,
    "--track-name", $TrackName
)

if ($RunTrackBuilder) {
    $args += @(
        "--run-track-builder",
        "--builder-script", $trackBuilderScript
    )
}

Write-Host "Exporting Carbon slice to SR3"
Write-Host "Input blend: $BlendPath"
Write-Host "Track name: $TrackName"
Write-Host "Output root: $OutputRoot"

& $BlenderExe @args
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "Carbon slice to SR3 export failed with exit code $exitCode"
}

Write-Host ""
Write-Host "SR3 export ready:"
Write-Host "  $OutputRoot"
