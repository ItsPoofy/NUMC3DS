[CmdletBinding()]
param(
    [string]$BuildRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) 'build\numc3ds'),
    [string]$TargetRoot = (Join-Path $env:APPDATA 'Azahar\load\mods\00040000001B8700')
)

$ErrorActionPreference = 'Stop'
$BuildRoot = [IO.Path]::GetFullPath($BuildRoot)
$TargetRoot = [IO.Path]::GetFullPath($TargetRoot)
$ManifestPath = Join-Path $BuildRoot 'numc3ds-release-manifest.json'
if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) { throw "Missing release manifest: $ManifestPath" }
$Manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
if ($Manifest.patch_owner -ne 'Luma LayeredFS and code patching only' -or $Manifest.target_title_id -ne '00040000001B8700') { throw 'The manifest is not a Luma-only USA release.' }
foreach ($File in $Manifest.files) {
    $Source = Join-Path $BuildRoot ($File.path.Replace('/', '\'))
    if (-not (Test-Path -LiteralPath $Source -PathType Leaf) -or (Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash -ne $File.sha256) { throw "Manifest verification failed: $($File.path)" }
}
$Parent = Split-Path -Parent $TargetRoot
$Stage = Join-Path $Parent ('.00040000001B8700.numc3ds-stage-' + $PID)
$Backup = Join-Path $Parent ('.00040000001B8700.numc3ds-backup-' + $PID)
if (Test-Path -LiteralPath $Stage) { throw "Staging path already exists: $Stage" }
New-Item -ItemType Directory -Path $Stage | Out-Null
try {
    foreach ($File in $Manifest.files) {
        $Relative = $File.path.Replace('/', '\')
        $Destination = Join-Path $Stage $Relative
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null
        Copy-Item -LiteralPath (Join-Path $BuildRoot $Relative) -Destination $Destination -Force
    }
    if (Test-Path -LiteralPath $TargetRoot) { Move-Item -LiteralPath $TargetRoot -Destination $Backup }
    Move-Item -LiteralPath $Stage -Destination $TargetRoot
    if (Test-Path -LiteralPath $Backup) { Remove-Item -LiteralPath $Backup -Recurse -Force }
    Write-Host "Deployed clean Luma-only release: $TargetRoot"
} catch {
    if ((Test-Path -LiteralPath $Backup) -and -not (Test-Path -LiteralPath $TargetRoot)) { Move-Item -LiteralPath $Backup -Destination $TargetRoot }
    throw
} finally {
    if (Test-Path -LiteralPath $Stage) { Remove-Item -LiteralPath $Stage -Recurse -Force }
}
