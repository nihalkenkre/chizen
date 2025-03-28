#include "dx12_renderer.hpp"

#include <sstream>
#include <dxcapi.h>
#include <d3d12shader.h>
#include <Shlwapi.h>
#include <filesystem>
#include <fstream>

#include <cgltf/cgltf.h>
#include <meshoptimizer/src/meshoptimizer.h>

#define MAX_VERTICES 64
#define MAX_TRIANGLES 124

struct float3
{
    float x;
    float y;
    float z;
};

inline void DX_CHECK(const std::string action, const HRESULT result)
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
        std::exit(result);
    }
}

inline std::vector<std::string> tokenize(std::string str, char delim)
{
    auto str_stream = std::stringstream(str);

    std::string tmp_str;
    std::vector<std::string> tokens;

    while (std::getline(str_stream, tmp_str, delim))
    {
        tokens.push_back(tmp_str);
    }

    return tokens;
}

void dx12_renderer::create_pipelines()
{
    char curr_dir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, curr_dir);

#ifdef DEBUG
    PathRemoveFileSpecA(curr_dir);
#endif

    std::string tmp(curr_dir);
    tmp.append("\\shaders\\world");

    ComPtr<IDxcUtils> utils;
    ComPtr<IDxcCompiler3> compiler;
    DX_CHECK("create dxc utils instance", DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
    DX_CHECK("create dxc compile instance", DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    ComPtr<IDxcIncludeHandler> include_handler;
    DX_CHECK("create include handler", utils->CreateDefaultIncludeHandler(&include_handler));

    ComPtr<IDxcBlob> as;
    ComPtr<IDxcBlob> ms;
    ComPtr<IDxcBlob> ps;
    ComPtr<IDxcBlob> root_sig_blob;

    for (auto const &file : std::filesystem::directory_iterator(std::filesystem::path(tmp)))
    {
        auto file_path_tokens = tokenize(file.path().string(), '\\');
        auto file_name_tokens = tokenize(file_path_tokens[file_path_tokens.size() - 1], '.');

        if (file_name_tokens[file_name_tokens.size() - 1] == "hlsl")
        {
            ComPtr<IDxcBlobEncoding> source_file;
            DX_CHECK("load shader file", utils->LoadFile(file.path().c_str(), nullptr, &source_file));
            const DxcBuffer source = {
                .Ptr = source_file->GetBufferPointer(),
                .Size = source_file->GetBufferSize(),
                .Encoding = DXC_CP_ACP,
            };

            if (file_name_tokens[0].find("root_sig") != std::string::npos)
            {
                std::vector<LPCWSTR> args;
                args.push_back(file.path().c_str());

                args.push_back(L"-E");
                args.push_back(L"ROOTSIG");
                args.push_back(L"-T");
                args.push_back(L"rootsig_1_1");

                auto cs_name = file_name_tokens[0] + ".root";
                auto output_cso = std::wstring(std::begin(cs_name), std::end(cs_name));

                args.push_back(L"-Fo");
                args.push_back(output_cso.c_str());

                ComPtr<IDxcResult> results;
                DX_CHECK("compile rootsig", compiler->Compile(&source, args.data(), args.size(), include_handler.Get(), IID_PPV_ARGS(&results)));

                ComPtr<IDxcBlobEncoding> errors;
                DX_CHECK("get error buffer", results->GetErrorBuffer(&errors));

                if (errors != nullptr && errors->GetBufferSize() > 0)
                {
                    std::cout << reinterpret_cast<char *>(errors->GetBufferPointer()) << '\n';
                    std::exit(0xdeadbeef);
                }

                ComPtr<IDxcBlobUtf16> name;
                DX_CHECK("get rootsig", results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&root_sig_blob), &name));
                // std::wcout << "rootsig name " << name->GetStringPointer() << '\n';
            }
            else
            {
                auto shader_types = tokenize(file_name_tokens[0], '_');

                for (size_t i = 0; i < shader_types.size(); ++i)
                {
                    std::vector<LPCWSTR> args;
                    args.push_back(file.path().c_str());

                    auto base_name = shader_types[i];
                    auto obj_name = base_name + ".cso";
                    auto output_cso = std::wstring(std::begin(obj_name), std::end(obj_name));

                    auto pdb_name = base_name + ".pdb";
                    auto output_pdb = std::wstring(std::begin(pdb_name), std::end(pdb_name));

                    auto ref_name = base_name + ".ref";
                    auto output_ref = std::wstring(std::begin(ref_name), std::end(ref_name));

                    args.push_back(L"-Fo");
                    args.push_back(output_cso.c_str());
                    args.push_back(L"-Fd");
                    args.push_back(output_pdb.c_str());
                    args.push_back(L"-Fre");
                    args.push_back(output_ref.c_str());

                    if (shader_types[i] == "as")
                    {
                        args.push_back(L"-E");
                        args.push_back(L"asmain");
                        args.push_back(L"-T");
                        args.push_back(L"as_6_6");
                    }
                    else if (shader_types[i] == "ms")
                    {
                        args.push_back(L"-E");
                        args.push_back(L"msmain");
                        args.push_back(L"-T");
                        args.push_back(L"ms_6_6");
                    }
                    else if (shader_types[i] == "ps")
                    {
                        args.push_back(L"-E");
                        args.push_back(L"psmain");
                        args.push_back(L"-T");
                        args.push_back(L"ps_6_6");
                    }

                    args.push_back(L"-Zi");
                    args.push_back(L"-Qstrip_reflect");

                    ComPtr<IDxcResult> results;
                    DX_CHECK("compile shader source", compiler->Compile(&source, args.data(), args.size(), include_handler.Get(), IID_PPV_ARGS(&results)));

                    ComPtr<IDxcBlobEncoding> errors;
                    DX_CHECK("get error buffer", results->GetErrorBuffer(&errors));

                    if (errors != nullptr && errors->GetBufferSize() > 0)
                    {
                        std::cout << reinterpret_cast<char *>(errors->GetBufferPointer()) << '\n';
                        std::exit(0xdeadbeef);
                    }

                    ComPtr<IDxcBlobUtf16> name;
                    if (shader_types[i] == "as")
                    {
                        DX_CHECK("get shader binary", results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&as), &name));
                        // std::wcout << "bin name " << name->GetStringPointer() << '\n';
                    }
                    else if (shader_types[i] == "ms")
                    {
                        DX_CHECK("get shader binary", results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ms), &name));
                        // std::wcout << "bin name " << name->GetStringPointer() << '\n';
                    }
                    else if (shader_types[i] == "ps")
                    {
                        DX_CHECK("get shader binary", results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ps), &name));
                        // std::wcout << "bin name " << name->GetStringPointer() << '\n';
                    }

                    ComPtr<IDxcBlob> pdb;
                    DX_CHECK("get pdb", results->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdb), &name));
                    // std::wcout << "pdb name " << name->GetStringPointer() << '\n';

                    std::ofstream pdb_file(name->GetStringPointer());
                    pdb_file.write(reinterpret_cast<const char *>(pdb->GetBufferPointer()), pdb->GetBufferSize());
                    pdb_file.close();

                    ComPtr<IDxcBlob> reflection;
                    DX_CHECK("get reflection", results->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflection), &name));
                    // std::wcout << "ref name " << name->GetStringPointer() << '\n';
                }
            }
        }
    }

    DX_CHECK("create root signature", device10->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), root_sig_blob->GetBufferSize(), IID_PPV_ARGS(&root_sig)));

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC pipeline_desc = {
        .pRootSignature = root_sig.Get(),
        .MS = {
            .pShaderBytecode = ms->GetBufferPointer(),
            .BytecodeLength = ms->GetBufferSize(),
        },
        .PS = {
            .pShaderBytecode = ps->GetBufferPointer(),
            .BytecodeLength = ps->GetBufferSize(),
        },
        .BlendState = {
            .AlphaToCoverageEnable = FALSE,
            .IndependentBlendEnable = FALSE,
            .RenderTarget = {
                {
                    .BlendEnable = FALSE,
                    .SrcBlend = D3D12_BLEND_SRC_ALPHA,
                    .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
                    .BlendOp = D3D12_BLEND_OP_ADD,
                    .SrcBlendAlpha = D3D12_BLEND_ONE,
                    .DestBlendAlpha = D3D12_BLEND_ZERO,
                    .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                    .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
                },
            },
        },
        .SampleMask = D3D12_DEFAULT_SAMPLE_MASK,
        .RasterizerState = {
            .FillMode = D3D12_FILL_MODE_SOLID,
            .CullMode = D3D12_CULL_MODE_NONE,
            .FrontCounterClockwise = TRUE,
        },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
        },
    };

    CD3DX12_PIPELINE_MESH_STATE_STREAM pipeline_stream(pipeline_desc);

    const D3D12_PIPELINE_STATE_STREAM_DESC pipeline_stream_desc = {
        .SizeInBytes = sizeof(pipeline_stream),
        .pPipelineStateSubobjectStream = &pipeline_stream,
    };

    ComPtr<ID3D12PipelineState> pipeline;
    DX_CHECK("create pipeline", device10->CreatePipelineState(&pipeline_stream_desc, IID_PPV_ARGS(&pipeline)));

    pipelines.push_back(pipeline);
}

