# 運動用品客製購物系統

C++17 和 wxWidgets 寫的 Windows 購物程式，有 14 款商品，每款都能印自己的名字，球衣還可以印姓名和背號。

<p align="center"><img src="docs/screenshots/turntable.gif" width="640" alt="商品圖左右拖曳旋轉 360 度的示範"></p>

## 怎麼跑

[下載 Windows 版](../../releases/latest)，解壓縮後執行。

## 自己編

需要 Visual Studio 2026（C++ 桌面開發，PlatformToolset v145）和 wxWidgets 3.3。

1. wxWidgets 要自己編成 x64 靜態函式庫，輸出在 `lib\vc_x64_lib`。
2. 設定 `WXWIN` 環境變數指到 wxWidgets 資料夾，或在 `CustomSportswearStore` 資料夾放一個 `wx.local.props`（不會進 git）：

```xml
<Project>
  <PropertyGroup>
    <WXWIN>C:\wxWidgets-3.3.0</WXWIN>
  </PropertyGroup>
</Project>
```

3. 打開 `CustomSportswearStore.sln`，選 x64 按 F5。
