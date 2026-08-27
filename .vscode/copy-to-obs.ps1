$ErrorActionPreference = 'Stop'

$dll = Join-Path $PSScriptRoot '..\build_x64\RelWithDebInfo\move-workflow.dll'
$pdb = Join-Path $PSScriptRoot '..\build_x64\RelWithDebInfo\move-workflow.pdb'
$dest = 'C:\PORTABLE APPS\OBS Studio - RTS\obs-plugins\64bit'

$dll = [System.IO.Path]::GetFullPath($dll)
$pdb = [System.IO.Path]::GetFullPath($pdb)

if (!(Test-Path -LiteralPath $dll)) {
    throw "Build output DLL was not found: $dll"
}

if (!(Test-Path -LiteralPath $dest)) {
    throw "OBS plugin directory was not found: $dest"
}

$legacyDll = Join-Path $dest 'plugintemplate-for-obs.dll'
$legacyPdb = Join-Path $dest 'plugintemplate-for-obs.pdb'

foreach ($legacy in @($legacyDll, $legacyPdb)) {
    if (Test-Path -LiteralPath $legacy) {
        Remove-Item -LiteralPath $legacy -Force
        Write-Host "Removed legacy plugin: $legacy"
    }
}

Copy-Item -LiteralPath $dll -Destination $dest -Force

if (Test-Path -LiteralPath $pdb) {
    Copy-Item -LiteralPath $pdb -Destination $dest -Force
}

Write-Host "Move Workflow copied to OBS:"
Write-Host "  $dest\move-workflow.dll"
if (Test-Path -LiteralPath $pdb) {
    Write-Host "  $dest\move-workflow.pdb"
}