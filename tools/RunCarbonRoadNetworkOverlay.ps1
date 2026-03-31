[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BlendPath,
    [string]$RoadObj = "D:\Repos\Games\NFS-ModTools\AssetDumper\output\modelulator\RoadNetworks\WRoadNetwork.obj",
    [string]$BlenderExe = "D:\Development\Blender K-Cycles\blender.exe",
    [string]$OutBlend,
    [double[]]$Translate = @(0.0, 0.0, 0.0),
    [string]$CollectionName = "Carbon_RoadNetwork"
)

$builderScript = Join-Path $PSScriptRoot "CarbonRoadNetworkOverlay.py"
if (-not (Test-Path $builderScript)) {
    throw "Missing Blender overlay builder: $builderScript"
}

if (-not (Test-Path $BlenderExe)) {
    throw "Missing Blender executable: $BlenderExe"
}

if (-not (Test-Path $BlendPath)) {
    throw "Missing input blend: $BlendPath"
}

if (-not (Test-Path $RoadObj)) {
    throw "Missing road OBJ: $RoadObj"
}

if (-not $OutBlend) {
    $inItem = Get-Item $BlendPath
    $OutBlend = Join-Path $inItem.Directory.FullName ($inItem.BaseName + "_roads.blend")
}

if ($Translate.Count -ne 3) {
    throw "Translate must contain exactly 3 values: X Y Z"
}

$args = @(
    "--background",
    "--factory-startup",
    "--python", $builderScript,
    "--",
    "--in-blend", $BlendPath,
    "--road-obj", $RoadObj,
    "--out-blend", $OutBlend,
    "--translate", "$($Translate[0])", "$($Translate[1])", "$($Translate[2])",
    "--collection-name", $CollectionName
)

Write-Host "Overlaying Carbon road network"
Write-Host "Input blend: $BlendPath"
Write-Host "Road OBJ: $RoadObj"
Write-Host "Output blend: $OutBlend"

& $BlenderExe @args
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "Road overlay build failed with exit code $exitCode"
}

Write-Host ""
Write-Host "Road overlay blend ready:"
Write-Host "  $OutBlend"
