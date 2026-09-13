[CmdletBinding()]
param(
    [ValidateSet('module', 'bootstrap', 'layout')]
    [string]$DiagnosticStage = 'module',
    [ValidateSet('modded', 'stock')]
    [string]$DiagnosticResources = 'modded',
    [switch]$SkipDeploy
)

$arguments = @{
    DiagnosticStage = $DiagnosticStage
    DiagnosticResources = $DiagnosticResources
    SkipDeploy = $SkipDeploy
}
& (Join-Path $PSScriptRoot 'scripts\Build-Loose-Patch.ps1') @arguments
