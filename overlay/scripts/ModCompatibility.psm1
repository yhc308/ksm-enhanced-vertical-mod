Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
function Resolve-KsmSafePath {
    param([string]$Root,[string]$Relative)
    if([string]::IsNullOrWhiteSpace($Relative) -or [IO.Path]::IsPathRooted($Relative) -or $Relative.Contains(':') -or $Relative -match '(^|[\\/])\.{1,2}([\\/]|$)'){throw "Unsafe relative path: $Relative"}
    $base=[IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    $path=[IO.Path]::GetFullPath((Join-Path $base $Relative))
    if(-not $path.StartsWith($base+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Path escapes root.'}
    $scan=$path
    while($scan.Length -ge $base.Length){
        if(Test-Path -LiteralPath $scan){if((Get-Item -LiteralPath $scan -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Linked path rejected: $scan"}}
        if($scan -eq $base){break};$scan=Split-Path $scan -Parent
    }
    return $path
}
function Test-KsmModTarget {
    param([string]$GameDirectory,[string]$RegistryPath)
    $root=[IO.Path]::GetFullPath($GameDirectory)
    $exe=Resolve-KsmSafePath $root 'kshootmania.exe'
    if(-not(Test-Path -LiteralPath $exe -PathType Leaf)){throw 'Original kshootmania.exe missing.'}
    $registry=Get-Content -LiteralPath $RegistryPath -Raw|ConvertFrom-Json
    if($registry.schemaVersion -ne 2 -or $registry.modId -ne 'ksm-enhanced-vertical'){throw 'Unsupported target registry.'}
    $hash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    $matches=@($registry.profiles|Where-Object {$hash -in @($_.executableHashes) -and $_.status -in @('baseline','verified')})
    $issues=@();$profile=$null
    if($matches.Count -eq 1){
        $profile=$matches[0]
        foreach($file in $profile.requiredFiles){
            $path=Resolve-KsmSafePath $root $file.path
            if(-not(Test-Path -LiteralPath $path -PathType Leaf)){$issues+="Missing: $($file.path)"}
            elseif((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $file.sha256){$issues+="Resource contract mismatch: $($file.path)"}
        }
    }elseif($matches.Count -gt 1){$issues+='Ambiguous compatibility profiles.'}else{$issues+='Unknown upstream version/hash: add and validate a target profile before installation.'}
    [pscustomobject]@{supported=($null -ne $profile -and $issues.Count -eq 0);profileId=if($profile){$profile.id}else{$null};runtimeContract=if($profile){$profile.runtimeContract}else{$null};upstreamVersion=if($profile){$profile.upstreamVersion}else{$null};executableHash=$hash;issues=@($issues)}
}
function Assert-KsmPayloadPath {
    param([string]$Path)
    if($Path -match '^shaders/aps_color_dodge\.(hlsl|frag|essl\.frag)$' -or $Path -match '^(imgs/default|skin_preview)/[a-zA-Z0-9_./-]+$' -or $Path -in @('ui/menu_custom_icon.png','ui/menu_judge_icon.png','ui/menu_vertical_icon.png','ui/viewport_info_icon.png') -or $Path -eq 'ksm-enhanced-vertical.exe' -or $Path -match '^mods/(vertical|enhanced)/[a-zA-Z0-9_./-]+$' -or $Path -in @('imgs/bg_vertical.png','imgs/standby_vertical.png','imgs/bg2_vertical.png','imgs/msel_bar_vertical.png','imgs/result_v2_judge.png','imgs/result_v2_judge_inversion.png','bassasio.dll','bassmix.dll','lang/English.json','lang/Japanese.json','lang/Korean.json','lang/Simplified Chinese.json','lang/Traditional Chinese.json')){return}
    throw "Disallowed mod payload: $Path"
}
Export-ModuleMember -Function Resolve-KsmSafePath,Test-KsmModTarget,Assert-KsmPayloadPath