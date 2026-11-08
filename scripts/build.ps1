<#
.SYNOPSIS
  Release build of Devi on Windows (CMake with Visual Studio/MSVC, or MinGW-w64/MSYS2).

.EXAMPLE
  .\scripts\build.ps1                               # default generator (Visual Studio if installed)
  .\scripts\build.ps1 -Native                       # CPU-specific build (/arch:AVX2 or -march=native); don't ship it elsewhere
  .\scripts\build.ps1 -Generator "MinGW Makefiles"  # or "Ninja" with a MinGW-w64 toolchain on PATH

  If script execution is blocked:  powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
#>
param(
    [switch]$Native,
    [string]$Generator = "",
    [string]$BuildDir = "build"
)
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "cmake not found. Install Visual Studio 2022 (Desktop development with C++) or CMake + MinGW-w64, and run from a Developer PowerShell."
}

$configure = @("-S", ".", "-B", $BuildDir, "-DCMAKE_BUILD_TYPE=Release")
if ($Native)    { $configure += "-DDEVI_NATIVE=ON" }
if ($Generator) { $configure += @("-G", $Generator) }

& cmake @configure
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

& cmake --build $BuildDir --config Release -j
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

# Visual Studio puts binaries in build\Release\, single-config generators (Ninja/MinGW Makefiles) in build\.
$exe = @("$BuildDir\Release\devi.exe", "$BuildDir\devi.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($exe) { Write-Host "Built: $exe" } else { Write-Warning "Build finished but devi.exe was not found under $BuildDir" }
