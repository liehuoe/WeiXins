# WeiXins - 微信多开助手

[![Generic badge](https://img.shields.io/badge/C++-17+-blue.svg)](https://github.com/liehuoe/WeiXins)
[![Generic badge](https://img.shields.io/badge/MSVC-17+-blue.svg)](https://github.com/liehuoe/WeiXins)
[![Generic badge](https://img.shields.io/badge/gcc-15+-blue.svg)](https://github.com/liehuoe/WeiXins)

- 支持微信 4.0 以上的版本，不支持旧版本，需要的话可以联系我(有偿)增加支持；
- 界面使用 webview2 开发，用户电脑需要安装 webview2 runtime；

## 介绍

1. 界面支持根据系统设置切换至深/浅色模式
1. 第一次登录后程序会获取并显示微信头像（名称是自定义的，没有获取微信名称）
1. 第一次登录后程序会自动跳过微信弹出的登录确认框
1. 工具修改了微信窗口的标题和图标，以便在使用过程中识别不同的微信
1. 工具接管了快捷键`Ctrl + Alt + W`。多个微信的情况下，如果你刚关闭微信窗口，快捷键会重新打开刚关闭的微信窗口，否则快捷键会打开下一个微信的窗口；
1. 增加了快捷键`Ctrl + Alt + <Number>`，用户打开/关闭特定的微信窗口。`<Number>`对应你启动的第几个微信窗口，比如启动了2个微信，那么快捷键`Ctrl + Alt + 2`会打开/关闭第二个微信；
1. 支持多选模式，多选模式下可以一键批量登录多个微信

## 示例

![运行示例](./docs/running.gif)

## 编译

```bash
# 编译前端页面
cd pages
npm install
npm run build
# 编译C++
cd ..
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 如果编译 Debug 版本，C++会通过 http://localhost:5173 加载页面
# 这时需要通过 npm run dev 启动前端，方便开发前端页面

# 启动前端页面
cd pages
npm install
npm run dev
# 编译C++
cd ..
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## 支持

<table>
    <td align="center">
        <div>我的微信</div>
        <img src="pages/public/1001.jpg" width="200px" />
    </td>
    <td align="center">
        <div>打赏支持</div>
        <img src="pages/public/1002.jpg" width="200px" />
    </td>
</table>
