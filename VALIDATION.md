# v0.3.0 验证记录

验证日期：2026-10-22。环境：Windows x64，Tecplot 360 EX 2025 R1（2025.1.0.72401），Qt 5.15.2。

| 检查项 | 结果 |
| --- | --- |
| 7,774 个匹配及规范化词条由实际 Qt 加载并逐条查询 | 通过 |
| QString、QVariant、QObject 属性读写接口 | 通过 |
| 干净目录使用发布源码重建 QM | 与发布 QM 字节完全一致 |
| PowerShell 脚本语法解析 | 通过 |
| 通用安装器安装、重新安装、重启 | 通过 |
| 恢复英文 | tecplot.add 与最初备份字节完全一致 |
| 菜单与快捷键 | 文件、远程加载、纸张设置已翻译；Alt+F 正常 |
| 欢迎页 | 入口、资源链接、提示和版本标签已翻译 |
| 重复加载自带 DuctFlow 示例 | 无重复菜单，图形正常显示 |
| 区域样式 | 标签、表头、Overlay/Solid 显示为中文 |
| 等值线与多色设置 | 级别、色带、截断、重复周期等显示中文 |
| 自带示例导出 PNG | 成功，1200×927 |
| 开启/停用汉化分别导出 ASCII 数据集 | 两次进程退出码均为 0，SHA256 相同 |

数据集 SHA256：

```
5C0B57A6F8E7BA7911FDA5CB3547C35CEF8C57F944EE8D3EFCD0B1BF554E1A89
```

这些检查说明所测试的流程通过，不代表所有功能均已验证。未逐一测试第三方插件、远程连接、全部错误提示、不同 DPI/主题或大型 Abaqus ODB 文件。用户数据和图中注释不作为语言包覆盖目标。两次图形渲染可能存在少量像素差异，不声明 PNG 逐像素一致。

## 可复现的核心检查

```powershell
python .\test_catalog.py 'C:\Program Files\Tecplot\Tecplot 360 EX 2025 R1\bin'
python .\test_qt_abi.py 'C:\Program Files\Tecplot\Tecplot 360 EX 2025 R1\bin'
```

导出对比使用 Tecplot 自带的 `examples/SimpleData/DuctFlow.lay`。加载布局后以 PNG 导出图形，并以 ASCII、12 位精度导出数据，分别在汉化启用/停用时比较数据文件 SHA256。
