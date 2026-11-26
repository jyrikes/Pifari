$ErrorActionPreference = "Stop"
$python = Join-Path $HOME "miniconda3\python.exe"
if (-not (Test-Path $python)) {
    $python = (Get-Command python.exe -ErrorAction Stop).Source
}
& $python (Join-Path $PSScriptRoot "gerar_catalogo.py")
$port = Get-NetTCPConnection -LocalAddress 127.0.0.1 -LocalPort 18766 -State Listen -ErrorAction SilentlyContinue
if (-not $port) {
    $quotedGuide = '"' + $PSScriptRoot + '"'
    $args = @("-m", "http.server", "18766", "--bind", "127.0.0.1", "--directory", $quotedGuide)
    Start-Process -FilePath $python -ArgumentList $args -WorkingDirectory $PSScriptRoot -RedirectStandardOutput (Join-Path $env:TEMP "PifariGuideServer.log") -RedirectStandardError (Join-Path $env:TEMP "PifariGuideServer.err") -WindowStyle Hidden
    Start-Sleep -Seconds 2
}
$response = Invoke-WebRequest -Uri "http://127.0.0.1:18766/" -UseBasicParsing -TimeoutSec 5
if ($response.StatusCode -ne 200) { throw "O servidor do guia respondeu com HTTP $($response.StatusCode)." }
Start-Process "http://127.0.0.1:18766/"
Write-Output "Guia Pifari: http://127.0.0.1:18766/"
