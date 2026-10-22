param([string]$TecplotRoot)
. (Join-Path $PSScriptRoot 'common.ps1')
$root = Find-TecplotRoot $TecplotRoot
Assert-TecplotClosed $root
$addon = Join-Path $root 'tecplot.add'
$text = [IO.File]::ReadAllText($addon)
$clean = Remove-ZhLoadBlock $text
$original = Join-Path $root '.tecplot-zh-cn-backup\tecplot.add.original'
if ($text -ne $clean) {
    if ((Test-Path -LiteralPath $original) -and $clean.Trim() -eq [IO.File]::ReadAllText($original).Trim()) {
        Copy-Item -LiteralPath $original -Destination $addon -Force
    } else { [IO.File]::WriteAllText($addon,$clean,[Text.UTF8Encoding]::new($false)) }
}
Write-Output '汉化加载已停用。重新打开 Tecplot 后恢复英文。'
