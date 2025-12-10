// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include"base.h"
#include "DX12Hook/STImgui.h"

bool ShowMenu = false;
bool ImGui_Initialised = false;



HRESULT Present(IDXGISwapChain3* gpSwapChain, UINT SyncInterval, UINT Flags);//我们自身的present函数
HRESULT ResizeBuffers(IDXGISwapChain3* gpSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);//我们自身的重设置窗口分辨率函数
LRESULT WINAPI HookWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);//hook原窗口函数
void hkExecuteCommandLists(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists);
void APIENTRY hkDrawInstanced(ID3D12GraphicsCommandList* dCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
void APIENTRY hkDrawIndexedInstanced(ID3D12GraphicsCommandList* dCommandList, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);

STIMGUI Gimgui;
DX12Hook Dx12Hook;
namespace ImguiParm
{
    bool main_open = true;
    ImColor color(255, 220, 255, 255);
    ImVec2 start = { 0,0 };
    ImVec2 end = { 0,0 };
    float width = 0.0f;

}
void MainThread()
{
    Dx12Hook.SetHookDx12(Present,ResizeBuffers,hkDrawInstanced, hkDrawIndexedInstanced, hkExecuteCommandLists);
}



BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
         if(Config::GetIsDebug())
         {
             AllocConsole();
             FILE* FILE = NULL;
             freopen_s(&FILE, "CONOUT$", "w+t", stdout);
             STdebug_printf("start debug\n");
         }
        HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainThread, NULL, 0, NULL);


    }
    case DLL_PROCESS_DETACH: {

        Dx12Hook.RemoveHookDx12();
        Dx12Hook.RemoveWndProcHook();
        Gimgui.release();
        break;
    }
    break;
    }
    return true;
}



HRESULT Present(IDXGISwapChain3* gpSwapChain, UINT SyncInterval, UINT Flags)
{
   
    if (Config::GetIsImgui() and Dx12Hook.ISHOOKDX12)
    {
        if (!Gimgui.isDX12Init) {
            STdebug_printf("[HkPresent] Init ImGui!\n");
            Dx12Hook.SetWndprocHook(HookWndProc);
            if (!Gimgui.InitImgui(gpSwapChain, Dx12Hook.GHwnd)) {
               /* Dx12Hook.RemoveWndProcHook();*/
                STdebug_printf("[HkPresent] Init failed\n");
                goto return_gamePresent;
            }
        }

        if (Gimgui.MenuOpen) {
            // 菜单打开时：让 ImGui 画一个光标，或者允许显示系统光标
            Gimgui.io->MouseDrawCursor = true;
            Gimgui.io->ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
            // 或者使用 Gimgui.io->ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange; 来允许 ImGui 切换系统光标
        }
        else {
            // 菜单关闭时：隐藏 ImGui 光标
            Gimgui.io->ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            Gimgui.io->MouseDrawCursor = false;
        }
        


        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin(u8"ST", &Gimgui.MenuOpen, ImGuiWindowFlags_AlwaysAutoResize);
            if (Gimgui.MenuOpen) {
                ImGui::Text(u8"测试");
                ImGui::Text(u8"fps :%0.1f", Gimgui.io->Framerate);
                
            }
            ImGui::End(); //imgui菜单尾

            if(Gimgui.MenuOpen)
            {
                ImGui::ShowDemoWindow();
            }
        }

        
        Gimgui.ImGuiFinal(gpSwapChain);
    }
return_gamePresent:
    return Dx12Hook.OriginalPresent(gpSwapChain, SyncInterval, Flags);
};




