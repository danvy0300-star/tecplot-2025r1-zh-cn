$ErrorActionPreference = 'Stop'
function Find-TecplotRoot([string]$Requested) {
    $candidates = [Collections.Generic.List[string]]::new()
    if ($Requested) { $candidates.Add($Requested) }
    if (-not $Requested) {
        foreach ($desktop in @([Environment]::GetFolderPath('CommonDesktopDirectory'),[Environment]::GetFolderPath('DesktopDirectory'))) {
            $link = Join-Path $desktop 'Tecplot 360 EX 2025 R1.lnk'
            if (Test-Path -LiteralPath $link) {
                $shortcut = (New-Object -ComObject WScript.Shell).CreateShortcut($link)
                if ($shortcut.TargetPath) { $candidates.Add((Split-Path -Parent (Split-Path -Parent $shortcut.TargetPath))) }
            }
        }
        foreach ($programRoot in @($env:ProgramFiles,${env:ProgramFiles(x86)})) {
            if ($programRoot) { $candidates.Add((Join-Path $programRoot 'Tecplot\Tecplot 360 EX 2025 R1')) }
        }
    }
    foreach ($candidate in $candidates) {
        $exe = Join-Path $candidate 'bin\tec360.exe'
        if ((Test-Path -LiteralPath $exe) -and (Get-Item -LiteralPath $exe).VersionInfo.FileVersion -eq '2025.1.0.72401') {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    if ($Requested) { throw '指定目录不是受支持的 Tecplot 2025.1.0.72401 安装目录。' }
    Add-Type -AssemblyName System.Windows.Forms
    $dialog = New-Object Windows.Forms.OpenFileDialog
    $dialog.Title = '请选择 Tecplot 2025 R1 的 bin\tec360.exe'
    $dialog.Filter = 'Tecplot executable (tec360.exe)|tec360.exe'
    if ($dialog.ShowDialog() -ne 'OK') { throw '已取消选择。' }
    return Find-TecplotRoot (Split-Path -Parent (Split-Path -Parent $dialog.FileName))
}
function Assert-TecplotClosed([string]$Root) {
    $exe = Join-Path $Root 'bin\tec360.exe'
    foreach ($process in @(Get-Process tec360 -ErrorAction SilentlyContinue)) {
        if ($process.Path -eq $exe) { throw '请先退出 Tecplot，然后重新运行。' }
    }
}
function Remove-ZhLoadBlock([string]$Text) {
    return [regex]::Replace($Text,'(?m)^# BEGIN LOCAL ZH_CN 2025R1\r?\n[\s\S]*?^# END LOCAL ZH_CN 2025R1\r?\n?','')
}
