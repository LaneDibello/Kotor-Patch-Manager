<#
.SYNOPSIS
    Emits a cache key for the shared Common/GameAPI static library.

.DESCRIPTION
    create-patch.bat caches Common.lib so that every DETOUR patch does not have to
    recompile the ~48 shared translation units. This script produces the key that
    decides whether that cache is still valid: one line per Common source and
    header, carrying its full path, size, and last-write time.

    The batch script writes this output to sources.new and compares it against the
    sources.stamp left by the previous successful build. Any edit, addition, or
    deletion changes a line, so the library is rebuilt.

    This is deliberately not done with `dir` in the batch script itself. `dir`
    reports timestamps only to the minute, so a header edited and rebuilt inside
    the same minute would look unchanged and the build would silently link a stale
    Common.lib -- the worst possible failure here, because the patch would compile
    clean against new headers while linking code built from the old ones. Ticks are
    100ns resolution, which closes that window.

    Timestamps and sizes are used rather than content hashes because they are what
    build systems normally key on, and reading every file's contents would cost
    more than the build this cache is meant to save. The one case that misses --
    replacing a file with different content of identical size and identical
    100ns-resolution timestamp -- cannot occur in practice.

.PARAMETER CommonDir
    Path to the Patches\Common directory.
#>
param(
    [Parameter(Mandatory = $true)]
    [string] $CommonDir
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $CommonDir)) {
    Write-Error "Common directory not found: $CommonDir"
    exit 1
}

# Sort by path so the listing order does not depend on how the filesystem
# enumerates entries; an unstable order would look like a change and rebuild
# the library on every invocation.
#
# The extension test is a Where-Object rather than -Include on purpose:
# Get-ChildItem silently ignores -Include when it is combined with -LiteralPath,
# which would let every file under the directory into the key. -LiteralPath is
# still the right way to name the directory, because -Path would treat any
# bracket in the repository path as a wildcard.
Get-ChildItem -LiteralPath $CommonDir -Recurse -File |
    Where-Object { $_.Extension -eq '.cpp' -or $_.Extension -eq '.h' } |
    Sort-Object -Property FullName |
    ForEach-Object { "$($_.FullName)|$($_.Length)|$($_.LastWriteTimeUtc.Ticks)" }
