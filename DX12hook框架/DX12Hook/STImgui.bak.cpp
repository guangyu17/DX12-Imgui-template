#include"STImgui.h"

#include <d3d12.h>
#include <dxgi1_5.h>



// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
FrameContext* WaitForNextFrameContext();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);











bool STIMGUI::InitImgui(IDXGISwapChain3* pSwapChain,HWND GHwnd)
{
   

    if (!isDX12Init) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&gDevice))) {
            

            ImGui::CreateContext();//创建环境
            if (!isDX12reInit)//上面是d3d初始化 这里面是imgui初始化
            {
                ImGui::StyleColorsLight();
                if (!ImGui_ImplWin32_Init(GHwnd)) return false;
                io = &ImGui::GetIO();
                //加载io设置字体
                io->IniFilename = nullptr;//保存配置文件 null为不保存
                io->LogFilename = nullptr;//保存log文件 null为不保存
                /*io->Fonts->AddFontDefault();*/
                io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 18.f, NULL, io->Fonts->GetGlyphRangesChineseFull());//使其支持中文
                ImguiDrawFunc::Setstyle();
                isDX12reInit = true;//这部分在d3d重新设置时不需要再初始化
            }
            STdebug_printf("[IMGUI device building] DX12 ImGui Init Success!\n");
            
            DXGI_SWAP_CHAIN_DESC Desc;
            pSwapChain->GetDesc(&Desc);
            Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            Desc.OutputWindow = GHwnd;
            Desc.Windowed = ((GetWindowLongPtr(GHwnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

            gBuffersCounts = Desc.BufferCount;
            gFrameContext = new _FrameContext[gBuffersCounts];

            D3D12_DESCRIPTOR_HEAP_DESC DescriptorImGuiRender = {};
            DescriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            DescriptorImGuiRender.NumDescriptors = gBuffersCounts;
            DescriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            if (gDevice->CreateDescriptorHeap(&DescriptorImGuiRender, IID_PPV_ARGS(&gDescriptorHeapImGuiRender)) != S_OK)
                return false;
            STdebug_printf("[IMGUI device building]     DX12 CreateDescriptorHeap Success!\n");

            if (gDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gFence)) != S_OK)
                return false;
            STdebug_printf("[IMGUI device building]     DX12 gFence Success!\n");

            gFenceEvent = CreateEventA(
                nullptr, // 默认安全属性
                FALSE,   // 自动重置（Auto-reset）。当 SetEventOnCompletion 触发后，会自动恢复到非信号状态。
                FALSE,   // 初始状态为非信号 (Non-signaled)
                ""  // 无名称
            );
            /*gFenceValue = gFence->GetCompletedValue();*/

            for (UINT i = 0; i < gBuffersCounts; i++)
                if (gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gFrameContext[i].CommandAllocator)) != S_OK)
                    return false;

            STdebug_printf("[IMGUI device building]     DX12 CreateCommandAllocator Success!\n");
            

            if (gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gFrameContext[0].CommandAllocator, NULL, IID_PPV_ARGS(&gCommandList)) != S_OK ||gCommandList->Close() != S_OK)
                return false;
            STdebug_printf("[IMGUI device building]     DX12 CreateCommandList Success!\n");
            D3D12_DESCRIPTOR_HEAP_DESC DescriptorBackBuffers;
            DescriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            DescriptorBackBuffers.NumDescriptors = gBuffersCounts;
            DescriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            DescriptorBackBuffers.NodeMask = 1;

            if (gDevice->CreateDescriptorHeap(&DescriptorBackBuffers, IID_PPV_ARGS(&gDescriptorHeapBackBuffers)) != S_OK)
                return false;
            STdebug_printf("[IMGUI device building]     DX12 CreateDescriptorHeap Success!\n");
            const auto RTVDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle =gDescriptorHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();

            for (size_t i = 0; i < gBuffersCounts; i++) {
                ID3D12Resource* pBackBuffer = nullptr;
                gFrameContext[i].DescriptorHandle = RTVHandle;
                pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
                gDevice->CreateRenderTargetView(pBackBuffer, nullptr, RTVHandle);
                STdebug_printf("[IMGUI device building]         DX12 CreateRenderTargetView %d \n",i);
                gFrameContext[i].Resource = pBackBuffer;
                RTVHandle.ptr += RTVDescriptorSize;
            }
            STdebug_printf("[IMGUI device building]     DX12 CreateRenderTargetView Success!\n");

            g_hSwapChainWaitableObject = pSwapChain->GetFrameLatencyWaitableObject();
            {
                
            


            g_pd3dSrvDescHeapAlloc.Create(gDevice, gDescriptorHeapImGuiRender);
            ImGui_ImplDX12_InitInfo init_info = {};
            init_info.Device = gDevice;
            init_info.CommandQueue = gCommandQueue;
            init_info.NumFramesInFlight = gBuffersCounts;
            init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
            init_info.SrvDescriptorHeap = gDescriptorHeapImGuiRender;
            init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle,out_gpu_handle); };
            init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
            if(!ImGui_ImplDX12_Init(&init_info)) return false;
            STdebug_printf("[IMGUI device building]     DX12 ImGui_ImplDX12_Init Success!\n");
            if(!ImGui_ImplDX12_CreateDeviceObjects()) return false;
            STdebug_printf("[IMGUI device building]     DX12 ImGui_ImplDX12_CreateDeviceObjects Success!\n");
            isDX12Init = true;
            }
            
        }
        
    }
    STdebug_printf("[IMGUI device building finished]\n");
    return isDX12Init;
}
// 假设这是您的同步函数，用于等待 GPU 完成所有命令
// 您需要在 InitImgui 之外定义它




