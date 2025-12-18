#include "Hook.h"
#include"../../base.h"
#include <iostream>
const char* EngineWindowName = "UnrealWindow";//窗口类名 根据引擎决定

void HookVTB(unsigned long long* swapvtable, int idx, void* new_add) {
    DWORD* Protect = new DWORD;
    VirtualProtect(swapvtable, sizeof(unsigned long long) * (idx + 1), PAGE_EXECUTE_READWRITE, Protect);
    swapvtable[idx] = (unsigned long long)new_add;
    VirtualProtect(swapvtable, sizeof(unsigned long long) * (idx + 1), *Protect, Protect);
    delete Protect;
};
enum VFuncIndex{
    ExecuteCommandLists = 10,
    DrawInstanced = 12,
    DrawIndexedInstanced = 13,
    Present=8,
    ResizeBuffers=13
  
	};

bool is_init = false;//判断是否需要进行初始化
bool is_init_hotkey = false;//是否要注册hot key

WNDCLASSEXA WindowClass;
HWND WindowHwnd;

bool InitWindow() {

    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = DefWindowProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = GetModuleHandle(NULL);
    WindowClass.hIcon = NULL;
    WindowClass.hCursor = NULL;
    WindowClass.hbrBackground = NULL;
    WindowClass.lpszMenuName = NULL;
    WindowClass.lpszClassName = "ST";
    WindowClass.hIconSm = NULL;
    RegisterClassExA(&WindowClass);
    WindowHwnd = CreateWindowA(WindowClass.lpszClassName, "STDXWindow", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, WindowClass.hInstance, NULL);
    if (WindowHwnd == NULL) {
        return false;
    }
    return true;
}

bool DeleteWindow() {
    DestroyWindow(WindowHwnd);
    UnregisterClassA(WindowClass.lpszClassName, WindowClass.hInstance);
    if (WindowHwnd != NULL) {
        return false;
    }
    return true;
}



