$ErrorActionPreference = "Stop"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake not found. Install CMake 3.22+ and reopen PowerShell."
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git not found. Install Git for Windows and reopen PowerShell."
}

$cmakeContent = Get-Content "CMakeLists.txt" -Raw
$versionMatch = [regex]::Match($cmakeContent, 'project\("Santos Leveler" VERSION\s+([\d.]+)')
if (-not $versionMatch.Success) {
    throw "Could not extract the Santos Leveler version from CMakeLists.txt."
}
$version = $versionMatch.Groups[1].Value

Write-Host "=== SANTOS LEVELER v$version - Windows x64 VST3 build ===" -ForegroundColor Cyan

cmake --preset windows-x64-release
cmake --build --preset windows-x64-release --target SantosLeveler_VST3 SantosLevelerDSPTests
ctest --test-dir build/windows-x64 -C Release --output-on-failure

$plugin = Get-ChildItem -Path "build/windows-x64" -Recurse -Directory -Filter "*.vst3" | Select-Object -First 1
if ($null -eq $plugin) {
    throw "Build completed, but no .vst3 bundle was found."
}

Write-Host ""
Write-Host "VST3 created:" -ForegroundColor Green
Write-Host $plugin.FullName
Write-Host ""
Write-Host "To install it for all users, copy the whole .vst3 folder to:" -ForegroundColor Yellow
Write-Host "C:\Program Files\Common Files\VST3"
