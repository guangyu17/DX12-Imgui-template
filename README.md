# DX12hook框架
未使用 MinHook 实现了基本的 对dx12 游戏的基本hook hook了五个核心 func

![Gametest.png](https://github.com/guangyu17/DX12hook-/blob/master/Game%20hook%20test.png)

------
## 注意了 注入EAC游戏会遇到这个 问题 以下是解决方法


这是一个涉及到**反作弊机制（EAC）和输入层级**的技术问题。

首先需要明确一点：**EAC (Easy Anti-Cheat)** 等反作弊系统通常会在内核层过滤或通过 `ObRegisterCallbacks` 等机制剥夺普通进程对游戏窗口句柄的权限，或者通过钩子拦截 `WndProc` 中的消息。这意味着标准的 `SendMessage` 或 `PostMessage` 可能会失败，或者被反作弊视为异常行为而标记。

但我可以从 **ImGui 的底层原理** 和 **输入处理逻辑** 的角度，告诉你如何在没有标准 Windows 消息（如 `WM_LBUTTONDOWN` 等）的情况下，让 ImGui 正常响应输入。

ImGui 是平台无关的（Platform Agnostic），它本质上并不依赖 Windows 消息，它只依赖 `ImGuiIO` 结构体中的数据。因此，你不需要“伪造消息”，而是应该**直接驱动 ImGui 的 IO 核心**。

以下是几种在受限环境下处理 ImGui 输入的通用技术方案：

------

### 1. 核心思路：抛弃 WndProc，直接操作 `ImGuiIO`

通常 ImGui 的初始化（如 `ImGui_ImplWin32_WndProcHandler`）依赖于 Windows 消息循环。但在 EAC 拦截了消息的情况下，你可以手动轮询硬件输入状态，并直接赋值给 ImGui。

你可以在你的 `Render` 循环（每一帧 `ImGui::NewFrame()` 之前）手动更新 `ImGuiIO`。

#### 代码实现示例

你需要使用 Windows API（如 `GetAsyncKeyState` 和 `GetCursorPos`）来获取全局输入状态，绕过游戏窗口的消息队列。

C++

```
#include <imgui.h>
#include <windows.h>

void UpdateImGuiInputManual()
{
    ImGuiIO& io = ImGui::GetIO();

    // 1. 处理鼠标位置 (直接获取屏幕坐标并转换为窗口相对坐标)
    POINT p;
    if (GetCursorPos(&p))
    {
        // 如果是外部绘制(Overlay)，直接用屏幕坐标
        // 如果是内部绘制(Hook)，需要ScreenToClient转换，但EAC可能会拦截ScreenToClient
        // 建议：如果是内部Hook，尽量自行计算或读取游戏内存中的鼠标位置
        io.MousePos = ImVec2((float)p.x, (float)p.y);
    }

    // 2. 处理鼠标按键 (直接轮询物理按键状态)
    // 0: 左键, 1: 右键, 2: 中键
    io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    io.MouseDown[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

    // 3. 处理键盘输入 (仅举例常用键，实际可能需要遍历)
    // 注意：ImGui 新版本使用 AddKeyEvent，旧版本直接操作 KeysDown
    io.KeyCtrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    
    // 如果需要文本输入，这一步比较麻烦，因为 GetAsyncKeyState 不处理字符映射。
    // 通常通过 Hook 游戏的输入处理函数（如 DirectInput 或 XInput）来获取更底层的输入。
}

// 在你的渲染循环中：
void RenderLoop()
{
    UpdateImGuiInputManual(); // <--- 手动喂数据
    
    ImGui_ImplDX11_NewFrame(); // 或者你使用的其他后端
    ImGui_ImplWin32_NewFrame(); // 这一步里通常会重置某些IO，要注意调用顺序
    ImGui::NewFrame();
    
    // ... 绘制你的 UI ...
}
```

### 2. 外部绘制 (External Overlay) vs 内部注入 (Internal Hook)

根据你的工具类型，解决方案截然不同：

#### A. 如果你是外部工具 (External Overlay)

这是最安全的做法。你不应该试图向游戏窗口发送消息。

- **做法：** 创建一个**透明的、置顶的 (TopMost)** 窗口，覆盖在游戏窗口上方。
- **输入流向：** 所有的鼠标点击和键盘事件实际上是发送给**你的透明窗口**的，而不是游戏窗口。
- **交互逻辑：**
  - 当菜单打开时（例如按下 Insert 键），你的窗口捕获输入焦点。
  - 当菜单关闭时，你的窗口通过 `WS_EX_TRANSPARENT` 或穿透属性，让鼠标点击“穿透”到后面的游戏窗口。
- **优势：** EAC 无法阻止发送给你自己创建的窗口的消息。

#### B. 如果你是内部注入 (Internal Hook)

如果你通过 DLL 注入在游戏进程内运行，EAC 确实会保护 `WndProc`。

- **Hook 游戏的输入 API：** 游戏本身也要接收输入（通常不完全依赖 WndProc，而是使用 DirectInput 或 RawInput）。
- **做法：** 既然你在进程内部，你可以 Hook 游戏的输入处理函数。
- **常用 Hook 点：**
  - `SetCursorPos` / `GetCursorPos`
  - `DirectInput8::GetDeviceState`
- **逻辑：** 在这些函数被调用时，记录输入状态并传给 ImGui。如果 ImGui 正在通过菜单交互（`io.WantCaptureMouse` 为 true），则在 Hook 中返回“无输入”给游戏引擎，从而实现“打开菜单时屏蔽游戏操作”的效果。

### 3. 使用 Raw Input (原始输入)

如果 `WndProc` 被屏蔽，Windows 的 Raw Input API 通常仍然有效。你可以注册一个 Raw Input 设备来监听鼠标和键盘。

- 这种方式比 `GetAsyncKeyState` 更灵敏，也不会受到某些应用层消息过滤的影响。
- 但是实现起来比直接轮询要复杂得多，你需要自己解析 HID 数据包。

### 4. 特别注意：反作弊检测风险

虽然上述方法解决了“输入”的技术问题，但你需要注意以下风险：

- **GetAsyncKeyState 滥用：** 频繁在循环中调用 `GetAsyncKeyState` 可能会被某些行为分析标记为异常（尽管主要是针对宏/脚本，UI 交互一般没事）。
- **鼠标移动模拟：** 如果你试图通过 ImGui 自动化游戏操作（如自动瞄准），修改 `ViewAngles` 比模拟鼠标移动（Mouse Event）要隐蔽得多。模拟鼠标消息（`mouse_event` 或 `SendInput`）通常会被 LLMHF (Low Level Mouse Hook Flag) 标记，EAC 会直接拦截没有硬件标志的输入。

### 总结建议

不要尝试去破解 EAC 对 `WndProc` 的保护（这很难且风险极高）。

**推荐方案：** 在你的渲染循环中，**直接修改 `ImGui::GetIO()` 的状态**。使用 `GetAsyncKeyState` 读取物理按键状态，使用 `GetCursorPos` 读取鼠标位置。这是驱动 ImGui 最底层、最直接的方式，完全绕过了 Windows 消息队列机制。