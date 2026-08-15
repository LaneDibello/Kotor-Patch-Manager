<#
.SYNOPSIS
  Build KOTOR patches in parallel and collect them into a release folder.

.DESCRIPTION
  The parallel counterpart to the serial for-loop that publish.bat used to run.
  Each patch directory (one containing a manifest.toml) keeps its own working
  directory, intermediates (obj/def/kpatch), and no shared PDB, so the per-patch
  half of create-patch.bat runs safely side by side. This fans the builds out
  across background jobs, throttled to the CPU count, then copies the produced
  .kpatch files (and any "additional" folders) into the release and prints a
  pass/fail summary.

  The one thing DETOUR patches do share is the cached Common/GameAPI static
  library, which create-patch.bat builds on first use. Several jobs building it
  at once would write the same object files and the same archive concurrently,
  so one DETOUR patch is built serially first to seed that cache. After that the
  remaining jobs only read it, and the fan-out is safe again.

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

# A patch is DETOUR (needs the shared Common library) if it has any .cpp in its
# root or an immediate subdirectory. This mirrors how create-patch.bat decides.
function Test-IsDetourPatch {
    param([string]$Dir)
    if (Get-ChildItem -LiteralPath $Dir -Filter '*.cpp' -File -ErrorAction SilentlyContinue) {
        return $true
    }
    foreach ($sub in Get-ChildItem -LiteralPath $Dir -Directory -ErrorAction SilentlyContinue) {
        if (Get-ChildItem -LiteralPath $sub.FullName -Filter '*.cpp' -File -ErrorAction SilentlyContinue) {
            return $true
        }
    }
    return $false
}

$byName = @{}

# Seed the shared Common/GameAPI library with one serial build before fanning
# out, so the parallel jobs find it already cached and only ever read it. When
# the cache is already warm this costs one ordinary patch build.
$seed = $patches | Where-Object { Test-IsDetourPatch $_.FullName } | Select-Object -First 1
if ($seed) {
    Write-Host ("  Seeding shared Common library via {0}..." -f $seed.Name)
    $seedResult = & $work $seed.FullName $createPatchBat $seed.Name
    if ($seedResult) { $byName[$seedResult.Name] = $seedResult }

    # Every DETOUR patch links this library, so if it cannot be built there is no
    # point starting dozens of jobs that will each fail the same way -- and they
    # would race rebuilding it, burying the real error in sharing violations.
    if (-not $seedResult -or $seedResult.Code -ne 0) {
        Write-Host ''
        Write-Host '  ERROR: the shared Common/GameAPI library failed to build.'
        Write-Host '         Every DETOUR patch depends on it, so the build is stopping here.'
        if ($seedResult -and $seedResult.Output) {
            ($seedResult.Output -split "`r?`n" | Where-Object { $_ } | Select-Object -Last 15) |
                ForEach-Object { Write-Host "         $_" }
        }
        throw 'Shared Common library build failed.'
    }
}

# Start-Job has no native throttle, so gate new starts on the running count.
$jobs = New-Object System.Collections.Generic.List[object]
foreach ($p in $patches) {
    if ($seed -and $p.FullName -eq $seed.FullName) { continue }
    while (@($jobs | Where-Object { $_.State -eq 'Running' }).Count -ge $ThrottleLimit) {
        Start-Sleep -Milliseconds 150
    }
    $jobs.Add((Start-Job -ScriptBlock $work -ArgumentList $p.FullName, $createPatchBat, $p.Name))
}
$null = $jobs | Wait-Job

# Gather each job's result object, keyed by patch name.
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
