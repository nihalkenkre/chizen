#include "dx12_renderer.hpp"

#include <sstream>

#include <cgltf/cgltf.h>

inline void DX_CHECK(std::string action, HRESULT result)
{
    if FAILED (result)
    {
        std::stringstream msg;
        msg << "ERR: " << action << " 0x" << std::hex << result << "\nExiting...\n";
#ifdef DEBUG
        OutputDebugStringA(msg.str().c_str());
#else
        std::cout << msg.str();
#endif
        PostQuitMessage(result);
    }
}

dx12_renderer::dx12_renderer(const HWND h_wnd)
{
    UINT dxgi_factory_flags = 0;
#ifdef DEBUG
    DX_CHECK("get debug interface", D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
    debug_controller->EnableDebugLayer();
    dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    DX_CHECK("create dxgi factory", CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&factory7)));

    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory7->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter4)); ++i)
    {
        DXGI_ADAPTER_DESC3 adapter_desc3;
        adapter4->GetDesc3(&adapter_desc3);

        if (adapter_desc3.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        if (SUCCEEDED(D3D12CreateDevice(reinterpret_cast<IUnknown *>(adapter4.Get()), D3D_FEATURE_LEVEL_12_2, IID_ID3D12Device10, nullptr)))
        {
            break;
        }
    }

    DX_CHECK("create device", D3D12CreateDevice(reinterpret_cast<IUnknown *>(adapter4.Get()), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device10)));

    const D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_2,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS feat_levels = {
        .NumFeatureLevels = _countof(feature_levels),
        .pFeatureLevelsRequested = feature_levels,
        .MaxSupportedFeatureLevel = D3D_FEATURE_LEVEL_11_0,
    };

    DX_CHECK("check feature level support", device10->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feat_levels, sizeof(feat_levels)));

    const D3D12_COMMAND_QUEUE_DESC queue_desc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };

    DX_CHECK("create command queue", device10->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&cmd_queue)));

    GetWindowRect(h_wnd, &wnd_rect);
    wnd_rect.right -= wnd_rect.left;
    wnd_rect.left -= wnd_rect.left;
    wnd_rect.bottom -= wnd_rect.top;
    wnd_rect.top -= wnd_rect.top;

    const DXGI_SWAP_CHAIN_DESC1 sc_desc1 = {
        .Width = UINT(wnd_rect.right - wnd_rect.left),
        .Height = UINT(wnd_rect.bottom - wnd_rect.top),
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = RENDER_TARGET_COUNT,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };

    ComPtr<IDXGISwapChain1> swapchain1;
    DX_CHECK("create swapchain", factory7->CreateSwapChainForHwnd(cmd_queue.Get(), h_wnd, &sc_desc1, nullptr, nullptr, &swapchain1));
    swapchain1.As(&swapchain4);

    img_idx = swapchain4->GetCurrentBackBufferIndex();

    const D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = sc_desc1.BufferCount,
    };

    DX_CHECK("create rtv desc heap", device10->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_desc_heap)));

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_desc_heap_hnd = rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();

    for (UINT f = 0; f < RENDER_TARGET_COUNT; ++f)
    {
        DX_CHECK("create render target", swapchain4->GetBuffer(f, IID_PPV_ARGS(&rt[f])));

        const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
        };

        rtv_desc_heap_hnd.ptr += f * device10->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        device10->CreateRenderTargetView(rt[f].Get(), &rtv_desc, rtv_desc_heap_hnd);

        DX_CHECK("create cmd allocator", device10->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&rt_cmd_allocs[f])));
        DX_CHECK("create cmd list", device10->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&rt_cmd_lists[f])));
        DX_CHECK("create fence", device10->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&rt_fncs[f])));

        rt_fnc_vals[f] = 0;
    }
}

void dx12_renderer::import_scene(cgltf_data *data)
{
}

void dx12_renderer::render_background(const POINT pt)
{
    img_idx = swapchain4->GetCurrentBackBufferIndex();

    D3D12_CPU_DESCRIPTOR_HANDLE curr_rtv_hnd = rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
    curr_rtv_hnd.ptr += (img_idx * device10->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

    float clear_color[4] = {(float)pt.x / (wnd_rect.right - wnd_rect.left), (float)pt.y / (wnd_rect.bottom - wnd_rect.top), 1, 0};

    DX_CHECK("reset command allocator", rt_cmd_allocs[img_idx]->Reset());
    DX_CHECK("reset command list", rt_cmd_lists[img_idx]->Reset(rt_cmd_allocs[img_idx].Get(), nullptr));

    const D3D12_RESOURCE_BARRIER prsnt_to_rt_barr = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = {
            .pResource = rt[img_idx].Get(),
            .Subresource = 0,
            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
            .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
        },
    };

    rt_cmd_lists[img_idx]->ResourceBarrier(1, &prsnt_to_rt_barr);
    rt_cmd_lists[img_idx]->ClearRenderTargetView(curr_rtv_hnd, clear_color, 1, &wnd_rect);

    const D3D12_RESOURCE_BARRIER rt_to_prsnt_barr = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = {
            .pResource = rt[img_idx].Get(),
            .Subresource = 0,
            .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
            .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
        },
    };
    rt_cmd_lists[img_idx]->ResourceBarrier(1, &rt_to_prsnt_barr);
    DX_CHECK("close cmd list", rt_cmd_lists[img_idx]->Close());

    ID3D12CommandList *cmd_lists[] = {
        rt_cmd_lists[img_idx].Get(),
    };

    cmd_queue->ExecuteCommandLists(1, cmd_lists);
    DX_CHECK("swapchain present", swapchain4->Present(0, 0));

    ++rt_fnc_vals[img_idx];
    cmd_queue->Signal(rt_fncs[img_idx].Get(), rt_fnc_vals[img_idx]);
    if (rt_fncs[img_idx]->GetCompletedValue() < rt_fnc_vals[img_idx])
    {
        HANDLE wait_idle_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);

        DX_CHECK("set wait idle event", rt_fncs[img_idx]->SetEventOnCompletion(rt_fnc_vals[img_idx], wait_idle_event));

        WaitForSingleObject(wait_idle_event, UINT64_MAX);
        CloseHandle(wait_idle_event);
    }
}

dx12_renderer::~dx12_renderer()
{
    if (rt_fncs[img_idx]->GetCompletedValue() < rt_fnc_vals[img_idx])
    {
        HANDLE wait_idle_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);

        DX_CHECK("set wait idle event", rt_fncs[img_idx]->SetEventOnCompletion(rt_fnc_vals[img_idx], wait_idle_event));

        WaitForSingleObject(wait_idle_event, UINT64_MAX);
        CloseHandle(wait_idle_event);
    }
}

namespace dx12
{
    std::unique_ptr<dx12_renderer> init(const HWND h_wnd)
    {
        auto r = std::make_unique<dx12_renderer>(h_wnd);
        return r;
    }
}