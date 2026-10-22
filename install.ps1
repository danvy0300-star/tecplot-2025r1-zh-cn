param([string]$TecplotRoot)
. (Join-Path $PSScriptRoot 'common.ps1')
$root = Find-TecplotRoot $TecplotRoot
Assert-TecplotClosed $root
$manifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'manifest.json') -Raw | ConvertFrom-Json
foreach ($name in @('tecplot_zh_CN.dll','tecplot_zh_CN.qm')) {
    if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $name)).Hash -ne $manifest.files.$name) {
        throw "文件校验失败：$name。请重新下载完整安装包。"
    }
}
foreach ($name in @('Qt5Core.dll','Qt5Widgets.dll','Qt5Gui.dll')) {
    if ((Get-FileHash -LiteralPath (Join-Path $root "bin\$name")).Hash -ne $manifest.host.$name) {
        throw "Qt 运行库与已验证版本不匹配：$name。为避免兼容问题，未进行安装。"
    }
}
$addon = Join-Path $root 'tecplot.add'
$backup = Join-Path $root '.tecplot-zh-cn-backup'
New-Item -ItemType Directory -Path $backup -Force | Out-Null
$text = [IO.File]::ReadAllText($addon)
$clean = Remove-ZhLoadBlock $text
$original = Join-Path $backup 'tecplot.add.original'
if (-not (Test-Path -LiteralPath $original)) {
    if ($text -eq $clean) { Copy-Item -LiteralPath $addon -Destination $original }
    else { [IO.File]::WriteAllText($original,$clean,[Text.UTF8Encoding]::new($false)) }
}
foreach ($name in @('tecplot_zh_CN.dll','tecplot_zh_CN.qm')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination (Join-Path $root "bin\$name") -Force
}
$block = "`r`n# BEGIN LOCAL ZH_CN 2025R1`r`n" + '$!LoadAddon "tecplot_zh_CN"' + "`r`n    IsOptional = True`r`n# END LOCAL ZH_CN 2025R1`r`n"
[IO.File]::WriteAllText($addon,$clean.TrimEnd()+$block,[Text.UTF8Encoding]::new($false))
Write-Output "汉化已安装到：$root。请从原快捷方式重新打开 Tecplot。"
