# Creates a local development payload, not a distributable ZIP.
param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'ModCompatibility.psm1') -Force
$root=Split-Path $PSScriptRoot -Parent
$build=Get-Content -LiteralPath (Join-Path $root 'build/latest.json') -Raw|ConvertFrom-Json
if((Get-FileHash -LiteralPath $build.executable -Algorithm SHA256).Hash -ne $build.sha256){throw 'Build changed.'}
$destination=Join-Path $root ('build/payloads/'+$build.buildId+'-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $destination 'payload') -Force|Out-Null
$files=@()
function Add-Payload([string]$Source,[string]$Target,[string]$Policy='replace'){
    Assert-KsmPayloadPath $Target
    $dest=Resolve-KsmSafePath $destination ('payload/'+$Target)
    New-Item -ItemType Directory -Path (Split-Path $dest -Parent) -Force|Out-Null
    Copy-Item -LiteralPath $Source -Destination $dest
    $script:files+=[pscustomobject]@{path=$Target;sha256=(Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash;policy=$Policy}
}
Add-Payload $build.executable 'ksm-enhanced-vertical.exe'
$mods=Join-Path $root 'kshootmania/App/mods/vertical'
foreach($file in Get-ChildItem -LiteralPath $mods -Recurse -File){Add-Payload $file.FullName ('mods/vertical/'+[IO.Path]::GetRelativePath($mods,$file.FullName).Replace('\','/'))}
foreach($name in @('bg_vertical.png','standby_vertical.png')){Add-Payload (Join-Path $root "kshootmania/App/imgs/$name") "imgs/$name"}
foreach($name in @('bg2_vertical.png','msel_bar_vertical.png')){Add-Payload (Join-Path $root "kshootmania/App/imgs/$name") "imgs/$name" 'seed'}
$enhanced=Join-Path $root 'kshootmania/App/mods/enhanced'
foreach($file in Get-ChildItem -LiteralPath $enhanced -Recurse -File){Add-Payload $file.FullName ('mods/enhanced/'+[IO.Path]::GetRelativePath($enhanced,$file.FullName).Replace('\','/'))}
foreach($file in Get-ChildItem -LiteralPath (Join-Path $root 'kshootmania/App/shaders') -Filter 'aps_color_dodge.*' -File){Add-Payload $file.FullName ('shaders/'+$file.Name)}
foreach($name in @('bassasio.dll','bassmix.dll')){Add-Payload (Join-Path $root "kshootmania/App/$name") $name}
foreach($name in @('result_v2_judge.png','result_v2_judge_inversion.png')){Add-Payload (Join-Path $root ('kshootmania/App/imgs/'+$name)) ('imgs/'+$name) 'seed'}
foreach($file in Get-ChildItem -LiteralPath (Join-Path $root 'kshootmania/App/lang') -Filter '*.json'){Add-Payload $file.FullName ('lang/'+$file.Name)}
foreach($file in Get-ChildItem -LiteralPath (Join-Path $root 'kshootmania/App/imgs/default') -Recurse -File){Add-Payload $file.FullName ('imgs/default/'+[IO.Path]::GetRelativePath((Join-Path $root 'kshootmania/App/imgs/default'),$file.FullName).Replace('\','/')) $(if($file.Name -in @('bg_vertical.png','standby_vertical.png')){'replace'}else{'seed'})}
foreach($file in Get-ChildItem -LiteralPath (Join-Path $root 'kshootmania/App/skin_preview') -Recurse -File){Add-Payload $file.FullName ('skin_preview/'+[IO.Path]::GetRelativePath((Join-Path $root 'kshootmania/App/skin_preview'),$file.FullName).Replace('\','/'))}
foreach($name in @('menu_custom_icon.png','menu_judge_icon.png','menu_vertical_icon.png')){Add-Payload (Join-Path $root "kshootmania/App/ui/$name") "ui/$name"}
Add-Payload (Join-Path $root 'LICENSE') 'mods/enhanced/LICENSE'
[ordered]@{schemaVersion=2;modId='ksm-enhanced-vertical';status='development';runtimeContract='ksm-2.1.0-enhanced-skin-v1';supportedProfiles=@('ksm-2.1.0-win-x64');buildId=$build.buildId;files=$files}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $destination 'manifest.json') -Encoding UTF8
Copy-Item -LiteralPath (Join-Path $root 'compatibility/targets.json') -Destination (Join-Path $destination 'targets.json')
Write-Output $destination