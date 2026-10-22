param([string]$Zig = 'zig', [string]$Python = 'python')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    & $Python 'build_catalog.py'
    if ($LASTEXITCODE -ne 0) { throw 'Catalog build failed.' }
    & $Zig cc -shared -O2 'tecplot_zh_CN.c' -o 'tecplot_zh_CN.dll' -lversion
    if ($LASTEXITCODE -ne 0) { throw 'Plugin build failed.' }
    $manifestPath = Join-Path $PSScriptRoot 'manifest.json'
    if (Test-Path -LiteralPath $manifestPath) {
        $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
        foreach ($name in @('tecplot_zh_CN.dll','tecplot_zh_CN.qm')) { $manifest.files.$name = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $name)).Hash }
        [IO.File]::WriteAllText($manifestPath,($manifest | ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
    }
    Write-Output 'Build complete; package hashes updated.' 
} finally { Pop-Location }
