# Builds the Release exe and packs everything a user needs into one zip:
#   dist\CustomSportswearStore-<version>-win64.zip
#     CustomSportswearStore\CustomSportswearStore.exe
#     CustomSportswearStore\assets\...
#     CustomSportswearStore\msvcp140.dll, vcruntime140.dll, vcruntime140_1.dll
#     CustomSportswearStore\README.txt
#
# Usage (from the repo root, in PowerShell):
#   powershell -ExecutionPolicy Bypass -File tools\package.ps1 -Version v2.2.0

param([string]$Version = "v2.2.0")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# Visual Studio + MSBuild
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $vs "MSBuild\Current\Bin\MSBuild.exe"

& $msbuild CustomSportswearStore.sln /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

# The C++ runtime DLLs may be shipped next to the exe (Microsoft's
# "app-local" deployment), so the app runs without installing anything.
$crt = Get-ChildItem (Join-Path $vs "VC\Redist\MSVC\*\x64\Microsoft.VC*.CRT") | Sort-Object FullName | Select-Object -Last 1

$out = Join-Path $root "dist\CustomSportswearStore"
if (Test-Path (Join-Path $root "dist")) { Remove-Item (Join-Path $root "dist") -Recurse -Force }
New-Item -ItemType Directory -Force -Path $out | Out-Null

Copy-Item "x64\Release\CustomSportswearStore.exe" $out
Copy-Item "assets" (Join-Path $out "assets") -Recurse
foreach ($dll in "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll") {
    Copy-Item (Join-Path $crt.FullName $dll) $out
}

$readme = @"
運動用品客製購物系統 Custom Sportswear Store $Version

使用方式
1. 把整個 CustomSportswearStore 資料夾解壓縮到任何地方（例如桌面）。
2. 雙擊 CustomSportswearStore.exe 開啟。
   第一次開啟時 Windows 可能跳出「Windows 已保護您的電腦」，
   按「其他資訊」→「仍要執行」即可（程式沒有數位簽章才會出現這個提示）。

操作小提示
- 商品圖可以左右拖曳 360° 旋轉，雙擊回到正面。
- F11 切換全螢幕，Esc 離開全螢幕。
- 購物車可輸入優惠碼：WELCOME100（滿 NT`$1,000 折 NT`$100）、TEAM10（5 件以上 9 折）。
- 單筆滿 NT`$2,000 免運費。
- 本程式為課堂專題展示用途，不會實際收款或寄送商品。

系統需求：Windows 10 / 11（64 位元）

----
Custom Sportswear Store $Version
Unzip the whole folder anywhere and double-click CustomSportswearStore.exe.
If Windows SmartScreen appears, choose "More info" > "Run anyway" (the app is not code-signed).
Drag a product to turn it round; F11 toggles full screen. This is a demo: no payment or shipping happens.
Requires 64-bit Windows 10 or 11.
"@
Set-Content -Path (Join-Path $out "README.txt") -Value $readme -Encoding UTF8

$zip = Join-Path $root "dist\CustomSportswearStore-$Version-win64.zip"
Compress-Archive -Path $out -DestinationPath $zip
Write-Host "Packed $zip"
