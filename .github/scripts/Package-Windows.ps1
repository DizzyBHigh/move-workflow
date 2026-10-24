[CmdletBinding()]
param(
    [ValidateSet('x64')]
    [string] $Target = 'x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo',
    [string] $Version = ''
)

$ErrorActionPreference = 'Stop'

if ( $DebugPreference -eq 'Continue' ) {
    $VerbosePreference = 'Continue'
    $InformationPreference = 'Continue'
}
if ( $env:CI -eq $null ) { throw "Package-Windows.ps1 requires CI environment" }
if ( ! [System.Environment]::Is64BitOperatingSystem ) {
    throw "Packaging script requires a 64-bit system to build and run."
}
if ( $PSVersionTable.PSVersion -lt '7.2.0' ) {
    Write-Warning 'The packaging script requires PowerShell Core 7: https://aka.ms/pscore6'
    exit 2
}

function Package {
    $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
    $BuildSpec = Get-Content -Path "$ProjectRoot/buildspec.json" -Raw | ConvertFrom-Json
    $ProductName = $BuildSpec.name
    $ProductVersion = $BuildSpec.version
    if ( -not [string]::IsNullOrWhiteSpace($Version) ) { $ProductVersion = $Version }

    $OutputName = "${ProductName}-${ProductVersion}-windows-${Target}"
    $InstallRoot = "$ProjectRoot/release/$Configuration"
    $PluginBin = "$InstallRoot/$ProductName/bin/64bit"
    $StageRoot = "$ProjectRoot/release/package-staging"
    $LegacyBin = "$StageRoot/OBS30-32/obs-plugins/64bit"
    $NativeBin = "$StageRoot/OBS33/plugins/$ProductName"

    if ( ! (Test-Path -LiteralPath $PluginBin -PathType Container) ) {
        throw "Expected installed plugin directory was not found: $PluginBin"
    }

    Remove-Item -LiteralPath $StageRoot -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $LegacyBin -Force | Out-Null
    New-Item -ItemType Directory -Path $NativeBin -Force | Out-Null
    Copy-Item -Path "$PluginBin/*" -Destination $LegacyBin -Recurse -Force
    Copy-Item -Path "$PluginBin/*" -Destination $NativeBin -Recurse -Force

    $LegacyDll = Join-Path $LegacyBin "$ProductName.dll"
    $NativeDll = Join-Path $NativeBin "$ProductName.dll"
    if ( !(Test-Path -LiteralPath $LegacyDll -PathType Leaf) -or
         !(Test-Path -LiteralPath $NativeDll -PathType Leaf) ) {
        throw "Both OBS layout folders must contain $ProductName.dll."
    }

    Remove-Item -Path "$ProjectRoot/release/$ProductName-*-windows-*.zip" -Force -ErrorAction SilentlyContinue
    $Archive = "$ProjectRoot/release/$OutputName.zip"
    Remove-Item -LiteralPath $Archive -Force -ErrorAction SilentlyContinue
    Write-Host "Creating dual-layout archive: $Archive"
    Compress-Archive -Path "$StageRoot/OBS30-32", "$StageRoot/OBS33" -CompressionLevel Optimal -DestinationPath $Archive -Force
    Remove-Item -LiteralPath $StageRoot -Recurse -Force
}

Package
