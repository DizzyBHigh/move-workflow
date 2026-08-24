$ErrorActionPreference = 'Stop'

$dll = Join-Path $PSScriptRoot '..\build_x64\RelWithDebInfo\obs-move-workflow.dll'
$pdb = Join-Path $PSScriptRoot '..\build_x64\RelWithDebInfo\obs-move-workflow.pdb'
$dest = 'C:\PORTABLE APPS\OBS Studio - RTS\obs-plugins\64bit'

$dll = [System.IO.Path]::GetFullPath($dll)
$pdb = [System.IO.Path]::GetFullPath($pdb)

if (!(Test-Path -LiteralPath $dll)) {
    throw "Build output DLL was not found: $dll"
}

if (!(Test-Path -LiteralPath $dest)) {
    throw "OBS plugin directory was not found: $dest"
}

Copy-Item -LiteralPath $dll -Destination $dest -Force

if (Test-Path -LiteralPath $pdb) {
    Copy-Item -LiteralPath $pdb -Destination $dest -Force
}

Write-Host "Move Workflow copied to OBS:"
Write-Host "  $dest\obs-move-workflow.dll"
if (Test-Path -LiteralPath $pdb) {
    Write-Host "  $dest\obs-move-workflow.pdb"
}