dx12_renderer::dx12_renderer(const HWND h_wnd, const std::string &path)
{
    std::cout << __FUNCTION__ << '\n';
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

    D3D12_FEATURE_DATA_SHADER_MODEL shader_model = {D3D_SHADER_MODEL_6_5};
    DX_CHECK("check shader model support", device10->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shader_model, sizeof(shader_model)));

    if (shader_model.HighestShaderModel < D3D_SHADER_MODEL_6_5)
    {
        std::cout << "Shader Model 6.5 not supported\n";
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    DX_CHECK("Check options7 support", device10->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)));

    if (options7.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
    {
        std::cout << "Mesh shading tier not supported\n";
    }

    const D3D12_COMMAND_QUEUE_DESC queue_desc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };

    DX_CHECK("create command queue", device10->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&cmd_queue)));

    GetWindowRect(h_wnd, &wnd_rect);
    wnd_rect.right -= wnd_rect.left;
    wnd_rect.left = 0;
    wnd_rect.bottom -= wnd_rect.top;
    wnd_rect.top = 0;

    const DXGI_SWAP_CHAIN_DESC1 sc_desc1 = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER,
        .BufferCount = RENDER_TARGET_COUNT,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };

    ComPtr<IDXGISwapChain1> swapchain1;
    DX_CHECK("create swapchain", factory7->CreateSwapChainForHwnd(cmd_queue.Get(), h_wnd, &sc_desc1, nullptr, nullptr, &swapchain1));
    swapchain1.As(&swapchain4);

    const D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = sc_desc1.BufferCount,
    };

    DX_CHECK("create rtv desc heap", device10->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_desc_heap)));

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_desc_heap_hnd = rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();

    for (UINT f = 0; f < RENDER_TARGET_COUNT; ++f)
    {
        DX_CHECK("get render target buffer", swapchain4->GetBuffer(f, IID_PPV_ARGS(&rt[f])));

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

    create_pipelines();

    if (!path.empty())
    {
        cgltf_options options = {};
        cgltf_data *data = nullptr;

        if (!(cgltf_parse_file(&options, path.c_str(), &data) == cgltf_result_success && cgltf_validate(data) == cgltf_result_success && cgltf_load_buffers(&options, data, path.c_str()) == cgltf_result_success))
        {
            std::cerr << "ERR Could not parse gltf file\n";
        }
        else
        {
            this->import_scene(data);
            cgltf_free(data);
        }
    }

    is_inited = true;
}

