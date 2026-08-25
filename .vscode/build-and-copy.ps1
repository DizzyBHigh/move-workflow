$ErrorActionPreference = 'Stop'

$cmake = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# Always re-run configure so the generated Visual Studio project cannot
# retain an obsolete target source list after branch/source changes.
Write-Host 'Refreshing CMake configuration'
& $cmake -S . -B build_x64 -DENABLE_QT=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $cmake --build build_x64 --config RelWithDebInfo
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'copy-to-obs.ps1')
exit $LASTEXITCODE