void STIMGUI::ResizeImgui(IDXGISwapChain3* pSwapChain)
{
    
    if (!isDX12Init)
        return;
    ImGui_ImplWin32_Shutdown();
    ImGui_ImplDX12_Shutdown();
    ImGui::DestroyContext();
    // 4. 释放所有 FrameContext 资源 (Back Buffers)
    for (size_t i = 0; i < gBuffersCounts; i++) {
        if (gFrameContext[i].Resource) {
            gFrameContext[i].Resource->Release();
            gFrameContext[i].Resource = nullptr;
        }
        // 命令分配器 CommandAllocator 通常不需要释放，但如果在 InitImgui 中分配了新的，这里也应该清理
        if (gFrameContext[i].CommandAllocator) {
            // gFrameContext[i].CommandAllocator->Release(); // 取决于您 InitImgui 中的分配方式
        }
    }

    // 5. 释放 FrameContext 数组本身
    if (gFrameContext) {
        delete[] gFrameContext;
        gFrameContext = nullptr;
    }
    // 6. 释放 DX12 资源
    if (gCommandList) gCommandList->Release();
    if (gDescriptorHeapImGuiRender) gDescriptorHeapImGuiRender->Release(); // ImGui SRV 堆
    if (gDescriptorHeapBackBuffers) gDescriptorHeapBackBuffers->Release(); // RTV 堆

    // 7. 释放您自己的同步资源 (如果它们是在 Hook 成功后创建的)
    // if (gFence) gFence->Release(); 
    // if (gFenceEvent) CloseHandle(gFenceEvent); 

    // 8. 重置 ImGui 描述符分配器追踪 (如果您使用了自定义分配器)
    // g_pd3dSrvDescHeapAlloc.Destroy(); 

    // 9. 重置状态标志
    gCommandList = nullptr;
    gDescriptorHeapImGuiRender = nullptr;
    gDescriptorHeapBackBuffers = nullptr;
    gBuffersCounts = 0;

    isDX12Init = false;
    isDX12reInit = false; // 如果您想在重置后重新加载字体，则重置此标志
}










void ExampleDescriptorHeapAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
{
    int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
    int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
    IM_ASSERT(cpu_idx == gpu_idx);
    FreeIndices.push_back(cpu_idx);
}
void ExampleDescriptorHeapAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
    IM_ASSERT(FreeIndices.Size > 0);
    int idx = FreeIndices.back();
    FreeIndices.pop_back();
    out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
    out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
}
void ExampleDescriptorHeapAllocator::Destroy()
{
    Heap = nullptr;
    FreeIndices.clear();
}
void ExampleDescriptorHeapAllocator::Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
{
    IM_ASSERT(Heap == nullptr && FreeIndices.empty());
    Heap = heap;
    D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
    HeapType = desc.Type;
    HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
    HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
    HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
    FreeIndices.reserve((int)desc.NumDescriptors);
    for (int n = desc.NumDescriptors; n > 0; n--)
        FreeIndices.push_back(n - 1);
}


void STIMGUI::WaitForPendingOperations()
{
    gCommandQueue->Signal(gFence, ++gfenceLastSignaledValue);

    gFence->SetEventOnCompletion(gfenceLastSignaledValue, gFenceEvent);
    ::WaitForSingleObject(gFenceEvent, INFINITE);
}

STIMGUI::_FrameContext* STIMGUI::WaitForNextFrameContext()
{
    _FrameContext* frame_context = &gFrameContext[g_frameIndex % gBuffersCounts];
    if (gFence->GetCompletedValue() < frame_context->FenceValue)
    {
        gFence->SetEventOnCompletion(frame_context->FenceValue, gFenceEvent);
        HANDLE waitableObjects[] = { gFenceEvent, gFenceEvent };
        ::WaitForMultipleObjects(2, waitableObjects, TRUE, INFINITE);
    }
    else
        ::WaitForSingleObject(g_hSwapChainWaitableObject, INFINITE);

    return frame_context;
}









void STIMGUI::ImGuiFinal(IDXGISwapChain3* pSwapChain) {


        ImGui::Render();
        _FrameContext* frameCtx = WaitForNextFrameContext();
        STIMGUI::_FrameContext& CurrentFrameContext = gFrameContext[((IDXGISwapChain3*)pSwapChain)->GetCurrentBackBufferIndex()];
        
        CurrentFrameContext.CommandAllocator->Reset();
        D3D12_RESOURCE_BARRIER Barrier;
        Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        Barrier.Transition.pResource = CurrentFrameContext.Resource;
        Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        gCommandList->Reset(CurrentFrameContext.CommandAllocator, nullptr);
        gCommandList->ResourceBarrier(1, &Barrier);

        STdebug_printf("[ImGuiFinal] Barrier Finished\n");

        gCommandList->OMSetRenderTargets(1, &CurrentFrameContext.DescriptorHandle, FALSE, nullptr);
        gCommandList->SetDescriptorHeaps(1, &gDescriptorHeapImGuiRender);

       
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), gCommandList);
        STdebug_printf("[ImGuiFinal] ImGui_ImplDX12_RenderDrawData Finished\n");
        Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        gCommandList->ResourceBarrier(1, &Barrier);
        gCommandList->Close();
        gCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&gCommandList));

        gCommandQueue->Signal(gFence, ++gfenceLastSignaledValue);
        frameCtx->FenceValue = gfenceLastSignaledValue;
        STdebug_printf("[ImGuiFinal] Finished\n");

        g_frameIndex++;
};