void dx12_renderer::import_scene(const cgltf_data *data)
{
    Sleep(20);
    // std::vector<uint32_t> indices;
    // std::vector<float> positions;

    // std::vector<meshopt_Meshlet> meshlets;
    // std::vector<uint32_t> meshlets_vertices;
    // std::vector<uint8_t> meshlets_triangles;

    // for (cgltf_size m = 0; m < data->meshes_count; ++m)
    // {
    //     cgltf_mesh *curr_mesh = data->meshes + m;
    //     for (cgltf_size p = 0; p < curr_mesh->primitives_count; ++p)
    //     {
    //         cgltf_primitive *curr_prim = curr_mesh->primitives + p;

    //         indices.resize(curr_prim->indices->count);

    //         if (curr_prim->indices->component_type == cgltf_component_type_r_16u)
    //         {
    //             for (cgltf_size i = 0; i < curr_prim->indices->count; ++i)
    //             {
    //                 indices[i] = *(uint16_t *)((ULONG_PTR)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset + i);
    //             }
    //         }
    //         else if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
    //         {
    //             std::memcpy(indices.data(), (void *)((ULONG_PTR)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset), curr_prim->indices->buffer_view->size);
    //         }

    //         for (cgltf_size a = 0; a < curr_prim->attributes_count; ++a)
    //         {
    //             cgltf_attribute *curr_attr = curr_prim->attributes + a;

    //             if (std::strcmp(curr_attr->name, "POSITION") == 0)
    //             {
    //                 positions.resize(curr_attr->data->count);
    //                 std::memcpy(positions.data(), (void *)((ULONG_PTR)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->buffer_view->size);

    //                 break;
    //             }

    //             break;
    //         }

    //         break;
    //     }
    // }

    // size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), MAX_VERTICES, MAX_TRIANGLES);

    // meshlets.resize(max_meshlets);
    // meshlets_vertices.resize(max_meshlets * MAX_VERTICES);
    // meshlets_triangles.resize(max_meshlets * MAX_TRIANGLES * 3);

    // size_t meshlets_count = meshopt_buildMeshlets(meshlets.data(), meshlets_vertices.data(), meshlets_triangles.data(), indices.data(), indices.size(), reinterpret_cast<float *>(positions.data()), positions.size(), sizeof(float3), MAX_VERTICES, MAX_TRIANGLES, 0);

    // std::cout << "meshlet count " << meshlets_count << '\n';
}

