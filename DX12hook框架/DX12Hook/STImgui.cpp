#include"STImgui.h"

#include <d3d12.h>
#include <dxgi1_5.h>
inline void LogHRESULT(const char* label, HRESULT hr) {
    STdebug_printf("[d3d12hook] %s: hr=0x%08X\n", label, hr);
}

bool STIMGUI::InitImgui(IDXGISwapChain3* pSwapChain,HWND GHwnd)
{
   

    if (!isDX12Init) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&gDevice))) {
            if (gCommandQueue == nullptr)
                return false;

            if (!isDX12reInit)//上面是d3d初始化 这里面是imgui初始化
            {   
                ImGui::CreateContext();//创建环境
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
                STdebug_printf("[IMGUI device building]     DX12 ImGui Init Success!\n");
            }
            
            
            

            DXGI_SWAP_CHAIN_DESC Desc;
            pSwapChain->GetDesc(&Desc);
            Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            Desc.OutputWindow = GHwnd;
            Desc.Windowed = ((GetWindowLongPtr(GHwnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;
            gBuffersCounts = Desc.BufferCount;
            gFrameContext = new _FrameContext[gBuffersCounts];
            g_mainRenderTargetResource = new ID3D12Resource * [gBuffersCounts];
            g_mainRenderTargetDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[gBuffersCounts];


            {
                D3D12_DESCRIPTOR_HEAP_DESC DescriptorImGuiRender = {};
                DescriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                DescriptorImGuiRender.NumDescriptors = gBuffersCounts;
                DescriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                if (gDevice->CreateDescriptorHeap(&DescriptorImGuiRender, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
                    return false;
                g_pd3dSrvDescHeapAlloc.Create(gDevice, g_pd3dSrvDescHeap);
                STdebug_printf("[IMGUI device building]     DX12 CreateDescriptorHeap Success!\n");
            }

            {
                if (gDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gFence)) != S_OK)
                    return false;
                STdebug_printf("[IMGUI device building]     DX12 gFence Success!\n");
                gFenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
                if (gFenceEvent == nullptr)
                    return false;
            }


            for (UINT i = 0; i < gBuffersCounts; i++)
                if (gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gFrameContext[i].CommandAllocator)) != S_OK)
                    return false;
            STdebug_printf("[IMGUI device building]     DX12 CreateCommandAllocator Success!\n");

            if (gDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gFrameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&gCommandList)) != S_OK ||
                gCommandList->Close() != S_OK)
                return false;
            STdebug_printf("[IMGUI device building]     DX12 CreateCommandList Success!\n");


            {
                D3D12_DESCRIPTOR_HEAP_DESC desc = {};
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                desc.NumDescriptors = gBuffersCounts;
                desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                desc.NodeMask = 1;
                if (gDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
                    return false;
                SIZE_T rtvDescriptorSize = gDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
                
                for (UINT i = 0; i < gBuffersCounts; i++)
                {
                    ID3D12Resource* pBackBuffer = nullptr;
                    pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
                    g_mainRenderTargetDescriptor[i] = rtvHandle;
                    gDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
                    g_mainRenderTargetResource[i] = pBackBuffer;
                    rtvHandle.ptr += rtvDescriptorSize;
                    gFrameContext[i].FenceValue = 0;
                }
                STdebug_printf("[IMGUI device building]         Rtv build Success\n");
            }

            /*g_hSwapChainWaitableObject = pSwapChain->GetFrameLatencyWaitableObject();
            STdebug_printf("[IMGUI device building]     GetFrameLatencyWaitableObject\n");*/

            



            {
            ImGui_ImplDX12_InitInfo init_info = {};
            init_info.Device = gDevice;
            init_info.CommandQueue = gCommandQueue;
            init_info.NumFramesInFlight = gBuffersCounts;
            init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
            init_info.SrvDescriptorHeap = g_pd3dSrvDescHeap;
            init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle,out_gpu_handle); };

            init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };


            if(!ImGui_ImplDX12_Init(&init_info)) return false;
            STdebug_printf("[IMGUI device building]     DX12 ImGui_ImplDX12_Init Success!\n");

            if(!ImGui_ImplDX12_CreateDeviceObjects()) return false;
            STdebug_printf("[IMGUI device building]     DX12 SrvDescriptorFreeFn created Success!\n");
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
    
    WaitForPendingOperations();
    /*ImGui_ImplWin32_Shutdown();*/
    ImGui_ImplDX12_Shutdown();/*
    ImGui::DestroyContext();*/
    CloseHandle(gFenceEvent);
    // 4. 释放所有 FrameContext 资源 (Back Buffers)
    for (size_t i = 0; i < gBuffersCounts; i++) {
        if (g_mainRenderTargetResource[i]) {
            g_mainRenderTargetResource[i]->Release();
            g_mainRenderTargetResource[i] = nullptr;
        }
        // 命令分配器 CommandAllocator 通常不需要释放，但如果在 InitImgui 中分配了新的，这里也应该清理
        if (gFrameContext[i].CommandAllocator) {
            gFrameContext[i].CommandAllocator->Release(); // 取决于您 InitImgui 中的分配方式
        }
    }

    // 5. 释放 FrameContext 数组本身
    if (gFrameContext) {
        delete[] gFrameContext;
        gFrameContext = nullptr;
    }
    // 6. 释放 DX12 资源
    if (gCommandList) gCommandList->Release();
    if (g_pd3dSrvDescHeap) g_pd3dSrvDescHeap->Release(); // ImGui SRV 堆
    if (g_pd3dRtvDescHeap) g_pd3dRtvDescHeap->Release(); // RTV 堆

    // 7. 释放您自己的同步资源 (如果它们是在 Hook 成功后创建的)
    if (gFence) gFence->Release();
    if (gFenceEvent) CloseHandle(gFenceEvent);

    // 8. 重置 ImGui 描述符分配器追踪 (如果您使用了自定义分配器)
    g_pd3dSrvDescHeapAlloc.Destroy();


    // 9. 重置状态标志
    gCommandList = nullptr;
    g_pd3dSrvDescHeap = nullptr;
    g_pd3dRtvDescHeap = nullptr;
    gBuffersCounts = 0;


    isDX12Init = false;
    isDX12reInit = true; // 如果您想在重置后重新加载字体，则重置此标志
}

