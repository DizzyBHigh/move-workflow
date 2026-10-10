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

if ( $env:CI -eq $null ) {
    throw "Package-Windows.ps1 requires CI environment"
}

if ( ! ( [System.Environment]::Is64BitOperatingSystem ) ) {
    throw "Packaging script requires a 64-bit system to build and run."
}

if ( $PSVersionTable.PSVersion -lt '7.2.0' ) {
    Write-Warning 'The packaging script requires PowerShell Core 7. Install or upgrade your PowerShell version: https://aka.ms/pscore6'
    exit 2
}

function Package {
    trap {
        Pop-Location -Stack PackageRoot -ErrorAction 'SilentlyContinue'
        Write-Error $_
        exit 2
    }

    $ScriptHome = $PSScriptRoot
    $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
    $BuildSpecFile = "${ProjectRoot}/buildspec.json"

    $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse

    foreach( $Utility in $UtilityFunctions ) {
        Write-Debug "Loading $($Utility.FullName)"
        . $Utility.FullName
    }

    $BuildSpec = Get-Content -Path ${BuildSpecFile} -Raw | ConvertFrom-Json
    $ProductName = $BuildSpec.name
    $ProductVersion = $BuildSpec.version
    if ( -not [string]::IsNullOrWhiteSpace($Version) ) {
        $ProductVersion = $Version
    }

    $OutputName = "${ProductName}-${ProductVersion}-windows-${Target}"
    $InstallRoot = "${ProjectRoot}/release/${Configuration}"
    $PluginBin = "${InstallRoot}/${ProductName}/bin/64bit"
    $ObsPluginBin = "${InstallRoot}/obs-plugins/64bit"

    if ( ! ( Test-Path -LiteralPath $PluginBin -PathType Container ) ) {
        throw "Expected installed plugin directory was not found: ${PluginBin}"
    }

    Remove-Item -Path "${InstallRoot}/obs-plugins" -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $ObsPluginBin -Force | Out-Null
    Copy-Item -Path "${PluginBin}/*" -Destination $ObsPluginBin -Recurse -Force

    $RemoveArgs = @{
        ErrorAction = 'SilentlyContinue'
        Path = @(
            "${ProjectRoot}/release/${ProductName}-*-windows-*.zip"
        )
    }

    Remove-Item @RemoveArgs

    Log-Group "Archiving ${ProductName}..."
    Push-Location -Stack PackageRoot $InstallRoot
    try {
        $CompressArgs = @{
            Path = 'obs-plugins'
            CompressionLevel = 'Optimal'
            DestinationPath = "${ProjectRoot}/release/${OutputName}.zip"
            Verbose = ($Env:CI -ne $null)
        }
        Compress-Archive -Force @CompressArgs
    }
    finally {
        Pop-Location -Stack PackageRoot
    }
    Log-Group
}

Package