void dx12_renderer::resize(const WORD width, const WORD height)
{
    wnd_rect.right = width;
    wnd_rect.bottom = height;

    for (UINT f = 0; f < RENDER_TARGET_COUNT; ++f)
    {
        if (rt_fncs[f]->GetCompletedValue() < rt_fnc_vals[f])
        {
            HANDLE wait_idle_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);

            DX_CHECK("set wait idle event", rt_fncs[f]->SetEventOnCompletion(rt_fnc_vals[f], wait_idle_event));

            WaitForSingleObject(wait_idle_event, UINT64_MAX);
            CloseHandle(wait_idle_event);
        }
        rt[f].Reset();
    }

    DX_CHECK("swapchain resize buffers", swapchain4->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_desc_heap_hnd = rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
    const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
    };

    for (UINT f = 0; f < RENDER_TARGET_COUNT; ++f)
    {
        DX_CHECK("swapchain get buffer", swapchain4->GetBuffer(f, IID_PPV_ARGS(&rt[f])));

        rtv_desc_heap_hnd.ptr += f * device10->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        device10->CreateRenderTargetView(rt[f].Get(), &rtv_desc, rtv_desc_heap_hnd);
    }
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
    rt_cmd_lists[img_idx]->OMSetRenderTargets(1, &curr_rtv_hnd, FALSE, nullptr);
    rt_cmd_lists[img_idx]->ClearRenderTargetView(curr_rtv_hnd, clear_color, 1, &wnd_rect);

    const D3D12_VIEWPORT viewports[] = {
        {
            .TopLeftX = 0,
            .TopLeftY = 0,
            .Width = (float)wnd_rect.right,
            .Height = (float)wnd_rect.bottom,
            .MinDepth = 0.0,
            .MaxDepth = 1.0,
        },
    };

    const D3D12_RECT scissors[] = {
        wnd_rect,
    };

    rt_cmd_lists[img_idx]->RSSetViewports(_countof(viewports), viewports);
    rt_cmd_lists[img_idx]->RSSetScissorRects(_countof(scissors), scissors);

    rt_cmd_lists[img_idx]->SetGraphicsRootSignature(root_sig.Get());
    rt_cmd_lists[img_idx]->SetPipelineState(pipelines[0].Get());
    rt_cmd_lists[img_idx]->DispatchMesh(1, 1, 1);

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
    std::cout << __FUNCTION__ << '\n';

    if (rt_fncs[img_idx]->GetCompletedValue() < rt_fnc_vals[img_idx])
    {
        HANDLE wait_idle_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);

        DX_CHECK("set wait idle event", rt_fncs[img_idx]->SetEventOnCompletion(rt_fnc_vals[img_idx], wait_idle_event));

        WaitForSingleObject(wait_idle_event, UINT64_MAX);
        CloseHandle(wait_idle_event);
    }
}