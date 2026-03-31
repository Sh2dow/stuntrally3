[CmdletBinding(DefaultParameterSetName = "Source")]
param(
    [Parameter(ParameterSetName = "Source")]
    [ValidateSet("Bridge", "Unified")]
    [string]$Source = "Unified",

    [Parameter(ParameterSetName = "Explicit", Mandatory = $true)]
    [string[]]$SectionNumbers,

    [string]$BasePackName = "sections_parallel",
    [string]$UnifiedRoot = "f:\NFS\NFSC_Mods\W2C\Blender",
    [string]$BlenderExe = "D:\Development\Blender K-Cycles\blender.exe",
    [string]$OutputRoot,
    [int]$ChunkSize = 50,
    [int]$MaxParallel = 4,
    [int]$First = 0,
    [switch]$SkipAssetDumper,
    [switch]$SkipModelulator,
    [switch]$HideAssetDumperReference = $true,
    [switch]$KeepNonLocalModelulator,
    [switch]$SkipColladaExport
)

function Resolve-SectionNumbers {
    param(
        [string[]]$Values,
        [string]$Mode,
        [string]$UnifiedRootPath
    )

    if ($Values -and $Values.Count -gt 0) {
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

    switch ($Mode) {
        "Bridge" {
            $bridgeCsv = Join-Path $UnifiedRootPath "section_solid_bridge.csv"
            if (-not (Test-Path $bridgeCsv)) {
                throw "Missing bridge CSV: $bridgeCsv"
            }
            return @((Import-Csv $bridgeCsv).section_number | ForEach-Object { [int]$_ } | Sort-Object -Unique)
        }
        "Unified" {
            $manifestPath = Join-Path $UnifiedRootPath "unified_manifest.json"
            if (-not (Test-Path $manifestPath)) {
                throw "Missing unified manifest: $manifestPath"
            }
            $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
            return @($manifest.sections.section_number | ForEach-Object { [int]$_ } | Sort-Object -Unique)
        }
    }

    throw "Unsupported section resolution mode: $Mode"
}

function Split-IntoChunks {
    param(
        [int[]]$Numbers,
        [int]$Size
    )

    $chunks = @()
    for ($i = 0; $i -lt $Numbers.Count; $i += $Size) {
        $end = [Math]::Min($i + $Size - 1, $Numbers.Count - 1)
        $slice = @($Numbers[$i..$end])
        $chunks += [pscustomobject]@{
            Sections = $slice
        }
    }
    return ,$chunks
}

function New-ChunkPackName {
    param(
        [string]$Prefix,
        [int[]]$Chunk,
        [int]$ChunkIndex
    )

    return "{0}_{1:D4}_{2}_{3}_{4}count" -f $Prefix, $ChunkIndex, $Chunk[0], $Chunk[-1], $Chunk.Count
}

$singleRunner = Join-Path $PSScriptRoot "RunCarbonValidationPack.ps1"
if (-not (Test-Path $singleRunner)) {
    throw "Missing single-pack runner: $singleRunner"
}

if (-not (Test-Path $BlenderExe)) {
    throw "Missing Blender executable: $BlenderExe"
}

if ($ChunkSize -lt 1) {
    throw "ChunkSize must be at least 1"
}

if ($MaxParallel -lt 1) {
    throw "MaxParallel must be at least 1"
}

if (-not $OutputRoot) {
    $OutputRoot = Join-Path $UnifiedRoot "validation_packs_parallel"
}

$resolvedSections = Resolve-SectionNumbers -Values $SectionNumbers -Mode $Source -UnifiedRootPath $UnifiedRoot
if ($First -gt 0) {
    $resolvedSections = @($resolvedSections | Select-Object -First $First)
}

if (-not $resolvedSections -or $resolvedSections.Count -eq 0) {
    throw "No section numbers were resolved"
}

$chunks = Split-IntoChunks -Numbers $resolvedSections -Size $ChunkSize
$runRoot = Join-Path $OutputRoot $BasePackName
$logsRoot = Join-Path $runRoot "logs"
New-Item -ItemType Directory -Force -Path $logsRoot | Out-Null

Write-Host "Parallel validation pack build"
if ($PSCmdlet.ParameterSetName -eq "Explicit") {
    Write-Host "  Source: Explicit"
} else {
    Write-Host "  Source: $Source"
}
Write-Host "  Sections: $($resolvedSections.Count)"
Write-Host "  Chunk size: $ChunkSize"
Write-Host "  Workers: $MaxParallel"
Write-Host "  Run root: $runRoot"

$running = New-Object System.Collections.Generic.List[object]
$results = New-Object System.Collections.Generic.List[object]

for ($index = 0; $index -lt $chunks.Count; $index++) {
    $chunk = [int[]]$chunks[$index].Sections
    $chunkNumber = $index + 1
    $packName = New-ChunkPackName -Prefix $BasePackName -Chunk $chunk -ChunkIndex $chunkNumber
    $logPath = Join-Path $logsRoot ("{0}.log" -f $packName)
    $sectionsArg = ($chunk | ForEach-Object { "$_" }) -join ','

    $processArgs = @(
        "-ExecutionPolicy", "Bypass",
        "-File", $singleRunner,
        "-SectionNumbers", $sectionsArg,
        "-PackName", $packName,
        "-UnifiedRoot", $UnifiedRoot,
        "-BlenderExe", $BlenderExe,
        "-OutputRoot", $runRoot
    )

    if ($SkipAssetDumper) {
        $processArgs += "-SkipAssetDumper"
    }
    if ($SkipModelulator) {
        $processArgs += "-SkipModelulator"
    }
    if ($HideAssetDumperReference) {
        $processArgs += "-HideAssetDumperReference"
    }
    if ($KeepNonLocalModelulator) {
        $processArgs += "-KeepNonLocalModelulator"
    }
    if ($SkipColladaExport) {
        $processArgs += "-SkipColladaExport"
    }

    while ($running.Count -ge $MaxParallel) {
        $completed = Wait-Job -Job ($running | ForEach-Object { $_.Job }) -Any -Timeout 5
        if ($null -eq $completed) {
            continue
        }

        foreach ($item in $running.ToArray()) {
            if ($item.Job.State -in @("Completed", "Failed", "Stopped")) {
                $jobOutput = Receive-Job -Job $item.Job -ErrorAction SilentlyContinue
                $exitCode = if ($jobOutput -and $jobOutput.ExitCode -ne $null) { [int]$jobOutput.ExitCode } else { 1 }
                $results.Add([pscustomobject]@{
                    PackName = $item.PackName
                    Sections = $item.Sections
                    LogPath = $item.LogPath
                    ExitCode = $exitCode
                })
                Remove-Job -Job $item.Job -Force | Out-Null
                $running.Remove($item) | Out-Null
            }
        }
    }

    Write-Host ("Starting chunk {0}/{1}: {2} ({3}-{4}, {5} sections)" -f $chunkNumber, $chunks.Count, $packName, $chunk[0], $chunk[-1], $chunk.Count)
    $job = Start-Job -ScriptBlock {
        param($RunnerPath, $ArgsList, $OutLog, $Pack)

        & powershell @ArgsList *>> $OutLog
        [pscustomobject]@{
            PackName = $Pack
            ExitCode = $LASTEXITCODE
        }
    } -ArgumentList $singleRunner, $processArgs, $logPath, $packName

    $running.Add([pscustomobject]@{
        PackName = $packName
        Sections = $chunk
        LogPath = $logPath
        Job = $job
    }) | Out-Null
}

while ($running.Count -gt 0) {
    $completed = Wait-Job -Job ($running | ForEach-Object { $_.Job }) -Any -Timeout 5
    if ($null -eq $completed) {
        continue
    }

    foreach ($item in $running.ToArray()) {
        if ($item.Job.State -in @("Completed", "Failed", "Stopped")) {
            $jobOutput = Receive-Job -Job $item.Job -ErrorAction SilentlyContinue
            $exitCode = if ($jobOutput -and $jobOutput.ExitCode -ne $null) { [int]$jobOutput.ExitCode } else { 1 }
            $results.Add([pscustomobject]@{
                PackName = $item.PackName
                Sections = $item.Sections
                LogPath = $item.LogPath
                ExitCode = $exitCode
            })
            Remove-Job -Job $item.Job -Force | Out-Null
            $running.Remove($item) | Out-Null
        }
    }
}

$orderedResults = @($results | Sort-Object PackName)
$failed = @($orderedResults | Where-Object { $_.ExitCode -ne 0 })

$summary = [pscustomobject]@{
    base_pack_name = $BasePackName
    unified_root = $UnifiedRoot
    output_root = $runRoot
    section_count = $resolvedSections.Count
    chunk_size = $ChunkSize
    max_parallel = $MaxParallel
    chunk_count = $chunks.Count
    failures = $failed.Count
    packs = @(
        $orderedResults | ForEach-Object {
            [pscustomobject]@{
                pack_name = $_.PackName
                first_section = $_.Sections[0]
                last_section = $_.Sections[-1]
                section_count = $_.Sections.Count
                exit_code = $_.ExitCode
                log_path = $_.LogPath
                output_dir = (Join-Path $runRoot $_.PackName)
            }
        }
    )
}

$summaryPath = Join-Path $runRoot "parallel_manifest.json"
$summary | ConvertTo-Json -Depth 6 | Set-Content -Path $summaryPath -Encoding utf8

Write-Host ""
Write-Host "Parallel build complete:"
Write-Host "  Summary: $summaryPath"
Write-Host "  Chunks: $($chunks.Count)"
Write-Host "  Failures: $($failed.Count)"

if ($failed.Count -gt 0) {
    foreach ($item in $failed) {
        Write-Host ("  FAILED {0} (exit {1}) log={2}" -f $item.PackName, $item.ExitCode, $item.LogPath)
    }
    throw "One or more parallel pack builds failed"
}
