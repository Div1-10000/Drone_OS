$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$msysUcrt = "C:\msys64\ucrt64\bin"
$msysUsr = "C:\msys64\usr\bin"
if (Test-Path $msysUcrt) { $env:Path = "$msysUcrt;$msysUsr;$env:Path" }

Write-Host "[DroneOS] Configuring C++17 project..." -ForegroundColor Cyan
cmake -S . -B build -G Ninja
Write-Host "[DroneOS] Building backend..." -ForegroundColor Cyan
cmake --build build
Write-Host "[DroneOS] Starting simulator and API on port 8080..." -ForegroundColor Green
Write-Host "Keep this terminal open while using the dashboard. Press Ctrl+C to stop." -ForegroundColor Yellow
& .\build\DroneOS.exe
