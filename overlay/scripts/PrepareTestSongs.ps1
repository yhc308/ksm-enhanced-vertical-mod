param([Parameter(Mandatory=$true)][string]$SourceSongs)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=[IO.Path]::GetFullPath($SourceSongs)
foreach($required in @('magicalkittens/magicalkittens.ogg','flavorofyouth/flavorofyouth_ch.ksh')){
    if(-not(Test-Path -LiteralPath (Join-Path $source $required) -PathType Leaf)){throw "Not the required original K-Shoot MANIA song collection: $required"}
}
foreach($item in Get-ChildItem -LiteralPath $source -Recurse -Force){
    if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Linked test input rejected: $($item.FullName)"}
}
$destination=Join-Path $root 'tests/fixtures/songs/K-Shoot MANIA'
$files=@(Get-ChildItem -LiteralPath $source -Recurse -File)
# Validate every existing destination before copying anything. Existing fixtures are never overwritten.
foreach($file in $files){
    $target=Join-Path $destination ([IO.Path]::GetRelativePath($source,$file.FullName))
    if((Test-Path -LiteralPath $target) -and ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $target).Hash)){
        throw "Conflicting test input retained: $target. Use a fresh source worktree."
    }
}
foreach($file in $files){
    $target=Join-Path $destination ([IO.Path]::GetRelativePath($source,$file.FullName))
    if(Test-Path -LiteralPath $target){continue}
    New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force|Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $target
}
Write-Output "Local test inputs prepared: $destination"
Write-Output 'Original songs and user runtimes were not modified. Existing fixtures were retained. This snapshot is ignored by Git.'
