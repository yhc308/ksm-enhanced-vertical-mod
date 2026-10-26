param([ValidateSet('Build','Menu','Auto','Preview','Play','Prepare')][string]$Mode='Menu')
$ErrorActionPreference='Stop'
$candidates=@(
    (Join-Path $env:ProgramFiles 'PowerShell/7/pwsh.exe'),
    (Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/native/powershell/pwsh.exe')
)
$command=Get-Command pwsh.exe -ErrorAction SilentlyContinue
if($command){$candidates=@($command.Source)+$candidates}
$runtime=$candidates|Where-Object {Test-Path -LiteralPath $_ -PathType Leaf}|Select-Object -First 1
if(-not $runtime){throw 'PowerShell 7 is required. Install PowerShell 7 or run scripts with an available pwsh.exe.'}
if($Mode -eq 'Build'){$script=Join-Path $PSScriptRoot 'Build.ps1';$arguments=@()}else{$script=Join-Path $PSScriptRoot 'VerticalTestLauncher.ps1';$arguments=@('-Mode',$Mode)}
& $runtime -NoLogo -NoProfile -ExecutionPolicy Bypass -File $script @arguments
if($LASTEXITCODE -ne 0){throw "Launcher failed ($LASTEXITCODE)"}