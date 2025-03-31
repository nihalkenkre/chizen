#pragma once

#include <Windows.h>
#include <iostream>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>

#include "renderer.hpp"

#define RENDER_TARGET_COUNT 2

class dx12_renderer : public renderer
{
public:
    dx12_renderer(const HWND h_wnd, const std::string &path);
    void resize(const WORD width, const WORD height) override;
    void render_background(const POINT pt) override;
    ~dx12_renderer();

private:
    void import_scene(const cgltf_data *data);
    void create_pipelines();
    IDXGIFactory7 *factory7;
#ifdef DEBUG
    ID3D12Debug *debug_controller;
#endif
    IDXGIAdapter4 *adapter4;
    ID3D12Device10 *device10;
    ID3D12CommandQueue *cmd_queue;
    IDXGISwapChain4 *swapchain4;
    ID3D12DescriptorHeap *rtv_desc_heap;
    ID3D12Resource2 *rt[RENDER_TARGET_COUNT];
    ID3D12CommandAllocator *rt_cmd_allocs[RENDER_TARGET_COUNT];
    ID3D12GraphicsCommandList7 *rt_cmd_lists[RENDER_TARGET_COUNT];
    ID3D12Fence1 *rt_fncs[RENDER_TARGET_COUNT];
    UINT64 rt_fnc_vals[RENDER_TARGET_COUNT];

    std::vector<ID3D12PipelineState *> pipelines;

    ID3D12RootSignature *root_sig;

    UINT img_idx;
    D3D12_RECT wnd_rect;
};