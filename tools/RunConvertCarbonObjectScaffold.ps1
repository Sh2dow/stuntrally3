[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$TrackId,
    [string]$TrackRoot,
    [string]$MeshToolExe = "D:\Users\sh2dow\.conan2\p\ogre31836e56c68908\build-meshtool\bin\release\OgreMeshTool.exe",
    [string]$ObjectsDir,
    [int]$First = 0,
    [int]$MaxCount = 0,
    [switch]$Overwrite
)

$scriptPath = Join-Path $PSScriptRoot "ConvertCarbonObjectScaffold.py"
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path $scriptPath)) {
    throw "Missing conversion script: $scriptPath"
}

if (-not $TrackRoot) {
    $TrackRoot = Join-Path (Join-Path $repoRoot "data\tracks") $TrackId
}
if (-not $ObjectsDir) {
    $ObjectsDir = Join-Path $repoRoot "data\models\objectsC"
}
if (-not (Test-Path $MeshToolExe)) {
    throw "Missing OgreMeshTool executable: $MeshToolExe"
}

$args = @(
    $scriptPath,
    "--track-root", $TrackRoot,
    "--mesh-tool", $MeshToolExe,
    "--objects-dir", $ObjectsDir
)
if ($First -gt 0) {
    $args += @("--first", $First)
}
if ($MaxCount -gt 0) {
    $args += @("--max-count", $MaxCount)
}
if ($Overwrite) {
    $args += "--overwrite"
}

python @args
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "Carbon object scaffold conversion failed with exit code $exitCode"
}

Write-Host ""
Write-Host "Mesh conversion complete:"
Write-Host "  Track root: $TrackRoot"
Write-Host "  Objects dir: $ObjectsDir"
Write-Host "  Report: $(Join-Path (Join-Path $TrackRoot 'objects_scaffold') 'mesh_conversion_report.json')"
