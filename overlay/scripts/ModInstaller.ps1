param(
    [Parameter(Mandatory)][ValidateSet('Inspect','Install','Verify','Remove')][string]$Action,
    [Parameter(Mandatory)][string]$GameDirectory,
    [string]$PackageDirectory,
    [string]$RegistryPath,
    [switch]$AllowDevelopment
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'ModCompatibility.psm1') -Force
$root=[IO.Path]::GetFullPath($GameDirectory).TrimEnd('\','/')
if(-not(Test-Path -LiteralPath $root -PathType Container)){throw 'Game directory missing.'}
if(-not $RegistryPath){$RegistryPath=Join-Path (Split-Path $PSScriptRoot -Parent) 'compatibility/targets.json'}
if($Action -eq 'Inspect'){Test-KsmModTarget $root $RegistryPath;return}
$statePath=Resolve-KsmSafePath $root '.ksm-enhanced-install.json'
function Hash([string]$Path){(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash}
function Assert-Writable([string]$Path){
    if(Test-Path -LiteralPath $Path){$handle=[IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None);$handle.Dispose()}
}
function Copy-Checked([string]$Source,[string]$Destination,[string]$Expected){
    New-Item -ItemType Directory -Path (Split-Path $Destination -Parent) -Force|Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
    if((Hash $Destination) -ne $Expected){throw "Copy hash mismatch: $Destination"}
}
$lockPath=Resolve-KsmSafePath $root '.ksm-enhanced-install.lock'
$lock=[IO.File]::Open($lockPath,[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
try {
    if($Action -eq 'Install'){
        if(Test-Path -LiteralPath $statePath){throw 'Mod already installed. Remove with preservation checks before installing another build.'}
        if(-not $PackageDirectory){throw 'PackageDirectory required.'}
        $package=[IO.Path]::GetFullPath($PackageDirectory)
        $manifest=Get-Content -LiteralPath (Resolve-KsmSafePath $package 'manifest.json') -Raw|ConvertFrom-Json
        if($manifest.schemaVersion -ne 2 -or $manifest.modId -ne 'ksm-enhanced-vertical'){throw 'Unsupported mod manifest.'}
        if($manifest.status -ne 'release' -and -not $AllowDevelopment){throw 'Development payload requires AllowDevelopment in a disposable fixture.'}
        $target=Test-KsmModTarget $root (Resolve-KsmSafePath $package 'targets.json')
        if(-not $target.supported){throw ($target.issues -join '; ')}
        if($target.profileId -notin @($manifest.supportedProfiles) -or $target.runtimeContract -ne $manifest.runtimeContract){throw 'Payload does not implement this upstream runtime contract.'}
        $seen=@{};$entries=@();$transaction=[guid]::NewGuid().ToString('N')
        $backupRoot='.ksm-enhanced-backup/'+$transaction
        foreach($file in $manifest.files){
            $relative=[string]$file.path
            Assert-KsmPayloadPath $relative
            if($seen.ContainsKey($relative.ToLowerInvariant())){throw 'Duplicate payload path.'};$seen[$relative.ToLowerInvariant()]=$true
            if($file.policy -notin @('replace','seed')){throw 'Unknown file policy.'}
            $source=Resolve-KsmSafePath $package ('payload/'+$relative)
            if((Hash $source) -ne $file.sha256){throw "Invalid payload: $relative"}
            $destination=Resolve-KsmSafePath $root $relative
            $exists=Test-Path -LiteralPath $destination -PathType Leaf
            $owned=-not($exists -and $file.policy -eq 'seed')
            if($owned){Assert-Writable $destination}
            $entries+=[pscustomobject]@{path=$relative;sha256=$file.sha256;owned=$owned;retainOnRemove=($file.policy -eq 'seed');originalSha256=if($exists){Hash $destination}else{$null};backup=if($exists -and $owned){$backupRoot+'/'+$entries.Count+'.bin'}else{$null};stage=$backupRoot+'/stage-'+$entries.Count+'.bin'}
        }
        if(-not $seen.ContainsKey('ksm-enhanced-vertical.exe')){throw 'Mod executable missing.'}
        # Complete all backups and staging before replacing a single file.
        foreach($entry in $entries){
            if(-not $entry.owned){continue}
            if($entry.backup){Copy-Checked (Resolve-KsmSafePath $root $entry.path) (Resolve-KsmSafePath $root $entry.backup) $entry.originalSha256}
            Copy-Checked (Resolve-KsmSafePath $package ('payload/'+$entry.path)) (Resolve-KsmSafePath $root $entry.stage) $entry.sha256
        }
        $applied=@()
        try {
            foreach($entry in $entries){
                if(-not $entry.owned){continue}
                $applied+=$entry
                Copy-Checked (Resolve-KsmSafePath $root $entry.stage) (Resolve-KsmSafePath $root $entry.path) $entry.sha256
            }
            [ordered]@{schemaVersion=2;modId='ksm-enhanced-vertical';profileId=$target.profileId;upstreamHash=$target.executableHash;runtimeContract=$manifest.runtimeContract;buildId=$manifest.buildId;transaction=$transaction;files=$entries}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Resolve-KsmSafePath $root "$backupRoot/state.json") -Encoding UTF8
            Copy-Item -LiteralPath (Resolve-KsmSafePath $root "$backupRoot/state.json") -Destination $statePath
        } catch {
            [array]::Reverse($applied)
            foreach($entry in $applied){$path=Resolve-KsmSafePath $root $entry.path;if($entry.backup){Copy-Checked (Resolve-KsmSafePath $root $entry.backup) $path $entry.originalSha256}elseif(Test-Path -LiteralPath $path){Remove-Item -LiteralPath $path -Force}}
            throw
        }
        Write-Output 'Installed side-by-side mod executable. Original kshootmania.exe/config/songs/score preserved.'
    }else{
        $state=Get-Content -LiteralPath $statePath -Raw|ConvertFrom-Json
        if($state.schemaVersion -ne 2 -or $state.modId -ne 'ksm-enhanced-vertical' -or $state.transaction -notmatch '^[a-f0-9]{32}$'){throw 'Invalid installation state.'}
        $seen=@{}
        foreach($entry in $state.files){
            Assert-KsmPayloadPath $entry.path
            if($seen.ContainsKey($entry.path.ToLowerInvariant())){throw 'Duplicate installed path.'};$seen[$entry.path.ToLowerInvariant()]=$true
            $path=Resolve-KsmSafePath $root $entry.path
            if(-not $entry.owned -or ($entry.PSObject.Properties['retainOnRemove'] -and $entry.retainOnRemove)){continue}
            if(-not(Test-Path -LiteralPath $path) -or (Hash $path) -ne $entry.sha256){throw "Modified mod file preserved; resolve before removal: $($entry.path)"}
            if($entry.backup){
                if($entry.backup -notmatch ('^\.ksm-enhanced-backup/'+$state.transaction+'/[0-9]+\.bin$')){throw 'Invalid backup location.'}
                if((Hash (Resolve-KsmSafePath $root $entry.backup)) -ne $entry.originalSha256){throw 'Backup hash mismatch.'}
            }
            if($Action -eq 'Remove'){Assert-Writable $path}
        }
        if($Action -eq 'Verify'){
            if((Hash (Resolve-KsmSafePath $root 'kshootmania.exe')) -ne $state.upstreamHash){throw 'Upstream executable updated. Mod remains removable; qualify a new version profile before using it.'}
            Write-Output 'Mod files, original backups and upstream version verified.'
        }else{
            $undoRoot='.ksm-enhanced-backup/'+$state.transaction+'/remove-'+[guid]::NewGuid().ToString('N')
            $owned=@($state.files|Where-Object {$_.owned -and -not ($_.PSObject.Properties['retainOnRemove'] -and $_.retainOnRemove)});$undo=@();$applied=@()
            for($i=0;$i -lt $owned.Count;$i++){$stage=Resolve-KsmSafePath $root "$undoRoot/$i.bin";Copy-Checked (Resolve-KsmSafePath $root $owned[$i].path) $stage $owned[$i].sha256;$undo+=@{entry=$owned[$i];stage=$stage}}
            try {
                foreach($item in $undo){$applied+=$item;$entry=$item.entry;$path=Resolve-KsmSafePath $root $entry.path;if($entry.backup){Copy-Checked (Resolve-KsmSafePath $root $entry.backup) $path $entry.originalSha256}else{Remove-Item -LiteralPath $path -Force}}
                Remove-Item -LiteralPath $statePath -Force
            } catch {foreach($item in $applied){Copy-Checked $item.stage (Resolve-KsmSafePath $root $item.entry.path) $item.entry.sha256};throw}
            Write-Output 'Mod removed. Original files restored; seeded skins, user data and recovery backups retained.'
        }
    }
} finally {$lock.Dispose()}