bool DX12Hook::SetHookDx12(pPresent HookPresentFunc, pResizeBuffers HookResizeBuffersFunc, pDrawInstanced HookDrawInstanced, pDrawIndexedInstanced HookDrawIndexedInstanced, pExecuteCommandLists HookExecuteCommandLists) {
    if (ISHOOKDX12) return false;//防止二次hook把原函数搞不见
    GHwnd = FindWindowA(EngineWindowName, NULL);
	if (!GHwnd) return false;
    STdebug_printf("Found Window: %X\n", GHwnd);
        if(!InitWindow()) return false;
        
        HMODULE D3D12Module = GetModuleHandleA("d3d12.dll"); // 获取已加载的d3d12.dll模块句柄
        HMODULE DXGIModule = GetModuleHandleA("dxgi.dll"); // 获取已加载的dxgi.dll模块句柄
        if (D3D12Module == NULL || DXGIModule == NULL) { // 如果任一模块没有加载
            DeleteWindow();
            return false; // 返回false表示初始化失败
        }

        void* CreateDXGIFactory = GetProcAddress(DXGIModule, "CreateDXGIFactory"); // 获取CreateDXGIFactory函数地址
        if (CreateDXGIFactory == NULL) { // 如果获取失败
            DeleteWindow();
            return false; // 返回失败
        }

        IDXGIFactory* Factory; // 声明IDXGIFactory指针用于存放创建的Factory
        if (((long(__stdcall*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&Factory) < 0) { // 调用CreateDXGIFactory创建Factory实例，并判断返回值
            DeleteWindow();
            return false; // 返回失败
        }

        IDXGIAdapter* Adapter; // 声明IDXGIAdapter指针用于枚举适配器
        if (Factory->EnumAdapters(0, &Adapter) == DXGI_ERROR_NOT_FOUND) { // 枚举第一个适配器，若不存在则返回错误
            DeleteWindow();
            return false; // 返回失败
        }

        void* D3D12CreateDevice = GetProcAddress(D3D12Module, "D3D12CreateDevice"); // 获取D3D12CreateDevice函数地址
        if (D3D12CreateDevice == NULL) { // 如果未找到该函数
            DeleteWindow();
            return false; // 返回失败
        }

        STdebug_printf("D3D12CreateDevice Address: %p\n", D3D12CreateDevice);
        ID3D12Device* Device = nullptr;
         // 声明ID3D12Device指针用于接收创建的设备
        if (((long(__stdcall*)(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**))(D3D12CreateDevice))(Adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&Device) < 0) { // 调用D3D12CreateDevice以适配器创建设备并判断返回值
            DeleteWindow();
            return false; // 返回失败
        }

        

        STdebug_printf("D3D12 Device Address: %p\n", Device);
        D3D12_COMMAND_QUEUE_DESC QueueDesc; // 定义命令队列描述结构体
        QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // 设置命令队列类型为直接（Direct）
        QueueDesc.Priority = 0; // 使用默认优先级
        QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // 无特殊标志
        QueueDesc.NodeMask = 0; // 单节点（默认）


        ID3D12CommandQueue* CommandQueue; // 声明命令队列指针
        if (Device->CreateCommandQueue(&QueueDesc, __uuidof(ID3D12CommandQueue), (void**)&CommandQueue) < 0) { // 使用设备创建命令队列并检查返回值
            DeleteWindow();
            return false; // 返回失败
        }

        ID3D12CommandAllocator* CommandAllocator; // 声明命令分配器指针
        if (Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&CommandAllocator) < 0) { // 创建命令分配器并检查
            DeleteWindow();
            return false; // 返回失败
        }
         
        ID3D12GraphicsCommandList* CommandList; // 声明图形命令列表指针
        if (Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&CommandList) < 0) { // 创建图形命令列表并检查返回值
            DeleteWindow();
            return false; // 返回失败
        }
        
        DXGI_RATIONAL RefreshRate; // 定义刷新率结构体
		RefreshRate.Numerator = 60; // 分子设为60（60Hz）
		RefreshRate.Denominator = 1; // 分母设为1

		DXGI_MODE_DESC BufferDesc; // 定义缓冲区模式描述
		BufferDesc.Width = 100; // 缓冲区宽度设为100像素（临时占位）
		BufferDesc.Height = 100; // 缓冲区高度设为100像素（临时占位）
		BufferDesc.RefreshRate = RefreshRate; // 使用上面定义的刷新率
		BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 颜色格式设为常用的RGBA 8位无符号标准化格式
		BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; // 扫描线顺序未指定
		BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // 缩放未指定

		DXGI_SAMPLE_DESC SampleDesc; // 定义采样描述结构体
		SampleDesc.Count = 1; // 采样计数设为1（无MSAA）
		SampleDesc.Quality = 0; // 采样质量设为0

		DXGI_SWAP_CHAIN_DESC SwapChainDesc = {}; // 使用聚合初始化交换链描述为零
		SwapChainDesc.BufferDesc = BufferDesc; // 设置缓冲区描述
		SwapChainDesc.SampleDesc = SampleDesc; // 设置采样描述
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 缓冲区用途为渲染目标输出
		SwapChainDesc.BufferCount = 2; // 双缓冲
		SwapChainDesc.OutputWindow = WindowHwnd; // 输出到之前创建的临时窗口
		SwapChainDesc.Windowed = 1; // 以窗口模式运行
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 交换效果使用Flip Discard
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // 允许模式切换标志

		IDXGISwapChain* SwapChain; // 声明交换链指针

        if (Factory->CreateSwapChain(CommandQueue, &SwapChainDesc, &SwapChain) < 0) { // 使用Factory和命令队列创建交换链并检查返回值
            DeleteWindow();
            return false; // 返回失败
        }
       

      GDeviceVTable = *(UINT_PTR**)Device;//获取虚表地址
      GCommandQueueVTable = *(UINT_PTR**)CommandQueue;//获取虚表地址
      GCommandAllocatorVTable = *(UINT_PTR**)CommandAllocator;//获取虚表地址
      GCommandListVTable = *(UINT_PTR**)CommandList;//获取虚表地址
      GSwapChainVTable = *(UINT_PTR**)SwapChain;//获取虚表地址

    OriginalPresent = (pPresent)GSwapChainVTable[VFuncIndex::Present];//第八个位置
    OriginalResizeBuffers = (pResizeBuffers)GSwapChainVTable[VFuncIndex::ResizeBuffers];//第13个位置

    oDrawInstanced = (pDrawInstanced)GCommandListVTable[VFuncIndex::DrawInstanced];//第12个位置
    oDrawIndexedInstanced = (pDrawIndexedInstanced)GCommandListVTable[VFuncIndex::DrawIndexedInstanced];//第13个位置

    oExecuteCommandLists = (pExecuteCommandLists)GCommandQueueVTable[VFuncIndex::ExecuteCommandLists];//第10个位置
    if (HookPresentFunc != 0 and HookResizeBuffersFunc != 0)
    {
        STdebug_printf("[D3D12Hook] DX12 Hooking...\n");
        STdebug_printf("[D3D12Hook] DX12 Hooking Present\n");
        
        HookVTB(GSwapChainVTable, VFuncIndex::ResizeBuffers, HookResizeBuffersFunc);
        STdebug_printf("[D3D12Hook] DX12 Hooking HookDrawInstanced\n");
        HookVTB(GSwapChainVTable, VFuncIndex::Present, HookPresentFunc);
        
        STdebug_printf("[D3D12Hook] DX12 Hooking ResizeBuffers\n");

        HookVTB(GCommandListVTable, VFuncIndex::DrawInstanced, HookDrawInstanced);
        STdebug_printf("[D3D12Hook] DX12 Hooking DrawIndexedInstanced\n");
        HookVTB(GCommandListVTable, VFuncIndex::DrawIndexedInstanced, HookDrawIndexedInstanced);
        STdebug_printf("[D3D12Hook] DX12 Hooking ExecuteCommandLists\n");
        HookVTB(GCommandQueueVTable, VFuncIndex::ExecuteCommandLists, HookExecuteCommandLists);
        Device->Release();
        CommandQueue->Release();
        CommandAllocator->Release();
        CommandList->Release();
        SwapChain->Release();//对交换链进行释放 后续不需要使用了

        STdebug_printf("[D3D12Hook] DX12 Hooking Success!\n");
        ISHOOKDX12 = TRUE;
    }
    DeleteWindow();
    return true;

};

