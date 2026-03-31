[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [int]$SectionNumber,
    [string]$UnifiedRoot = "f:\NFS\NFSC_Mods\W2C\Blender",
    [string]$BlenderExe = "D:\Development\Blender K-Cycles\blender.exe",
    [string]$OutputDir,
    [switch]$SkipAssetDumper,
    [switch]$SkipModelulator,
    [switch]$HideModelulator = $true,
    [switch]$HideModelulatorExternal
)

$builderScript = Join-Path $PSScriptRoot "CarbonSectionBlendBuilder.py"
if (-not (Test-Path $builderScript)) {
    throw "Missing Blender builder script: $builderScript"
}

if (-not (Test-Path $BlenderExe)) {
    throw "Missing Blender executable: $BlenderExe"
}

if (-not $OutputDir) {
    $OutputDir = Join-Path $UnifiedRoot "blender_sections"
}

$outBlend = Join-Path $OutputDir ("{0}.blend" -f $SectionNumber)
$args = @(
    "--background",
    "--factory-startup",
    "--python", $builderScript,
    "--",
    "--unified-root", $UnifiedRoot,
    "--section-number", "$SectionNumber",
    "--out-blend", $outBlend
)

if ($SkipAssetDumper) {
    $args += "--skip-assetdumper"
}

if ($SkipModelulator) {
    $args += "--skip-modelulator"
}

if ($HideModelulator) {
    $args += "--hide-modelulator"
}

if ($HideModelulatorExternal) {
    $args += "--hide-modelulator-external"
}

Write-Host "Building Blender scene for Carbon section $SectionNumber"
Write-Host "Unified root: $UnifiedRoot"
Write-Host "Output: $outBlend"

& $BlenderExe @args
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "Blender scene build failed with exit code $exitCode"
}

Write-Host ""
Write-Host "Blend ready:"
Write-Host "  $outBlend"
