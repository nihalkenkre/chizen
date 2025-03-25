#pragma once

#include <Windows.h>
#include <iostream>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

#include "renderer.hpp"

#define RENDER_TARGET_COUNT 2

class dx12_renderer : public renderer
{
public:
    dx12_renderer(const HWND h_wnd);
    void import_scene(cgltf_data *data) override;
    void resize(const WORD width, const WORD height) override;
    void render_background(const POINT pt) override;
    ~dx12_renderer();

private:
#ifdef DEBUG
    ComPtr<ID3D12Debug> debug_controller;
#endif
    ComPtr<IDXGIFactory7> factory7;
    ComPtr<IDXGIAdapter4> adapter4;
    ComPtr<ID3D12Device10> device10;
    ComPtr<ID3D12CommandQueue> cmd_queue;
    ComPtr<IDXGISwapChain4> swapchain4;
    ComPtr<ID3D12DescriptorHeap> rtv_desc_heap;
    ComPtr<ID3D12Resource2> rt[RENDER_TARGET_COUNT];
    ComPtr<ID3D12CommandAllocator> rt_cmd_allocs[RENDER_TARGET_COUNT];
    ComPtr<ID3D12GraphicsCommandList7> rt_cmd_lists[RENDER_TARGET_COUNT];
    ComPtr<ID3D12Fence1> rt_fncs[RENDER_TARGET_COUNT];
    UINT64 rt_fnc_vals[RENDER_TARGET_COUNT];

    UINT img_idx;
    D3D12_RECT wnd_rect;
};

namespace dx12
{
    std::unique_ptr<dx12_renderer> init(const HWND h_wnd);
}
