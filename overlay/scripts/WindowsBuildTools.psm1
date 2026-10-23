Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
function Get-KsmMSBuild {
    if($env:KSM_MSBUILD){
        if(-not(Test-Path -LiteralPath $env:KSM_MSBUILD -PathType Leaf)){throw 'KSM_MSBUILD does not name an executable.'}
        return $env:KSM_MSBUILD
    }
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if(Test-Path -LiteralPath $vswhere){
        $found=@(& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild/**/Bin/MSBuild.exe')
        if($LASTEXITCODE -eq 0 -and $found.Count){return $found[0]}
    }
    $fallback=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe'
    if(Test-Path -LiteralPath $fallback){return $fallback}
    throw 'Visual Studio 2022 MSBuild with C++ desktop tools is required.'
}
Export-ModuleMember -Function Get-KsmMSBuild
