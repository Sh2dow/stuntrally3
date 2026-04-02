[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BlendPath,
    [Parameter(Mandatory = $true)]
    [string]$TrackId,
    [string]$BlenderExe = "D:\Development\Blender K-Cycles\blender.exe",
    [string]$OutputRoot,
    [string]$Prefix,
    [double]$ChunkSize = 500.0,
[int]$MaxChunks = 0
)

$builderScript = Join-Path $PSScriptRoot "CarbonObjectScaffoldBuilder.py"
$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not (Test-Path $builderScript)) {
    throw "Missing object scaffold builder: $builderScript"
}
if (-not (Test-Path $BlenderExe)) {
    throw "Missing Blender executable: $BlenderExe"
}
if (-not (Test-Path $BlendPath)) {
    throw "Missing input blend: $BlendPath"
}

if (-not $OutputRoot) {
    $OutputRoot = Join-Path (Join-Path (Join-Path $repoRoot "data\tracks") $TrackId) "objects_scaffold"
}
if (-not $Prefix) {
    $Prefix = ($TrackId.ToLower() + "_chunk")
}

$args = @(
    "--background",
    "--factory-startup",
    "--python", $builderScript,
    "--",
    "--in-blend", $BlendPath,
    "--out-dir", $OutputRoot,
    "--track-id", $TrackId,
    "--prefix", $Prefix,
    "--chunk-size", $ChunkSize
)

if ($MaxChunks -gt 0) {
    $args += @("--max-chunks", $MaxChunks)
}

Write-Host "Generating Carbon object scaffold"
Write-Host "Input blend: $BlendPath"
Write-Host "Track: $TrackId"
Write-Host "Output root: $OutputRoot"
Write-Host "Chunk size: $ChunkSize"

& $BlenderExe @args
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "Carbon object scaffold build failed with exit code $exitCode"
}

Write-Host ""
Write-Host "Object scaffold ready:"
Write-Host "  $OutputRoot"
Write-Host ""
Write-Host "Key outputs:"
Write-Host "  $(Join-Path $OutputRoot 'objects_scaffold_manifest.json')"
Write-Host "  $(Join-Path $OutputRoot 'scene_objects_scaffold.xml')"
Write-Host "  $(Join-Path $OutputRoot 'mesh_xml')"
