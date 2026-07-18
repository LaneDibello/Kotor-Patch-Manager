<#
.SYNOPSIS
  Build KOTOR patches in parallel and collect them into a release folder.

.DESCRIPTION
  The parallel counterpart to the serial for-loop that publish.bat used to run.
  Each patch directory (one containing a manifest.toml) is fully isolated -- its
  own working directory, its own intermediates (obj/def/kpatch), and no shared
  PDB -- so create-patch.bat is safe to run concurrently across patches. This
  fans the builds out across background jobs, throttled to the CPU count, then
  copies the produced .kpatch files (and any "additional" folders) into the
  release and prints a pass/fail summary.

  PowerShell 5.1 compatible on purpose: it uses Start-Job because
  ForEach-Object -Parallel does not exist before PowerShell 7.

.PARAMETER PatchesDir
  The Patches directory (holds the per-patch folders and create-patch.bat).

.PARAMETER OutDir
  Where the collected .kpatch files and "additional files" folders are written.

.PARAMETER ThrottleLimit
  Max concurrent builds. 0 (default) uses the processor count.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PatchesDir,
    [Parameter(Mandatory)][string]$OutDir,
    [int]$ThrottleLimit = 0
)

$ErrorActionPreference = 'Stop'

$PatchesDir = (Resolve-Path -LiteralPath $PatchesDir).Path
if (-not (Test-Path -LiteralPath $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
}
$OutDir = (Resolve-Path -LiteralPath $OutDir).Path
$createPatchBat = Join-Path $PatchesDir 'create-patch.bat'

if ($ThrottleLimit -le 0) {
    $ThrottleLimit = [int]$env:NUMBER_OF_PROCESSORS
    if ($ThrottleLimit -le 0) { $ThrottleLimit = 4 }
}

$patches = Get-ChildItem -LiteralPath $PatchesDir -Directory | Where-Object {
    Test-Path -LiteralPath (Join-Path $_.FullName 'manifest.toml')
} | Sort-Object Name

if (-not $patches) {
    Write-Host '  No patches with manifest.toml found.'
    return
}

Write-Host ("  {0} patch(es); building with up to {1} parallel job(s)..." -f $patches.Count, $ThrottleLimit)

# What each background job runs: create-patch.bat inside the patch dir. stdin is
# redirected from NUL so an error-path `pause` in the .bat returns immediately
# (EOF) instead of hanging a job that has no console. SKIP_PAUSE covers the final
# success pause; <NUL covers the rest.
$work = {
    param($Dir, $Bat, $Name)
    $env:SKIP_PAUSE = '1'
    $out = & cmd.exe /c "cd /d `"$Dir`" && call `"$Bat`" <NUL" 2>&1
    [pscustomobject]@{
        Name   = $Name
        Code   = $LASTEXITCODE
        Output = ($out -join [Environment]::NewLine)
    }
}

# Start-Job has no native throttle, so gate new starts on the running count.
$jobs = New-Object System.Collections.Generic.List[object]
foreach ($p in $patches) {
    while (@($jobs | Where-Object { $_.State -eq 'Running' }).Count -ge $ThrottleLimit) {
        Start-Sleep -Milliseconds 150
    }
    $jobs.Add((Start-Job -ScriptBlock $work -ArgumentList $p.FullName, $createPatchBat, $p.Name))
}
$null = $jobs | Wait-Job

# Gather each job's result object, keyed by patch name.
$byName = @{}
foreach ($j in $jobs) {
    $r = Receive-Job $j
    Remove-Job $j
    if ($r) { $byName[$r.Name] = $r }
}

# Collect outputs serially (fast file copies; avoids concurrent writes to OutDir).
$built = 0
$failed = @()
foreach ($p in $patches) {
    $r = $byName[$p.Name]
    $kpatch = Get-ChildItem -LiteralPath $p.FullName -Filter '*.kpatch' -ErrorAction SilentlyContinue |
        Select-Object -First 1

    if ($r -and $r.Code -eq 0 -and $kpatch) {
        Copy-Item -LiteralPath $kpatch.FullName -Destination $OutDir -Force
        Write-Host ("    [OK]   {0}" -f $kpatch.Name)
        $built++
    } else {
        Write-Host ("    [WARN] No .kpatch produced for {0}" -f $p.Name)
        $failed += $p.Name
        if ($r -and $r.Output) {
            # Surface the tail of the failed build so it is diagnosable.
            ($r.Output -split "`r?`n" | Where-Object { $_ } | Select-Object -Last 6) |
                ForEach-Object { Write-Host "           $_" }
        }
    }

    # "additional files" ship regardless of build result, mirroring publish.bat.
    $addl = Join-Path $p.FullName 'additional'
    if ((Test-Path -LiteralPath $addl) -and
        (Get-ChildItem -LiteralPath $addl -Force -ErrorAction SilentlyContinue)) {
        $dest = Join-Path $OutDir ("{0} additional files" -f $p.Name)
        New-Item -ItemType Directory -Path $dest -Force | Out-Null
        Copy-Item -Path (Join-Path $addl '*') -Destination $dest -Recurse -Force
        Write-Host ("    [OK]   Copied additional files for {0}" -f $p.Name)
    }
}

Write-Host ''
Write-Host ("  Patches built: {0}/{1}" -f $built, $patches.Count)
if ($failed.Count -gt 0) {
    Write-Host ("  Not produced : {0}" -f ($failed -join ', '))
}
