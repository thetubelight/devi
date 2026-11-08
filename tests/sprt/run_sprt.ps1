<#
.SYNOPSIS
  SPRT on Windows: does -New beat -Base? Wraps fastchess (preferred) or cutechess-cli.

.EXAMPLE
  .\tests\sprt\run_sprt.ps1 -New .\bin\devi-candidate.exe -Base .\bin\devi-v1.0.exe
  .\tests\sprt\run_sprt.ps1 -New .\bin\devi-candidate.exe -Base .\bin\devi-v1.0.exe -TC "10+0.1" -Elo1 3 -Concurrency 8
  .\tests\sprt\run_sprt.ps1 ... -Tool C:\tools\fastchess\fastchess.exe

  Early development (big gains expected): -Elo1 10. Later: -Elo1 5 or 3.
  Download fastchess (Windows build) from https://github.com/Disservin/fastchess/releases and either put it on PATH or pass -Tool.
  Verify flag names against your installed version (fastchess --help).
  If script execution is blocked:  powershell -ExecutionPolicy Bypass -File .\tests\sprt\run_sprt.ps1 ...
#>
param(
    [Parameter(Mandatory = $true)][string]$New,
    [Parameter(Mandatory = $true)][string]$Base,
    [string]$TC = "8+0.08",
    [double]$Elo0 = 0,
    [double]$Elo1 = 5,
    [double]$Alpha = 0.05,
    [double]$Beta = 0.05,
    [int]$Concurrency = 6,        # i5-13450HX has 16 threads; leave headroom so timing isn't distorted
    [int]$Rounds = 20000,         # each round = 2 games (colors swapped on the same opening)
    [int]$Hash = 16,
    [string]$Book = (Join-Path $PSScriptRoot "openings\book.epd"),
    [string]$Tool = "",
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Extra = @()
)
$ErrorActionPreference = "Stop"

$newPath  = (Resolve-Path $New).Path
$basePath = (Resolve-Path $Base).Path
$bookPath = (Resolve-Path $Book).Path
$log = "sprt_{0}.log" -f (Get-Date -Format "yyyyMMdd_HHmmss")

if (-not $Tool) {
    foreach ($name in @("fastchess", "cutechess-cli")) {
        if (Get-Command $name -ErrorAction SilentlyContinue) { $Tool = $name; break }
    }
}
if (-not $Tool) { throw "Neither fastchess nor cutechess-cli found on PATH. Install one, or pass -Tool <path-to-exe>." }

$toolArgs = @(
    "-engine", "cmd=$newPath",  "name=new",  "option.Hash=$Hash",
    "-engine", "cmd=$basePath", "name=base", "option.Hash=$Hash",
    "-each", "tc=$TC", "proto=uci",
    "-openings", "file=$bookPath", "format=epd", "order=random",
    "-rounds", "$Rounds", "-games", "2", "-repeat",
    "-concurrency", "$Concurrency", "-recover",
    "-sprt", "elo0=$Elo0", "elo1=$Elo1", "alpha=$Alpha", "beta=$Beta",
    "-ratinginterval", "50"
) + $Extra

& $Tool @toolArgs 2>&1 | Tee-Object -FilePath $log
Write-Host "Log saved to $log. Append the result to tests\sprt\README.md."
