#pragma once
#include"../base.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);//IMGUI自己的消息处理





//struct FrameContext
//{
//    ID3D12CommandAllocator* CommandAllocator;
//    UINT64                      FenceValue;
//};
// Simple free list based allocator
struct ExampleDescriptorHeapAllocator
{
    ID3D12DescriptorHeap* Heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT                        HeapHandleIncrement;
    ImVector<int>               FreeIndices;

	void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap);
	void Destroy();
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle);
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle);
};

static ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;



class STIMGUI
{
public:
	bool InitImgui(IDXGISwapChain3* SwapChain, HWND GHwnd);
	void ResizeImgui(IDXGISwapChain3* pSwapChain);
	void ImGuiFinal(IDXGISwapChain3* pSwapChain);
    void release();
    

	// Data
	
	ID3D12Device* gDevice = nullptr;
    ID3D12DescriptorHeap* g_pd3dRtvDescHeap;//gDescriptorHeapBackBuffers g_pd3dRtvDescHeap
    ID3D12DescriptorHeap* g_pd3dSrvDescHeap;//gDescriptorHeapImGuiRender g_pd3dSrvDescHeap
    ID3D12GraphicsCommandList* gCommandList;
    ID3D12CommandQueue* gCommandQueue;
    UINT64 gfenceLastSignaledValue = 0;
    ID3D12Resource** g_mainRenderTargetResource;
    D3D12_CPU_DESCRIPTOR_HANDLE*  g_mainRenderTargetDescriptor;



    struct _FrameContext {
        ID3D12CommandAllocator* CommandAllocator;
        UINT64 FenceValue;
    };

    UINT_PTR gBuffersCounts = -1;
    _FrameContext* gFrameContext;

    ID3D12Fence* gFence = nullptr;
    HANDLE gFenceEvent = nullptr;
    UINT64 gFenceValue = 0; // 用于追踪的栅栏值
    //UINT64 gFenceLastValue = 0; // 用于追踪的栅栏值
	 ImGuiIO* io = nullptr;
	 bool isDX12Init = false;//判断是否需要进行初始化

	 bool isDX12reInit = false; //resize后重新初始化
public:
	bool MenuOpen = 1;


private:
    int g_frameIndex = 0;
    _FrameContext* WaitForNextFrameContext();
    void WaitForPendingOperations();
};
