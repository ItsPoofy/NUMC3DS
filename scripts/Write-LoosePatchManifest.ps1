[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildRoot,
    [Parameter(Mandatory = $true)]
    [string]$InputCode
)

$ErrorActionPreference = 'Stop'
$ReleaseRoot = [IO.Path]::GetFullPath($BuildRoot)
$ManifestPath = Join-Path $ReleaseRoot 'numc3ds-release-manifest.json'
if (-not (Test-Path -LiteralPath (Join-Path $ReleaseRoot 'code.ips') -PathType Leaf)) {
    throw "Missing release code.ips: $ReleaseRoot"
}
$Files = Get-ChildItem -LiteralPath $ReleaseRoot -Recurse -File |
    Where-Object { $_.FullName -ne $ManifestPath -and $_.FullName -notlike (Join-Path $ReleaseRoot 'objects\*') } |
    Sort-Object FullName |
    ForEach-Object {
        $Relative = $_.FullName.Substring($ReleaseRoot.Length).TrimStart('\').Replace('\', '/')
        [pscustomobject]@{
            path = $Relative
            size = [int64]$_.Length
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    }
$Manifest = [ordered]@{
    schema = 1
    patch_owner = 'Luma LayeredFS and code patching only'
    target_title_id = '00040000001B8700'
    update_title_id = '0004000E001B8700'
    input_code_sha256 = (Get-FileHash -LiteralPath $InputCode -Algorithm SHA256).Hash
    files = @($Files)
}
[IO.File]::WriteAllText($ManifestPath, ($Manifest | ConvertTo-Json -Depth 4), [Text.UTF8Encoding]::new($false))
Write-Host ("Wrote release manifest with {0} files: {1}" -f $Files.Count, $ManifestPath)
