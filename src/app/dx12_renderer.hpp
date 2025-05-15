#pragma once

#include <Windows.h>
#include <iostream>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXMath.h>

#include "renderer.hpp"

using Microsoft::WRL::ComPtr;

#define RENDER_TARGET_COUNT 2

class dx12_renderer : public renderer
{
public:
    dx12_renderer(const HWND h_wnd);
    void import_scene_data(const std::string& file_path) override;
    void handle_mouse_move(const POINT mouse_pos) override {}
    void handle_mouse_l_btn_down() override {}
    void handle_mouse_l_btn_up() override {}
    void handle_mouse_m_btn_down() override {}
    void handle_mouse_m_btn_up() override {}
    void handle_mouse_r_btn_down() override {}
    void handle_mouse_r_btn_up() override {}
    void handle_w_down() override {}
    void handle_a_down() override {}
    void handle_s_down() override {}
    void handle_d_down() override {}
    void handle_q_down() override {}
    void handle_e_down() override {}
    void handle_w_up() override {}
    void handle_a_up() override {}
    void handle_s_up() override {}
    void handle_d_up() override {}
    void handle_q_up() override {}
    void handle_e_up() override {}
    void resize(const uint32_t width, const uint32_t height) override;
    void begin_frame() override;
    void clear_frame(const float color[]) override;
    void render_world() override;
    void end_frame() override;
    void clear_scene_data() override;
    ~dx12_renderer();

private:
    ComPtr<IDXGIFactory7> factory7;
#ifdef DEBUG
    ComPtr<ID3D12Debug> debug_controller;
#endif
    ComPtr<IDXGIAdapter4> adapter4;
    ComPtr<ID3D12Device10> device10;
    ComPtr<ID3D12CommandQueue> sc_cmd_queue;
    ComPtr<IDXGISwapChain4> swapchain4;
    ComPtr<ID3D12DescriptorHeap> rtv_desc_heap;
    ComPtr<ID3D12DescriptorHeap> dsv_desc_heap;
    ComPtr<ID3D12Resource2> sc_rt[RENDER_TARGET_COUNT];
    ComPtr<ID3D12Resource2> sc_ds;
    ComPtr<ID3D12CommandAllocator> sc_cmd_allocs[RENDER_TARGET_COUNT];
    ComPtr<ID3D12GraphicsCommandList7> sc_cmd_lists[RENDER_TARGET_COUNT];
    ComPtr<ID3D12Fence1> sc_fncs[RENDER_TARGET_COUNT];
    UINT64 sc_fnc_vals[RENDER_TARGET_COUNT];

    ComPtr<ID3D12CommandQueue> gnrl_cmd_queue;
    ComPtr<ID3D12CommandAllocator> gnrl_cmd_alloc;
    ComPtr<ID3D12GraphicsCommandList10> gnrl_cmd_list;
    ComPtr<ID3D12Fence> gnrl_fnc;
    UINT64 gnrl_fnc_val;

    struct float3
    {
        float x;
        float y;
        float z;
    };

    struct uint8_t3
    {
        uint8_t x;
        uint8_t y;
        uint8_t z;
    };

    struct primitive_data
    {
        DirectX::XMMATRIX xform;
        std::vector<ComPtr<ID3D12Resource2>> geometry_buffers;
        ComPtr<ID3D12Heap1> geometry_heap;
        size_t meshlets_count;
    };

    struct material_info
    {
        uint64_t id;
        std::vector<primitive_data> prims_data;

        bool operator==(const uint64_t id)
        {
            return this->id == id;
        }
    };

    struct scene_data
    {
        ComPtr<ID3D12Heap> geometry_data_heap;
        std::vector<material_info> material_infos;
    };
    std::unique_ptr<scene_data> sd;

    std::pair<std::vector<ComPtr<ID3D12Resource2>>, ComPtr<ID3D12Heap1>> CreateDefaultBuffersAndHeap(const std::vector<CD3DX12_RESOURCE_DESC1> resource_descs);
    std::pair<std::vector<ComPtr<ID3D12Resource2>>, ComPtr<ID3D12Heap1>> CreateDefaultBuffersAndHeapFromData(const std::vector<CD3DX12_RESOURCE_DESC1> resource_descs, std::vector<std::vector<uint8_t>> data);
    std::pair<std::vector<ComPtr<ID3D12Resource2>>, ComPtr<ID3D12Heap1>> CreateDefaultTexturesAndHeap(const std::vector<D3D12_RESOURCE_DESC1> resources_descs);
    std::pair<std::vector<ComPtr<ID3D12Resource2>>, ComPtr<ID3D12Heap1>> CreateDefaultTexturesAndHeapFromData(const std::vector<CD3DX12_RESOURCE_DESC1> resources_descs, std::vector<std::vector<uint8_t>> data);

    struct pipeline
    {
        ComPtr<ID3D12PipelineState> state;
        ComPtr<ID3D12RootSignature> root_signature;
    };
    void create_pipelines();
    void wait_for_gpu(ID3D12Fence* fence, UINT64 fence_value);

    std::vector<pipeline> pipelines;

    D3D12_VIEWPORT viewport;
    UINT img_idx;
};