$ErrorActionPreference = "Stop"

$plugin = Get-ChildItem -Path "build/windows-x64" -Recurse -Directory -Filter "*.vst3" | Select-Object -First 1
if ($null -eq $plugin) {
    throw "No built VST3 found. Run build-windows.ps1 first."
}

$dest = "C:\Program Files\Common Files\VST3"
Write-Host "Installing $($plugin.Name) to $dest"
Copy-Item -Path $plugin.FullName -Destination $dest -Recurse -Force
Write-Host "Installed. Rescan VST3 plug-ins in your DAW." -ForegroundColor Green
