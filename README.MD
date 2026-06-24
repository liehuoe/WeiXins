# WeiXins - 微信多开助手

[![Generic badge](https://img.shields.io/badge/C++-17+-blue.svg)](https://github.com/liehuoe/WeiXins)
[![Generic badge](https://img.shields.io/badge/MSVC-17+-blue.svg)](https://github.com/liehuoe/WeiXins)
[![Generic badge](https://img.shields.io/badge/gcc-15+-blue.svg)](https://github.com/liehuoe/WeiXins)

## 介绍

- 支持微信 4.0 以上的版本，不支持旧版本，需要的话可以联系我(有偿)增加支持；
- 界面使用 webview2 开发，用户电脑需要安装 webview2 runtime；
- 工具接管了快捷键`Ctrl + Alt + W`。多个微信的情况下，如果你刚关闭微信窗口，快捷键会重新唤醒刚关闭的微信窗口，否则快捷键会唤醒下一个微信的窗口；
- 增加了快捷键`Ctrl + Alt + <Number>`，用户唤醒特定的微信窗口。`<Number>`对应你启动的第几个微信窗口，比如启动了2个微信，那么快捷键`Ctrl + Alt + 2`会唤醒第二个微信；
- 修改了微信窗口的标题和图标，让用户更好识别不同的微信
- 自动跳过登录确认窗口

## 示例

![运行示例](./docs/running.gif)

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