void DX12Hook::SetWndprocHook(WNDPROC WndProc)
{
    if (!OriginalWndProc) OriginalWndProc = (WNDPROC)SetWindowLongPtrA(GHwnd, GWLP_WNDPROC, (LONG_PTR)WndProc); 
    else (WNDPROC)SetWindowLongPtrA(GHwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
};
void DX12Hook::RemoveWndProcHook()
{
    if (!OriginalWndProc) return;
    OriginalWndProc = (WNDPROC)SetWindowLongPtr(GHwnd, GWLP_WNDPROC, (LONG_PTR)OriginalWndProc);
};

void DX12Hook::RemoveHookDx12()
{
    if (ISHOOKDX12)
    {
        ISHOOKDX12 = 0;
        if (OriginalPresent != 0 and OriginalResizeBuffers != 0 and GSwapChainVTable != 0 and oDrawIndexedInstanced !=0 and oDrawInstanced !=0 and oExecuteCommandLists!=0)
        {
            HookVTB(GSwapChainVTable, VFuncIndex::Present, OriginalPresent);
            HookVTB(GSwapChainVTable, VFuncIndex::ResizeBuffers, OriginalResizeBuffers);
            HookVTB(GCommandListVTable, VFuncIndex::DrawIndexedInstanced, oDrawIndexedInstanced);
            HookVTB(GCommandListVTable, VFuncIndex::DrawInstanced, oDrawInstanced);
            HookVTB(GCommandQueueVTable, VFuncIndex::ExecuteCommandLists, oExecuteCommandLists);
        }
    }
}


//ExecuteCommandLists = 10, CommandQueue
//DrawInstanced = 84, 12 CommandList
//DrawIndexedInstanced = 85, 13 CommandList
//Present = 140, 8 SwapChain
//ResizeBuffers = 145, 13 SwapChain
//Device 44 个 
//CommandQueue 19 个
//CommandAllocator 9 个
//CommandList 60 个
//SwapChain 18 
//D3D12 Methods Table:
//[0]   QueryInterface
//[1]   AddRef
//[2]   Release
//[3]   GetPrivateData
//[4]   SetPrivateData
//[5]   SetPrivateDataInterface
//[6]   SetName
//[7]   GetNodeCount
//[8]   CreateCommandQueue
//[9]   CreateCommandAllocator
//[10]  CreateGraphicsPipelineState
//[11]  CreateComputePipelineState
//[12]  CreateCommandList
//[13]  CheckFeatureSupport
//[14]  CreateDescriptorHeap
//[15]  GetDescriptorHandleIncrementSize
//[16]  CreateRootSignature
//[17]  CreateConstantBufferView
//[18]  CreateShaderResourceView
//[19]  CreateUnorderedAccessView
//[20]  CreateRenderTargetView
//[21]  CreateDepthStencilView
//[22]  CreateSampler
//[23]  CopyDescriptors
//[24]  CopyDescriptorsSimple
//[25]  GetResourceAllocationInfo
//[26]  GetCustomHeapProperties
//[27]  CreateCommittedResource
//[28]  CreateHeap
//[29]  CreatePlacedResource
//[30]  CreateReservedResource
//[31]  CreateSharedHandle
//[32]  OpenSharedHandle
//[33]  OpenSharedHandleByName
//[34]  MakeResident
//[35]  Evict
//[36]  CreateFence
//[37]  GetDeviceRemovedReason
//[38]  GetCopyableFootprints
//[39]  CreateQueryHeap
//[40]  SetStablePowerState
//[41]  CreateCommandSignature
//[42]  GetResourceTiling
//[43]  GetAdapterLuid
//[44]  QueryInterface
//[45]  AddRef
//[46]  Release
//[47]  GetPrivateData
//[48]  SetPrivateData
//[49]  SetPrivateDataInterface
//[50]  SetName
//[51]  GetDevice
//[52]  UpdateTileMappings
//[53]  CopyTileMappings
//[54]  ExecuteCommandLists
//[55]  SetMarker
//[56]  BeginEvent
//[57]  EndEvent
//[58]  Signal
//[59]  Wait
//[60]  GetTimestampFrequency
//[61]  GetClockCalibration
//[62]  GetDesc
//[63]  QueryInterface
//[64]  AddRef
//[65]  Release
//[66]  GetPrivateData
//[67]  SetPrivateData
//[68]  SetPrivateDataInterface
//[69]  SetName
//[70]  GetDevice
//[71]  Reset
//[72]  QueryInterface
//[73]  AddRef
//[74]  Release
//[75]  GetPrivateData
//[76]  SetPrivateData
//[77]  SetPrivateDataInterface
//[78]  SetName
//[79]  GetDevice
//[80]  GetType
//[81]  Close
//[82]  Reset
//[83]  ClearState
//[84]  DrawInstanced
//[85]  DrawIndexedInstanced
//[86]  Dispatch
//[87]  CopyBufferRegion
//[88]  CopyTextureRegion
//[89]  CopyResource
//[90]  CopyTiles
//[91]  ResolveSubresource
//[92]  IASetPrimitiveTopology
//[93]  RSSetViewports
//[94]  RSSetScissorRects
//[95]  OMSetBlendFactor
//[96]  OMSetStencilRef
//[97]  SetPipelineState
//[98]  ResourceBarrier
//[99]  ExecuteBundle
//[100] SetDescriptorHeaps
//[101] SetComputeRootSignature
//[102] SetGraphicsRootSignature
//[103] SetComputeRootDescriptorTable
//[104] SetGraphicsRootDescriptorTable
//[105] SetComputeRoot32BitConstant
//[106] SetGraphicsRoot32BitConstant
//[107] SetComputeRoot32BitConstants
//[108] SetGraphicsRoot32BitConstants
//[109] SetComputeRootConstantBufferView
//[110] SetGraphicsRootConstantBufferView
//[111] SetComputeRootShaderResourceView
//[112] SetGraphicsRootShaderResourceView
//[113] SetComputeRootUnorderedAccessView
//[114] SetGraphicsRootUnorderedAccessView
//[115] IASetIndexBuffer
//[116] IASetVertexBuffers
//[117] SOSetTargets
//[118] OMSetRenderTargets
//[119] ClearDepthStencilView
//[120] ClearRenderTargetView
//[121] ClearUnorderedAccessViewUint
//[122] ClearUnorderedAccessViewFloat
//[123] DiscardResource
//[124] BeginQuery
//[125] EndQuery
//[126] ResolveQueryData
//[127] SetPredication
//[128] SetMarker
//[129] BeginEvent
//[130] EndEvent
//[131] ExecuteIndirect
//[132] QueryInterface
//[133] AddRef
//[134] Release
//[135] SetPrivateData
//[136] SetPrivateDataInterface
//[137] GetPrivateData
//[138] GetParent
//[139] GetDevice
//[140] Present
//[141] GetBuffer
//[142] SetFullscreenState
//[143] GetFullscreenState
//[144] GetDesc
//[145] ResizeBuffers
//[146] ResizeTarget
//[147] GetContainingOutput
//[148] GetFrameStatistics
//[149] GetLastPresentCount