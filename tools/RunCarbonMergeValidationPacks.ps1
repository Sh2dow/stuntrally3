[CmdletBinding(DefaultParameterSetName = "Explicit")]
param(
    [Parameter(ParameterSetName = "Explicit", Mandatory = $true)]
    [string[]]$PackNames,

    [Parameter(ParameterSetName = "Index", Mandatory = $true)]
    [string[]]$ChunkIndices,

    [string]$PackName,
    [string]$ParallelRoot = "f:\NFS\NFSC_Mods\W2C\Blender\validation_packs_parallel\sections_bridge",
    [string]$BlenderExe = "D:\Development\Blender K-Cycles\blender.exe",
    [string]$OutputRoot,
    [switch]$SkipColladaExport
)

function Resolve-PackNamesFromIndices {
    param(
        [string[]]$Indices,
        [string]$Root
    )

    $parallelManifest = Join-Path $Root "parallel_manifest.json"
    if (-not (Test-Path $parallelManifest)) {
        throw "Missing parallel manifest: $parallelManifest"
    }

    $manifest = Get-Content $parallelManifest -Raw | ConvertFrom-Json
    $byIndex = @{}
    $current = 1
    foreach ($pack in $manifest.packs) {
        $byIndex[$current] = [string]$pack.pack_name
        $current++
    }

    $resolvedIndices = @()
    foreach ($value in $Indices) {
        foreach ($piece in ($value -split ',')) {
            $trimmed = $piece.Trim()
            if (-not $trimmed) {
                continue
            }

            $parsed = 0
            if (-not [int]::TryParse($trimmed, [ref]$parsed)) {
                throw "Invalid chunk index: $trimmed"
            }
            $resolvedIndices += $parsed
        }
    }

    $resolved = @()
    foreach ($index in ($resolvedIndices | Select-Object -Unique)) {
        if (-not $byIndex.ContainsKey($index)) {
            throw "Chunk index out of range: $index"
        }
        $resolved += $byIndex[$index]
    }
    return @($resolved | Select-Object -Unique)
}

function New-PackName {
    param([string[]]$Names)
    if ($Names.Count -le 4) {
        return "merged_{0}" -f (($Names | ForEach-Object { $_ }) -join '_')
    }
    return "merged_{0}_{1}_{2}packs" -f $Names[0], $Names[-1], $Names.Count
}

$builderScript = Join-Path $PSScriptRoot "CarbonMergeValidationPacks.py"
if (-not (Test-Path $builderScript)) {
    throw "Missing Blender merge builder: $builderScript"
}

if (-not (Test-Path $BlenderExe)) {
    throw "Missing Blender executable: $BlenderExe"
}

if (-not $OutputRoot) {
    $OutputRoot = Join-Path $ParallelRoot "merged"
}

$resolvedPackNames = if ($PSCmdlet.ParameterSetName -eq "Index") {
    Resolve-PackNamesFromIndices -Indices $ChunkIndices -Root $ParallelRoot
} else {
    @($PackNames | Select-Object -Unique)
}

if (-not $resolvedPackNames -or $resolvedPackNames.Count -eq 0) {
    throw "No pack names were resolved"
}

if (-not $PackName) {
    $PackName = New-PackName -Names $resolvedPackNames
}

$outDir = Join-Path $OutputRoot $PackName
$args = @(
    "--background",
    "--factory-startup",
    "--python", $builderScript,
    "--",
    "--parallel-root", $ParallelRoot,
    "--pack-names"
)

foreach ($name in $resolvedPackNames) {
    $args += $name
}

$args += @(
    "--out-dir", $outDir,
    "--pack-name", $PackName
)

if ($SkipColladaExport) {
    $args += "--skip-collada-export"
}

Write-Host "Merging validation packs '$PackName'"
Write-Host "Parallel root: $ParallelRoot"
Write-Host "Packs: $($resolvedPackNames -join ', ')"
Write-Host "Output dir: $outDir"

& $BlenderExe @args
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "Validation pack merge failed with exit code $exitCode"
}

Write-Host ""
Write-Host "Merged validation pack ready:"
Write-Host "  $outDir"
