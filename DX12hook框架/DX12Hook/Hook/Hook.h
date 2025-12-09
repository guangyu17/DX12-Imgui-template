#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <Windows.h>
#pragma comment(lib,"d3d12.lib")
typedef HRESULT(*pPresent)(IDXGISwapChain3* gpSwapChain, UINT SyncInterval, UINT Flags);//present函数类型
typedef HRESULT(*pResizeBuffers)(IDXGISwapChain3* gpSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);//重设置窗口尺寸函数类型
typedef void(APIENTRY* pDrawInstanced)(ID3D12GraphicsCommandList* dCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
typedef void(APIENTRY* pDrawIndexedInstanced)(ID3D12GraphicsCommandList* dCommandList, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
typedef void(APIENTRY* pExecuteCommandLists)(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists);





class DX12Hook {

public:
    //Hook
    UINT_PTR* GSwapChainVTable = nullptr;//虚表函数数组
    UINT_PTR* GDeviceVTable = nullptr;//虚表函数数组
    UINT_PTR* GCommandQueueVTable = nullptr;//虚表函数数组
    UINT_PTR* GCommandAllocatorVTable = nullptr;//虚表函数数组
    UINT_PTR* GCommandListVTable = nullptr;//虚表函数数组

    pPresent OriginalPresent = nullptr;//旧的Present函数
    pResizeBuffers OriginalResizeBuffers = nullptr;//旧的resizebuffers函数
    WNDPROC OriginalWndProc = nullptr;//窗口消息函数
    pDrawInstanced oDrawInstanced = NULL;
    pDrawIndexedInstanced oDrawIndexedInstanced = NULL;
    pExecuteCommandLists oExecuteCommandLists = NULL;

    HWND GHwnd = NULL;

    bool SetHookDx12(pPresent HookPresentFunc, pResizeBuffers HookResizeBuffersFunc , pDrawInstanced HookDrawInstanced, pDrawIndexedInstanced HookDrawIndexedInstanced, pExecuteCommandLists HookExecuteCommandLists);//设置挂钩
    void RemoveHookDx12();
 
    void SetWndprocHook(WNDPROC WndProc);
    void RemoveWndProcHook();

    //初始化相关
    bool ISHOOKDX12 = false;
};

