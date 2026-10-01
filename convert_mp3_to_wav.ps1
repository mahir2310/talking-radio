$ErrorActionPreference = "Stop"

$audioRoot = Join-Path $PSScriptRoot "docs\audio"
$ffmpegCommand = Get-Command ffmpeg -ErrorAction SilentlyContinue
if ($null -eq $ffmpegCommand) {
    throw "FFmpeg was not found. Install FFmpeg, reopen PowerShell, and run this script again."
}

$mp3Files = Get-ChildItem -LiteralPath $audioRoot -Filter "*.mp3" -File -Recurse
if ($mp3Files.Count -eq 0) {
    throw "No MP3 recordings were found under $audioRoot"
}

$completed = 0
foreach ($mp3 in $mp3Files) {
    $wavPath = [System.IO.Path]::ChangeExtension($mp3.FullName, ".wav")
    & $ffmpegCommand.Source -y -i $mp3.FullName -map_metadata -1 -ac 1 -ar 22050 -c:a pcm_s16le -f wav $wavPath
    if ($LASTEXITCODE -ne 0) {
        throw "FFmpeg failed to convert $($mp3.FullName)"
    }
    $completed++
    Write-Host "[$completed/$($mp3Files.Count)] Created $wavPath"
}

Write-Host "Created $completed mono, 22050 Hz, 16-bit PCM WAV files."
