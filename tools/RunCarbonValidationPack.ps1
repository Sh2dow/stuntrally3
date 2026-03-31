[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string[]]$SectionNumbers,
    [string]$PackName,
    [string]$UnifiedRoot = "f:\NFS\NFSC_Mods\W2C\Blender",
    [string]$BlenderExe = "D:\Development\Blender K-Cycles\blender.exe",
    [string]$OutputRoot,
    [switch]$SkipAssetDumper,
    [switch]$SkipModelulator,
    [switch]$HideAssetDumperReference = $true,
    [switch]$KeepNonLocalModelulator,
    [switch]$SkipColladaExport
)

function Resolve-SectionNumbers {
    param([string[]]$Values)

    $resolved = New-Object System.Collections.Generic.List[int]
    foreach ($value in $Values) {
        foreach ($piece in ($value -split ',')) {
            $trimmed = $piece.Trim()
            if (-not $trimmed) {
                continue
            }

            $parsed = 0
            if (-not [int]::TryParse($trimmed, [ref]$parsed)) {
                throw "Invalid section number: $trimmed"
            }
            $resolved.Add($parsed)
        }
    }

    return @($resolved | Sort-Object -Unique)
}

function New-PackName {
    param([int[]]$Numbers)

    if ($Numbers.Count -le 6) {
        return "sections_{0}" -f (($Numbers | ForEach-Object { "$_" }) -join '_')
    }

    return "sections_{0}_{1}_{2}count" -f $Numbers[0], $Numbers[-1], $Numbers.Count
}

$builderScript = Join-Path $PSScriptRoot "CarbonValidationPackBuilder.py"
if (-not (Test-Path $builderScript)) {
    throw "Missing Blender validation pack builder: $builderScript"
}

if (-not (Test-Path $BlenderExe)) {
    throw "Missing Blender executable: $BlenderExe"
}

$resolvedSections = Resolve-SectionNumbers -Values $SectionNumbers
if (-not $resolvedSections -or $resolvedSections.Count -eq 0) {
    throw "No valid section numbers were provided"
}

if (-not $PackName) {
    $PackName = New-PackName -Numbers $resolvedSections
}

if (-not $OutputRoot) {
    $OutputRoot = Join-Path $UnifiedRoot "validation_packs"
}

$outDir = Join-Path $OutputRoot $PackName
$args = @(
    "--background",
    "--factory-startup",
    "--python", $builderScript,
    "--",
    "--unified-root", $UnifiedRoot,
    "--section-numbers"
)

foreach ($number in $resolvedSections) {
    $args += "$number"
}

$args += @(
    "--out-dir", $outDir,
    "--pack-name", $PackName
)

if ($SkipAssetDumper) {
    $args += "--skip-assetdumper"
}

if ($SkipModelulator) {
    $args += "--skip-modelulator"
}

if ($HideAssetDumperReference) {
    $args += "--hide-assetdumper-reference"
}

if ($KeepNonLocalModelulator) {
    $args += "--keep-nonlocal-modelulator"
}

if ($SkipColladaExport) {
    $args += "--skip-collada-export"
}

Write-Host "Building Carbon validation pack '$PackName'"
Write-Host "Sections: $($resolvedSections -join ', ')"
Write-Host "Unified root: $UnifiedRoot"
Write-Host "Output dir: $outDir"

& $BlenderExe @args
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "Validation pack build failed with exit code $exitCode"
}

Write-Host ""
Write-Host "Validation pack ready:"
Write-Host "  $outDir"
