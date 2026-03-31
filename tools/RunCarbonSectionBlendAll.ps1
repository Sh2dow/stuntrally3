[CmdletBinding()]
param(
    [ValidateSet("Bridge", "Unified")]
    [string]$Source = "Bridge",
    [string]$UnifiedRoot = "f:\NFS\NFSC_Mods\W2C\Blender",
    [string]$BlenderExe = "D:\Development\Blender K-Cycles\blender.exe",
    [string]$OutputDir,
    [switch]$SkipAssetDumper,
    [switch]$SkipModelulator,
    [switch]$HideModelulator = $true,
    [switch]$HideModelulatorExternal,
    [int]$First = 0
)

$batchRunner = Join-Path $PSScriptRoot "RunCarbonSectionBlendBatch.ps1"
if (-not (Test-Path $batchRunner)) {
    throw "Missing batch runner: $batchRunner"
}

if (-not $OutputDir) {
    $OutputDir = Join-Path $UnifiedRoot "blender_sections"
}

switch ($Source) {
    "Bridge" {
        $bridgeCsv = Join-Path $UnifiedRoot "section_solid_bridge.csv"
        if (-not (Test-Path $bridgeCsv)) {
            throw "Missing bridge CSV: $bridgeCsv"
        }
        $sectionNumbers = (Import-Csv $bridgeCsv).section_number
    }
    "Unified" {
        $manifestPath = Join-Path $UnifiedRoot "unified_manifest.json"
        if (-not (Test-Path $manifestPath)) {
            throw "Missing unified manifest: $manifestPath"
        }
        $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
        $sectionNumbers = $manifest.sections.section_number
    }
}

if ($First -gt 0) {
    $sectionNumbers = @($sectionNumbers | Select-Object -First $First)
}

if (-not $sectionNumbers -or $sectionNumbers.Count -eq 0) {
    throw "No section numbers were resolved for source '$Source'"
}

Write-Host "Resolved $($sectionNumbers.Count) sections from source '$Source'"

$invokeArgs = @{
    SectionNumbers = $sectionNumbers
    UnifiedRoot = $UnifiedRoot
    BlenderExe = $BlenderExe
    OutputDir = $OutputDir
}

if ($SkipAssetDumper) {
    $invokeArgs.SkipAssetDumper = $true
}

if ($SkipModelulator) {
    $invokeArgs.SkipModelulator = $true
}

if ($HideModelulator) {
    $invokeArgs.HideModelulator = $true
}

if ($HideModelulatorExternal) {
    $invokeArgs.HideModelulatorExternal = $true
}

& $batchRunner @invokeArgs
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "Process-all wrapper failed with exit code $exitCode"
}
