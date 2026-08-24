$ErrorActionPreference = 'Stop'

$cmake = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$files = @('CMakeLists.txt', 'buildspec.json')
$files += Get-ChildItem 'cmake/common' -Recurse -File | ForEach-Object {
    $_.FullName.Substring($root.Length + 1)
}
$hash = (Get-FileHash $files -Algorithm SHA256 | ForEach-Object Hash) -join ''
$stamp = Join-Path $root 'build_x64\.move-workflow-cmake-identity'
$old = if (Test-Path $stamp) { Get-Content $stamp -Raw } else { '' }

if ($hash -ne $old) {
    Write-Host 'Project identity changed - refreshing CMake configuration'
    & $cmake -S . -B build_x64 -DENABLE_QT=ON
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Set-Content $stamp $hash
} else {
    Write-Host 'Project identity unchanged - using existing CMake configuration'
}

& $cmake --build build_x64 --config RelWithDebInfo
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'copy-to-obs.ps1')
exit $LASTEXITCODE