HRESULT ResizeBuffers(IDXGISwapChain3* gpSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    STdebug_printf("[ResizeBuffers Hook]     Game ResizeBuffers\n");
    Gimgui.ResizeImgui(gpSwapChain);
    
    return Dx12Hook.OriginalResizeBuffers(gpSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
};


LRESULT WINAPI HookWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    
    switch (msg)
    {
    case WM_KEYDOWN: {
        //STdebug_printf("[Hkwndproc message]         message: 0X%X\n", wParam);
        if (wParam == VK_INSERT) {
            if (Gimgui.MenuOpen) {
                Gimgui.MenuOpen = 0;
            }
            else Gimgui.MenuOpen = 1;
        }
        break;

        //if (wParam == 't' || wParam == 'T') {
        //    ImguiParm::is_aim_bot = true;

        //}//长按开启aimbot 功能测试
    }
    case WM_KEYUP: {
        //if (wParam == 't' || wParam == 'T') {
        //    ImguiParm::is_aim_bot = false;

        //}//长按开启aimbot 功能测试
        break;
    }
    case WM_SETCURSOR: {
        if (!Gimgui.MenuOpen) {
            SetCursor(NULL);
            //如果菜单打开，阻止游戏设置光标
            return TRUE;
        }
        else {
            
        }
        break;
    }
    default:
        break;
    }

    //热键处理 hotkey是自己定义的热键标识 这样一来自己定义的热键处理 其他消息 包括其他热键都默认处理
    //这个热键是全局热键 在窗口不激活的时候也生效 所以一定要记得注销
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))return true;
    if (Gimgui.io->WantCaptureMouse == true && (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_MBUTTONUP || msg == WM_RBUTTONUP || msg == WM_LBUTTONDBLCLK || msg == WM_MBUTTONDBLCLK || msg == WM_RBUTTONDBLCLK))return true;
    //静止鼠标穿透 同理可以写禁止键盘穿透

    return CallWindowProc(Dx12Hook.OriginalWndProc, hWnd, msg, wParam, lParam);//执行本身的消息函数

};



void hkExecuteCommandLists(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists) {

    //获取实际使用的CommandQueue和Device 用于imgui初始化
    if (!Gimgui.gCommandQueue) {
        ID3D12Device* queueDevice = nullptr;
        if (SUCCEEDED(queue->GetDevice(__uuidof(ID3D12Device), (void**)&queueDevice))) {
            /*if (!Gimgui.gDevice) {
                Gimgui.gDevice = queueDevice;
            }*/
            STdebug_printf("[hkExecuteCommandLists] Captured queueDevice=%p\n", queueDevice);
            if (queueDevice == Gimgui.gDevice) {
                D3D12_COMMAND_QUEUE_DESC desc = queue->GetDesc();
                STdebug_printf("[hkExecuteCommandLists] CommandQueue type=%u\n", desc.Type);
                if (desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
                    queue->AddRef();
                    STdebug_printf("[hkExecuteCommandLists] Captured CommandQueue=%p\n", queue);
                    Gimgui.gCommandQueue = queue;
                }
                else {
                    STdebug_printf("[hkExecuteCommandLists] Skipping capture: non-direct queue\n");
                }
            }
            else {
                STdebug_printf("[hkExecuteCommandLists] Skipping capture: CommandQueue uses different device (%p != %p)\n", queueDevice, Gimgui.gDevice);
            }
        }

    
    }

    Dx12Hook.oExecuteCommandLists(queue, NumCommandLists, ppCommandLists);
}

void APIENTRY hkDrawInstanced(ID3D12GraphicsCommandList* dCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation) {

    return Dx12Hook.oDrawInstanced(dCommandList, VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}


void APIENTRY hkDrawIndexedInstanced(ID3D12GraphicsCommandList* dCommandList, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) {

    /*
    //cyberpunk 2077 no pants hack (low settings)
    if (nopants_enabled)
        if (IndexCountPerInstance == 10068 || //bargirl pants near
            IndexCountPerInstance == 3576) //med range
            return; //delete texture

    if (GetAsyncKeyState(VK_F12) & 1) //toggle key
        nopants_enabled = !nopants_enabled;


    //logger, hold down B key until a texture disappears, press END to log values of those textures
    if (GetAsyncKeyState('V') & 1) //-
        countnum--;
    if (GetAsyncKeyState('B') & 1) //+
        countnum++;
    if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState('9') & 1) //reset, set to -1
        countnum = -1;

    if (countnum == IndexCountPerInstance / 100)
        if (GetAsyncKeyState(VK_END) & 1) //log
            Log("IndexCountPerInstance == %d && InstanceCount == %d",
                IndexCountPerInstance, InstanceCount);

    if (countnum == IndexCountPerInstance / 100)
        return;
    */

    return Dx12Hook.oDrawIndexedInstanced(dCommandList, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}