[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string[]]$SectionNumbers,
    [string]$UnifiedRoot = "f:\NFS\NFSC_Mods\W2C\Blender",
    [string]$BlenderExe = "D:\Development\Blender K-Cycles\blender.exe",
    [string]$OutputDir,
    [switch]$SkipAssetDumper,
    [switch]$SkipModelulator,
    [switch]$HideModelulator = $true,
    [switch]$HideModelulatorExternal
)

$singleRunner = Join-Path $PSScriptRoot "RunCarbonSectionBlend.ps1"
if (-not (Test-Path $singleRunner)) {
    throw "Missing single-section runner: $singleRunner"
}

if (-not $OutputDir) {
    $OutputDir = Join-Path $UnifiedRoot "blender_sections"
}

$expandedSectionNumbers = @()
foreach ($value in $SectionNumbers) {
    foreach ($piece in ($value -split ',')) {
        $trimmed = $piece.Trim()
        if (-not $trimmed) {
            continue
        }

        $parsed = 0
        if (-not [int]::TryParse($trimmed, [ref]$parsed)) {
            throw "Invalid section number: $trimmed"
        }
        $expandedSectionNumbers += $parsed
    }
}

foreach ($sectionNumber in $expandedSectionNumbers) {
    $args = @(
        "-ExecutionPolicy", "Bypass",
        "-File", $singleRunner,
        "-SectionNumber", "$sectionNumber",
        "-UnifiedRoot", $UnifiedRoot,
        "-BlenderExe", $BlenderExe,
        "-OutputDir", $OutputDir
    )

    if ($SkipAssetDumper) {
        $args += "-SkipAssetDumper"
    }

    if ($SkipModelulator) {
        $args += "-SkipModelulator"
    }

    if ($HideModelulator) {
        $args += "-HideModelulator"
    }

    if ($HideModelulatorExternal) {
        $args += "-HideModelulatorExternal"
    }

    & powershell @args
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        throw "Batch stopped at section $sectionNumber with exit code $exitCode"
    }
}

Write-Host ""
Write-Host "Batch complete:"
Write-Host "  $OutputDir"
