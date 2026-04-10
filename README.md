# DX12 渲染管线 API 拦截与受限环境下的输入层重构框架
![Gametest.png](https://github.com/guangyu17/DX12hook-/blob/master/Game%20hook%20test.png)
## 项目概述

本项目旨在研究与实现针对 DirectX 12 渲染管线的底层拦截技术。通过脱离对第三方成熟库（如 MinHook）的依赖，本项目自主实现了一套轻量级的 API 拦截机制（API Interception / Method Overriding），成功挂钩了 DX12 SwapChain 及 CommandQueue 的五个核心渲染函数。该框架可用于图形渲染调试、性能监控（Overlay）以及自定义 UI 的无缝嵌入。

## 核心研究挑战：受限环境下的输入系统接管

在对具有内核级保护或高度沙盒化的应用（例如带有进程保护机制的商业软件）进行 UI 覆盖时，系统标准的 Windows 消息循环（`WndProc`）通常会被过滤或剥夺权限。常规的系统级消息传递（如 `SendMessage`）会面临失效或触发异常隔离的风险。

为解决这一问题，本项目深入研究了 Windows 底层输入架构，并提出了一套**独立于 WndProc 消息队列的平台无关性 UI 交互方案**。

### 技术实现方案

本项目基于 ImGui 的 Platform Agnostic 特性，重构了输入流向，彻底解耦了 GUI 渲染与 Windows 窗体消息的依赖：

#### 1. 硬件级输入轮询与 IO 状态机驱动

在无法获取合法 `WndProc` 回调的情况下，摒弃传统的事件驱动模式，转而采用底层状态轮询机制。

- **数据流向重构**：在每一帧的 `Render Loop` 周期内，直接调用 Windows 低级输入 API（如 `GetAsyncKeyState` 与底层游标读取）获取物理硬件的绝对状态。
- **状态映射**：将捕获的原始物理键鼠状态，经过屏幕空间到视口空间的坐标空间转换（ScreenToClient 算法模拟）后，直接喂入 `ImGuiIO` 状态机，实现无需 OS 消息队列的 UI 交互。

#### 2. 渲染架构对比与实现：Out-of-Process vs In-Process

针对不同的系统保护级别，本项目实现了两种 Overlay 渲染架构：

- **进程外覆盖 (Out-of-Process Overlay)**：通过构建带有 `WS_EX_TRANSPARENT` 等属性的置顶全透明独立窗口进行渲染。利用操作系统的窗口焦点管理机制，实现交互态（捕获输入）与非交互态（输入穿透）的平滑切换，有效避免了对目标进程地址空间的直接侵入。
- **进程内插桩 (In-Process Instrumentation)**：在目标进程地址空间内，通过挂钩底层的 `DirectInput` 或 `Raw Input` 等 API。在 UI 激活状态下，于底层输入流阶段拦截硬件信号并返回空载数据，从而实现“UI 层截获输入而底层应用无感知”的事件屏蔽机制。

### 扩展研究方向

- 基于 `Raw Input` (原始输入) API 的高频 HID 数据包深度解析。
- 分析不同输入注入方式（如模拟硬件中断）在内核行为监控下的特征差异与系统稳定性影响。
