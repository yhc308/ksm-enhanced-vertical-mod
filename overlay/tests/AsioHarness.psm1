Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Import-Module (Join-Path (Split-Path $PSScriptRoot -Parent) 'scripts/TestWorkspace.psm1') -Force
function Set-AsioFixtureConfig {
 param($Session,[hashtable]$Values=@{})
 $text="[config]`r`nmastervol=0`r`ncurrentlang=Korean`r`naudio_backend=default`r`nvertical_enabled=0`r`n"
 $text+=(@($Values.GetEnumerator()|ForEach-Object {'{0}={1}' -f $_.Key,$_.Value}) -join "`r`n")+"`r`n"
 [IO.File]::WriteAllText((Join-Path $Session.appRoot 'config.ini'),(New-KsmConfiguration -Mode Auto -Existing $text),[Text.UTF8Encoding]::new($false))
}
function New-AsioFixture {
 param([hashtable]$Values=@{},[switch]$Song)
 $root=Split-Path $PSScriptRoot -Parent
 $build=Get-Content -LiteralPath (Join-Path $root 'build/latest.json') -Raw|ConvertFrom-Json
 if($build.PSObject.Properties.Name -contains 'diagnostics' -and -not $build.diagnostics){throw 'Diagnostic fixtures require scripts/Build.ps1 -Diagnostics; normal gameplay builds do not accept fixture flags.'}
 $session=New-KsmTestRun -Mode Prepare
 New-Item -ItemType File -Path (Join-Path $session.appRoot 'audio-test-fixture')|Out-Null
 Set-AsioFixtureConfig -Session $session -Values $Values
 if($Song){
  $source=Join-Path (Split-Path $PSScriptRoot -Parent) 'tests/fixtures/songs'
  foreach($child in Get-ChildItem -LiteralPath $source){Copy-Item -LiteralPath $child.FullName -Destination (Join-Path $session.appRoot 'songs') -Recurse}
 }
 return $session
}
function Invoke-AsioFixture {
 param($Session,[string[]]$Flags,[string]$Report,[string]$Expected,[int]$Timeout=60)
 $path=Join-Path $Session.appRoot $Report
 if(Test-Path -LiteralPath $path){Remove-Item -LiteralPath $path}
 Start-KsmOwnedProcess -Session $Session -Arguments $Flags -TimeoutSeconds $Timeout
 if(-not(Test-Path -LiteralPath $path)){throw "Missing report $Report; $($Session.root)"}
 if($Expected){$text=Get-Content -LiteralPath $path -Raw;if($text -notmatch $Expected){throw "Unexpected report: $text"}}
 return $path
}
Export-ModuleMember -Function New-AsioFixture,Set-AsioFixtureConfig,Invoke-AsioFixture