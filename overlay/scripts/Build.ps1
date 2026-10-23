param([ValidateSet('Debug','Release')][string]$Configuration='Release')
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$sdk=$env:SIV3D_0_6_16
if(-not $sdk){$sdk=Join-Path (Split-Path $root -Parent) 'tools/OpenSiv3D_SDK_0.6.16'}
$msbuild='C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
if(-not(Test-Path -LiteralPath $sdk)){throw "SDK missing: $sdk"}
$env:SIV3D_0_6_16=$sdk
$id=(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8)
$output=Join-Path $root "build/bin/$id"
New-Item -ItemType Directory -Force -Path $output|Out-Null
$lock=[IO.File]::Open((Join-Path $root 'build/build.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
try {
    & $msbuild (Join-Path $root 'kshootmania.sln') /m:2 "/p:Configuration=$Configuration" /p:Platform=x64 "/p:OutDir=$output/" /verbosity:minimal /nologo 2>&1 | Tee-Object -FilePath (Join-Path $output 'build.log')
    if($LASTEXITCODE -ne 0){throw "MSBuild failed ($LASTEXITCODE); $output/build.log"}
    $exe=Join-Path $output 'ksm-enhanced-vertical.exe'
    if(-not(Test-Path -LiteralPath $exe)){throw 'Build did not produce the mod executable.'}
    $state=[ordered]@{schemaVersion=1;buildId=$id;configuration=$Configuration;executable=$exe;sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash;sourceBase='vertical-asio-sync-skin';baseCommit='41bfdaa7331808aff7239a9ec283cdba01b3b406'}
    $temporary=Join-Path $root "build/latest-$id.tmp"
    $state|ConvertTo-Json|Set-Content -LiteralPath $temporary -Encoding UTF8
    Move-Item -LiteralPath $temporary -Destination (Join-Path $root 'build/latest.json') -Force
    Write-Output "BUILD READY: $exe"
} finally {$lock.Dispose()}