[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$TrackId,
    [string]$TrackRoot,
[switch]$NoBackup
)

$scriptPath = Join-Path $PSScriptRoot "ApplyCarbonObjectScaffold.py"
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path $scriptPath)) {
    throw "Missing scaffold apply script: $scriptPath"
}

if (-not $TrackRoot) {
    $TrackRoot = Join-Path (Join-Path $repoRoot "data\tracks") $TrackId
}

$sceneXml = Join-Path $TrackRoot "scene.xml"
$objectsXml = Join-Path $TrackRoot "objects_scaffold\scene_objects_scaffold.xml"

if (-not (Test-Path $sceneXml)) {
    throw "Missing track scene.xml: $sceneXml"
}
if (-not (Test-Path $objectsXml)) {
    throw "Missing scaffold xml: $objectsXml"
}

$args = @(
    $scriptPath,
    "--scene-xml", $sceneXml,
    "--objects-xml", $objectsXml
)
if ($NoBackup) {
    $args += "--no-backup"
}

python @args
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    throw "Applying Carbon object scaffold failed with exit code $exitCode"
}
