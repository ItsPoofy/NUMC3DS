[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$InputCode,
    [Parameter(Mandatory = $true)]
    [byte[]]$PatchedCodeBytes,
    [Parameter(Mandatory = $true)]
    [string]$OutputIps
)

$ErrorActionPreference = 'Stop'
$Original = [IO.File]::ReadAllBytes($InputCode)
$Patched = $PatchedCodeBytes
if ($Original.Length -ne $Patched.Length) {
    throw 'IPS generation requires equal-size source and patched code binaries.'
}

$Output = [Collections.Generic.List[byte]]::new()
$Output.AddRange([Text.Encoding]::ASCII.GetBytes('PATCH'))
$Records = 0
$Index = 0
while ($Index -lt $Patched.Length) {
    if ($Original[$Index] -eq $Patched[$Index]) {
        $Index++
        continue
    }

    $Offset = $Index
    $Length = 0
    while ($Index -lt $Patched.Length -and $Original[$Index] -ne $Patched[$Index] -and $Length -lt 0xFFFF) {
        $Index++
        $Length++
    }
    if ($Offset -gt 0xFFFFFF) {
        throw ('IPS cannot encode code.bin offset 0x{0:X8}.' -f $Offset)
    }
    $Output.Add([byte](($Offset -shr 16) -band 0xFF))
    $Output.Add([byte](($Offset -shr 8) -band 0xFF))
    $Output.Add([byte]($Offset -band 0xFF))
    $Output.Add([byte](($Length -shr 8) -band 0xFF))
    $Output.Add([byte]($Length -band 0xFF))
    for ($ByteIndex = 0; $ByteIndex -lt $Length; $ByteIndex++) {
        $Output.Add($Patched[$Offset + $ByteIndex])
    }
    $Records++
}
$Output.AddRange([Text.Encoding]::ASCII.GetBytes('EOF'))
[IO.File]::WriteAllBytes($OutputIps, $Output.ToArray())
Write-Host ('Wrote code.ips with {0} records ({1} bytes).' -f $Records, $Output.Count)
