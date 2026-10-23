param()
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Import-Module (Join-Path $root 'tests/AsioHarness.psm1') -Force
$engine=Get-Content -LiteralPath (Join-Path $root 'build/audio-engine-latest.json') -Raw|ConvertFrom-Json
$destination=Join-Path $root 'reference/aps'
New-Item -ItemType Directory -Path $destination -Force|Out-Null
$results=@()
foreach($vertical in @(0,1)){
 $size=if($vertical){'720,1280'}else{'1280,720'}
 $session=New-AsioFixture -Values @{vertical_enabled=$vertical;windowsize=$size;vertical_resolution=$size;audio_backend='asio:KSM Mock ASIO';automaticmute=0;vsync=0}
 Copy-Item -LiteralPath (Join-Path $engine.runtime 'bassasio.dll') -Destination (Join-Path $session.appRoot 'bassasio.dll') -Force
 Copy-Item -LiteralPath (Join-Path $root 'tests/fixtures/songs/K-Shoot MANIA/magicalkittens/magicalkittens.ogg') -Destination (Join-Path $session.appRoot 'replay.ogg') -Force
 Copy-Item -LiteralPath (Join-Path $root 'tests/fixtures/songs/K-Shoot MANIA/magicalkittens/magicalkittens.jpg') -Destination (Join-Path $session.appRoot 'replay-jacket.jpg') -Force
 $flags=@('--aps-video-fixture');if($vertical){$flags+='--aps-video-portrait'}
 $report=Invoke-AsioFixture -Session $session -Flags $flags -Report 'aps-video.txt' -Expected 'TOTAL APS video passed' -Timeout 180
 $mode=if($vertical){'portrait'}else{'landscape'}
 foreach($file in Get-ChildItem -LiteralPath $session.appRoot -Filter 'aps-transition-*.mp4'){Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $destination $file.Name) -Force}
 foreach($file in Get-ChildItem -LiteralPath $session.appRoot -Filter 'aps-video-stage-*.png'){Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $destination ($mode+'-'+$file.Name)) -Force}
 Copy-Item -LiteralPath (Join-Path $session.appRoot 'aps-video-frames.csv') -Destination (Join-Path $destination ($mode+'-frames.csv')) -Force
 if(@(Get-ChildItem -LiteralPath (Join-Path $session.appRoot 'score') -Recurse -File -ErrorAction SilentlyContinue).Count){throw 'Video replay saved a score'}
 $results+=[pscustomobject]@{status='PASS';vertical=$vertical;session=$session.root;report=$report}
 $results|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $root 'build/aps-video-tests.json') -Encoding UTF8
 Write-Output (Get-Content -LiteralPath $report -Tail 1)
}