void STIMGUI::WaitForPendingOperations()
{
    gCommandQueue->Signal(gFence, ++gfenceLastSignaledValue);

    gFence->SetEventOnCompletion(gfenceLastSignaledValue, gFenceEvent);
    ::WaitForSingleObject(gFenceEvent, INFINITE);
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



STIMGUI::_FrameContext* STIMGUI::WaitForNextFrameContext()
{
    _FrameContext* frame_context = &gFrameContext[g_frameIndex % gBuffersCounts];
    if (frame_context->FenceValue != 0 and gFence->GetCompletedValue() < frame_context->FenceValue)
    {
        HRESULT hr = gFence->SetEventOnCompletion(frame_context->FenceValue, gFenceEvent);
        if (FAILED(hr))
        {
            // 错误处理：如果无法设置事件，强制等待可能会死锁或崩溃
            // 在这里可以添加日志


            STdebug_printf("[ImGuiFinal] SetEventOnCompletion Failed, forcing wait\n");
            return frame_context;
        }
        ::WaitForSingleObject(gFenceEvent, INFINITE);
    }
        

    return frame_context;
}




void STIMGUI::ImGuiFinal(IDXGISwapChain3* pSwapChain) {


    ImGui::Render();
    /*STIMGUI::_FrameContext& CurrentFrameContext = gFrameContext[((IDXGISwapChain3*)pSwapChain)->GetCurrentBackBufferIndex()];*/
    UINT backBufferIdx = pSwapChain->GetCurrentBackBufferIndex();
    _FrameContext* frameCtx = &gFrameContext[backBufferIdx];

    if (frameCtx->FenceValue != 0 && gFence->GetCompletedValue() < frameCtx->FenceValue)
    {
        HRESULT hr = gFence->SetEventOnCompletion(frameCtx->FenceValue, gFenceEvent);
        if (FAILED(hr))
        {
            // 错误处理：如果无法设置事件，强制等待可能会死锁或崩溃
            // 在这里可以添加日志
            STdebug_printf("[ImGuiFinal] SetEventOnCompletion Failed, forcing wait\n");
            return;
        }
        ::WaitForSingleObject(gFenceEvent, INFINITE);
    }

    frameCtx->CommandAllocator->Reset();
    D3D12_RESOURCE_BARRIER Barrier;
    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
    Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    gCommandList->Reset(frameCtx->CommandAllocator, nullptr);
    gCommandList->ResourceBarrier(1, &Barrier);

    

    gCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
    gCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);


    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), gCommandList);
    Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    gCommandList->ResourceBarrier(1, &Barrier);
    gCommandList->Close();
    gCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&gCommandList));

   HRESULT hr= gCommandQueue->Signal(gFence, ++gfenceLastSignaledValue);

    frameCtx->FenceValue = gfenceLastSignaledValue;
    g_frameIndex++;
};





void STIMGUI::release() {
    if (!isDX12Init)
        return;
    ImGui_ImplWin32_Shutdown();
    ImGui_ImplDX12_Shutdown();
    ImGui::DestroyContext();
    CloseHandle(gFenceEvent);


    // 4. 释放所有 FrameContext 资源 (Back Buffers)
    for (size_t i = 0; i < gBuffersCounts; i++) {
        if (g_mainRenderTargetResource[i]) {
            g_mainRenderTargetResource[i]->Release();
            g_mainRenderTargetResource[i] = nullptr;
        }
        // 命令分配器 CommandAllocator 通常不需要释放，但如果在 InitImgui 中分配了新的，这里也应该清理
        if (gFrameContext[i].CommandAllocator) {
            gFrameContext[i].CommandAllocator->Release(); // 取决于您 InitImgui 中的分配方式
        }
    }

    // 5. 释放 FrameContext 数组本身
    if (gFrameContext) {
        delete[] gFrameContext;
        gFrameContext = nullptr;
    }
    // 6. 释放 DX12 资源
    if (gCommandList) gCommandList->Release();
    if (g_pd3dSrvDescHeap) g_pd3dSrvDescHeap->Release(); // ImGui SRV 堆
    if (g_pd3dRtvDescHeap) g_pd3dRtvDescHeap->Release(); // RTV 堆

    // 7. 释放您自己的同步资源 (如果它们是在 Hook 成功后创建的)
     if (gFence) gFence->Release(); 
     if (gFenceEvent) CloseHandle(gFenceEvent); 

    // 8. 重置 ImGui 描述符分配器追踪 (如果您使用了自定义分配器)
     g_pd3dSrvDescHeapAlloc.Destroy(); 


    // 9. 重置状态标志
    gCommandList = nullptr;
    g_pd3dSrvDescHeap = nullptr;
    g_pd3dRtvDescHeap = nullptr;
    gBuffersCounts = 0;

    isDX12Init = false;
    isDX12reInit = false; // 如果您想在重置后重新加载字体，则重置此标志
    
}