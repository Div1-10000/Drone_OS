$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "dashboard")
Write-Host "DroneOS dashboard: http://localhost:5500" -ForegroundColor Green
Write-Host "Keep this terminal open while using the dashboard. Press Ctrl+C to stop." -ForegroundColor Yellow
python -m http.server 5500
