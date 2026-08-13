$ErrorActionPreference = 'Stop'

$shaderDirectory = Join-Path $PSScriptRoot 'shaders'
$outputPath = Join-Path $PSScriptRoot 'ShaderReplacements.generated.h'
$shaderFiles = @(Get-ChildItem -LiteralPath $shaderDirectory -Filter '*.arb' -File | Sort-Object Name)

if ($shaderFiles.Count -eq 0) {
    throw "No .arb files found in $shaderDirectory"
}

$lines = [Collections.Generic.List[string]]::new()
$lines.Add('#pragma once')
$lines.Add('')

$entries = [Collections.Generic.List[string]]::new()
for ($shaderIndex = 0; $shaderIndex -lt $shaderFiles.Count; $shaderIndex++) {
    $shaderFile = $shaderFiles[$shaderIndex]
    if ($shaderFile.Name -notmatch '^(fragment|vertex)_([0-9a-fA-F]{16})\.arb$') {
        throw "Invalid shader filename: $($shaderFile.Name)"
    }

    $target = if ($Matches[1] -eq 'fragment') { '0x8804u' } else { '0x8620u' }
    $hash = $Matches[2].ToLowerInvariant()
    $bytes = [IO.File]::ReadAllBytes($shaderFile.FullName)
    if ($bytes.Count -eq 0) {
        throw "Shader is empty: $($shaderFile.Name)"
    }

    $arrayName = "kShaderSource$shaderIndex"
    $lines.Add("static const unsigned char $arrayName[] = {")
    for ($offset = 0; $offset -lt $bytes.Count; $offset += 16) {
        $lastIndex = [Math]::Min($offset + 15, $bytes.Count - 1)
        $hexBytes = $bytes[$offset..$lastIndex] | ForEach-Object { '0x{0:X2}' -f $_ }
        $lines.Add('    ' + ($hexBytes -join ', ') + ',')
    }
    $lines.Add('};')
    $lines.Add('')
    $entries.Add("    {$target, 0x${hash}ull, $arrayName, sizeof($arrayName)},")
}

$lines.Add('static const ShaderSwapReplacement kShaderReplacements[] = {')
$lines.AddRange($entries)
$lines.Add('};')
$lines.Add('')
$lines.Add('static constexpr unsigned int kShaderReplacementCount =')
$lines.Add('    sizeof(kShaderReplacements) / sizeof(kShaderReplacements[0]);')

[IO.File]::WriteAllLines($outputPath, $lines, [Text.UTF8Encoding]::new($false))
Write-Host "Generated $outputPath with $($shaderFiles.Count) embedded shader replacement(s)."
Read-Host 'Press Enter to close' | Out-Null
