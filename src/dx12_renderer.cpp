#include "dx12_renderer.hpp"

#include <sstream>
#include <dxcapi.h>
#include <d3d12shader.h>
#include <Shlwapi.h>
#include <filesystem>
#include <fstream>

#include <cglm/include/cglm/cglm.h>
#include <cgltf/cgltf.h>
#include <meshoptimizer/src/meshoptimizer.h>

#define MAX_VERTICES 64
#define MAX_TRIANGLES 124

inline void static DX_CHECK(const std::string action, const HRESULT result)
{
	if FAILED(result)
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

static D3D12_VIEWPORT RECT_TO_VIEWPORT(const RECT& rect)
{
	const D3D12_VIEWPORT V =
	{
		.TopLeftX = (FLOAT)rect.left,
		.TopLeftY = (FLOAT)rect.top,
		.Width = (FLOAT)(rect.right - rect.left),
		.Height = (FLOAT)(rect.bottom - rect.top),
		.MinDepth = 0,
		.MaxDepth = 1,
	};

	return V;
}

static D3D12_RECT VIEWPORT_TO_RECT(const D3D12_VIEWPORT& viewport)
{
	const D3D12_RECT R = {
		.left = (LONG)viewport.TopLeftX,
		.top = (LONG)viewport.TopLeftY,
		.right = (LONG)viewport.Width,
		.bottom = (LONG)viewport.Height,
	};

	return R;
}

static D3D12_RECT SANITIZE_RECT_FOR_RENDER(const D3D12_RECT& rect)
{
	const D3D12_RECT SR = {
		.left = 0,
		.top = 0,
		.right = rect.right - rect.left,
		.bottom = rect.bottom - rect.top,
	};

	return SR;
}

static inline std::vector<std::string> tokenize(std::string str, char delim)
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

std::pair<std::vector<ComPtr<ID3D12Resource2>>, ComPtr<ID3D12Heap1>> dx12_renderer::CreateDefaultBuffersAndHeapFromData(const std::vector<CD3DX12_RESOURCE_DESC1> resource_descs, std::vector<std::vector<uint8_t>> data)
{
	size_t resource_count = resource_descs.size();

	std::vector<D3D12_RESOURCE_ALLOCATION_INFO1> res_allocs1(resource_descs.size());
	D3D12_RESOURCE_ALLOCATION_INFO res_alloc = device10->GetResourceAllocationInfo2(0, static_cast<UINT>(resource_count), resource_descs.data(), res_allocs1.data());

	const D3D12_HEAP_DESC upload_heap_desc = {
		.SizeInBytes = res_alloc.SizeInBytes,
		.Properties = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
		},
		.Alignment = res_alloc.Alignment,
	};

	ComPtr<ID3D12Heap1> upload_heap = nullptr;

	DX_CHECK("create upload heap", device10->CreateHeap1(&upload_heap_desc, nullptr, IID_PPV_ARGS(&upload_heap)));

	std::vector<ComPtr<ID3D12Resource2>>up_res(resource_count);

	size_t idx = 0;
	for (auto const& alloc : res_allocs1)
	{
		DX_CHECK("create upload placed resource", device10->CreatePlacedResource2(upload_heap.Get(), alloc.Offset, &resource_descs[idx], D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, 0, nullptr, IID_PPV_ARGS(&up_res[idx])));

		void* map;
		up_res[idx]->Map(0, nullptr, &map);
		std::memcpy(map, data[idx].data(), data[idx].size());
		up_res[idx]->Unmap(0, nullptr);
		++idx;
	}

	const D3D12_HEAP_DESC default_heap_desc = {
		.SizeInBytes = res_alloc.SizeInBytes,
		.Properties = {
			.Type = D3D12_HEAP_TYPE_DEFAULT,
		},
		.Alignment = res_alloc.Alignment,
	};

	ComPtr<ID3D12Heap1> default_heap = nullptr;

	DX_CHECK("create default heap", device10->CreateHeap1(&default_heap_desc, nullptr, IID_PPV_ARGS(&default_heap)));

	std::vector<ComPtr<ID3D12Resource2>>def_res(resource_count);

	idx = 0;
	for (auto const& alloc : res_allocs1)
	{
		DX_CHECK("create default placed resource", device10->CreatePlacedResource2(default_heap.Get(), alloc.Offset, &resource_descs[idx], D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, 0, nullptr, IID_PPV_ARGS(&def_res[idx])));

		++idx;
	}

	std::vector<D3D12_RESOURCE_BARRIER> up_res_barr(resource_count);
	std::vector<D3D12_RESOURCE_BARRIER> def_res_barr(resource_count);

	for (size_t r = 0;r < resource_count; ++r)
	{
		up_res_barr[r].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		up_res_barr[r].Transition.pResource = up_res[r].Get();
		up_res_barr[r].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		up_res_barr[r].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

		def_res_barr[r].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		def_res_barr[r].Transition.pResource = def_res[r].Get();
		def_res_barr[r].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		def_res_barr[r].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	DX_CHECK("reset gnrl cmd alloc", gnrl_cmd_alloc->Reset());
	DX_CHECK("reset gnrl cmd list", gnrl_cmd_list->Reset(gnrl_cmd_alloc.Get(), nullptr));

	gnrl_cmd_list->ResourceBarrier(static_cast<UINT>(up_res_barr.size()), up_res_barr.data());
	gnrl_cmd_list->ResourceBarrier(static_cast<UINT>(def_res_barr.size()), def_res_barr.data());

	for (size_t r = 0;r < resource_count; ++r)
	{
		gnrl_cmd_list->CopyResource(def_res[r].Get(), up_res[r].Get());

		def_res_barr[r].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		def_res_barr[r].Transition.StateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

		gnrl_cmd_list->ResourceBarrier(1, &def_res_barr[r]);
	}

	DX_CHECK("close gnrl cmd list", gnrl_cmd_list->Close());

	ID3D12CommandList* cmd_lists[] = {
		gnrl_cmd_list.Get(),
	};

	gnrl_cmd_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);
	DX_CHECK("signal gnrl fnc", gnrl_cmd_queue->Signal(gnrl_fnc.Get(), ++gnrl_fnc_val));

	if (gnrl_fnc->GetCompletedValue() <= gnrl_fnc_val)
	{
		HANDLE wait_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		if (wait_event != nullptr)
		{
			DX_CHECK("set event on complete gnrl fnc", gnrl_fnc->SetEventOnCompletion(gnrl_fnc_val, wait_event));

			WaitForSingleObject(wait_event, DWORD_MAX);
			CloseHandle(wait_event);
		}
	}

	return std::make_pair(def_res, default_heap);
}

void dx12_renderer::create_pipelines()
{
	char curr_dir[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(NULL), curr_dir, MAX_PATH);

	PathRemoveFileSpecA(curr_dir);

	std::string tmp(curr_dir);
	tmp.append("\\shaders\\world\\");

	ComPtr<IDxcUtils> utils = nullptr;
	ComPtr<IDxcCompiler3> compiler = nullptr;
	DX_CHECK("create dxc utils instance", DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
	DX_CHECK("create dxc compile instance", DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

	ComPtr<IDxcIncludeHandler> include_handler = nullptr;
	DX_CHECK("create include handler", utils->CreateDefaultIncludeHandler(&include_handler));

	ComPtr<IDxcBlob> as = nullptr;
	ComPtr<IDxcBlob> ms = nullptr;
	ComPtr<IDxcBlob> ps = nullptr;
	ComPtr<IDxcBlob> root_sig_blob = nullptr;

	for (auto const& file : std::filesystem::directory_iterator(std::filesystem::path(tmp)))
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
				auto cs_name = file_name_tokens[0] + ".root";
				auto output_cso = std::wstring(std::begin(tmp), std::end(tmp));// +std::wstring(std::begin(cs_name), std::end(cs_name));

				LPCWSTR args[] = {
					file.path().c_str(),
					L"-E",
					L"ROOTSIG",
					L"-T",
					L"rootsig_1_1",
					L"-Fo",
					output_cso.c_str(),
					//L"-Qstrip_reflect",
				};

				ComPtr<IDxcResult> results = nullptr;
				DX_CHECK("compile rootsig", compiler->Compile(&source, args, _countof(args), include_handler.Get(), IID_PPV_ARGS(&results)));

				ComPtr<IDxcBlobEncoding> errors = nullptr;
				DX_CHECK("get error buffer", results->GetErrorBuffer(&errors));

				if (errors != nullptr && errors->GetBufferSize() > 0)
				{
					std::cout << reinterpret_cast<char*>(errors->GetBufferPointer()) << '\n';
				}

				ComPtr<IDxcBlobUtf16> name = nullptr;
				DX_CHECK("get rootsig", results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&root_sig_blob), &name));
				std::wcout << "rootsig name " << name->GetStringPointer() << '\n';

				std::ofstream out_file(std::wstring(tmp.begin(), tmp.end()) + std::wstring(name->GetStringPointer()));
				out_file.write(reinterpret_cast<const char*>(root_sig_blob->GetBufferPointer()), root_sig_blob->GetBufferSize());
				out_file.close();
			}
			else
			{
				auto shader_types = tokenize(file_name_tokens[0], '_');
				ComPtr<IDxcBlobUtf16> name = nullptr;

				for (size_t i = 0; i < shader_types.size(); ++i)
				{
					auto base_name = shader_types[i];
					auto obj_name = "\\" + base_name + ".cso";
					auto output_cso = std::wstring(tmp.begin(), tmp.end());// +std::wstring(std::begin(obj_name), std::end(obj_name));

					auto pdb_name = "\\" + base_name + ".pdb";
					auto output_pdb = std::wstring(tmp.begin(), tmp.end());//+ std::wstring(std::begin(pdb_name), std::end(pdb_name));

					auto ref_name = "\\" + base_name + ".ref";
					auto output_ref = std::wstring(tmp.begin(), tmp.end());//+ std::wstring(std::begin(ref_name), std::end(ref_name));

					if (shader_types[i] == "as")
					{
						LPCWSTR args[] = {
							file.path().c_str(),
							L"-Fo",
							output_cso.c_str(),
							L"-Fd",
							output_pdb.c_str(),
							L"-Fre",
							output_ref.c_str(),
							L"-E",
							L"asmain",
							L"-T",
							L"as_6_8",
							L"-Zi",
#ifdef DEBUG
							L"-Od",
#endif
							L"-Qstrip_reflect",
						};

						ComPtr<IDxcResult> results = nullptr;
						DX_CHECK("compile shader source", compiler->Compile(&source, args, _countof(args), include_handler.Get(), IID_PPV_ARGS(&results)));

						ComPtr<IDxcBlobEncoding> errors = nullptr;
						DX_CHECK("get error buffer", results->GetErrorBuffer(&errors));

						if (errors != nullptr && errors->GetBufferSize() > 0)
						{
							std::cout << __LINE__ << " " << reinterpret_cast<char*>(errors->GetBufferPointer()) << '\n';
						}
						DX_CHECK("get shader binary", results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&as), &name));

						ComPtr<IDxcBlob> pdb = nullptr;
						DX_CHECK("get pdb", results->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdb), &name));
						std::wcout << "pdb name " << name->GetStringPointer() << '\n';

						std::ofstream pdb_file(std::wstring(tmp.begin(), tmp.end()) + std::wstring(name->GetStringPointer()), std::ios_base::binary);
						pdb_file.write(reinterpret_cast<const char*>(pdb->GetBufferPointer()), pdb->GetBufferSize());
						pdb_file.close();

						ComPtr<IDxcBlob> reflection = nullptr;
						DX_CHECK("get reflection", results->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflection), &name));
						std::wcout << "ref name " << name->GetStringPointer() << '\n';
					}
					else if (shader_types[i] == "ms")
					{
						LPCWSTR args[] = {
							file.path().c_str(),
							L"-Fo",
							output_cso.c_str(),
							L"-Fd",
							output_pdb.c_str(),
							L"-Fre",
							output_ref.c_str(),
							L"-E",
							L"msmain",
							L"-T",
							L"ms_6_8",
							L"-Zi",
	#ifdef DEBUG
							L"-Od",
#endif
							L"-Qstrip_reflect",
						};

						ComPtr<IDxcResult> results = nullptr;
						DX_CHECK("compile shader source", compiler->Compile(&source, args, _countof(args), include_handler.Get(), IID_PPV_ARGS(&results)));

						ComPtr<IDxcBlobEncoding> errors = nullptr;
						DX_CHECK("get error buffer", results->GetErrorBuffer(&errors));

						if (errors != nullptr && errors->GetBufferSize() > 0)
						{
							std::cout << __LINE__ << " " << reinterpret_cast<char*>(errors->GetBufferPointer()) << '\n';
						}
						DX_CHECK("get shader binary", results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ms), &name));

						std::ofstream bin_file(std::wstring(tmp.begin(), tmp.end()) + name->GetStringPointer());
						bin_file.write(reinterpret_cast<const char*>(ms->GetBufferPointer()), ms->GetBufferSize());
						bin_file.close();

						ComPtr<IDxcBlob> pdb = nullptr;
						DX_CHECK("get pdb", results->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdb), &name));
						std::wcout << "pdb name " << name->GetStringPointer() << '\n';

						std::ofstream pdb_file(std::wstring(tmp.begin(), tmp.end()) + std::wstring(name->GetStringPointer()), std::ios_base::binary);
						pdb_file.write(reinterpret_cast<const char*>(pdb->GetBufferPointer()), pdb->GetBufferSize());
						pdb_file.close();

						ComPtr<IDxcBlob> reflection = nullptr;
						DX_CHECK("get reflection", results->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflection), &name));
						std::wcout << "ref name " << name->GetStringPointer() << '\n';
					}
					else if (shader_types[i] == "ps")
					{
						LPCWSTR args[] = {
							file.path().c_str(),
							L"-Fo",
							output_cso.c_str(),
							L"-Fd",
							output_pdb.c_str(),
							L"-Fre",
							output_ref.c_str(),
							L"-E",
							L"psmain",
							L"-T",
							L"ps_6_8",
							L"-Zi",
	#ifdef DEBUG
							L"-Od",
#endif
							L"-Qstrip_reflect",
						};

						ComPtr<IDxcResult> results = nullptr;
						DX_CHECK("compile shader source", compiler->Compile(&source, args, _countof(args), include_handler.Get(), IID_PPV_ARGS(&results)));

						ComPtr<IDxcBlobEncoding> errors = nullptr;
						DX_CHECK("get error buffer", results->GetErrorBuffer(&errors));

						if (errors != nullptr && errors->GetBufferSize() > 0)
						{
							std::cout << __LINE__ << " " << reinterpret_cast<char*>(errors->GetBufferPointer()) << '\n';
						}
						DX_CHECK("get shader binary", results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ps), &name));
						std::wcout << "bin name " << name->GetStringPointer() << '\n';

						std::ofstream bin_file(std::wstring(tmp.begin(), tmp.end()) + name->GetStringPointer());
						bin_file.write(reinterpret_cast<const char*>(ps->GetBufferPointer()), ps->GetBufferSize());
						bin_file.close();

						ComPtr<IDxcBlob> pdb = nullptr;
						DX_CHECK("get pdb", results->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdb), &name));
						std::wcout << "pdb name " << name->GetStringPointer() << '\n';

						std::ofstream pdb_file(std::wstring(tmp.begin(), tmp.end()) + name->GetStringPointer(), std::ios_base::binary);
						pdb_file.write(reinterpret_cast<const char*>(pdb->GetBufferPointer()), pdb->GetBufferSize());
						pdb_file.close();

						ComPtr<IDxcBlob> reflection = nullptr;
						DX_CHECK("get reflection", results->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflection), &name));
						std::wcout << "ref name " << name->GetStringPointer() << '\n';
					}
				}
			}
		}
	}

	dx12_renderer::pipeline pipeline = {};
	DX_CHECK("create root signature", device10->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), root_sig_blob->GetBufferSize(), IID_PPV_ARGS(&pipeline.root_signature)));

	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC pipeline_desc = {
		.pRootSignature = pipeline.root_signature.Get(),
		.MS = {
			.pShaderBytecode = ms->GetBufferPointer(),
			.BytecodeLength = ms->GetBufferSize(),
		},
		.PS = {
			.pShaderBytecode = ps->GetBufferPointer(),
			.BytecodeLength = ps->GetBufferSize(),
		},
		.BlendState = {
			.RenderTarget = {
				{
					.BlendEnable = TRUE,
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
		//.DepthStencilState = {
		//	.DepthEnable = TRUE,
		//	.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
		//},
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

	DX_CHECK("create pipeline", device10->CreatePipelineState(&pipeline_stream_desc, IID_PPV_ARGS(&pipeline.state)));

	pipelines.push_back(pipeline);
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

	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory7->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter4)); ++i)
	{
		DXGI_ADAPTER_DESC3 adapter_desc3;
		adapter4->GetDesc3(&adapter_desc3);

		if (adapter_desc3.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(reinterpret_cast<IUnknown*>(adapter4.Get()), D3D_FEATURE_LEVEL_12_2, IID_ID3D12Device10, nullptr)))
		{
			break;
		}
	}

	DX_CHECK("create device", D3D12CreateDevice(reinterpret_cast<IUnknown*>(adapter4.Get()), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device10)));

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

	D3D12_FEATURE_DATA_SHADER_MODEL shader_model = { D3D_SHADER_MODEL_6_8 };
	DX_CHECK("check shader model support", device10->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shader_model, sizeof(shader_model)));

	if (shader_model.HighestShaderModel < D3D_SHADER_MODEL_6_8)
	{
		std::cout << "Shader Model 6.8 not supported\n";
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

	DX_CHECK("create sc cmd q", device10->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&sc_cmd_queue)));
	DX_CHECK("create gnrl cmd q", device10->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&gnrl_cmd_queue)));

	GetWindowRect(h_wnd, &wnd_rect);
	wnd_rect = SANITIZE_RECT_FOR_RENDER(wnd_rect);
	viewport = RECT_TO_VIEWPORT(wnd_rect);

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
	DX_CHECK("create swapchain", factory7->CreateSwapChainForHwnd(sc_cmd_queue.Get(), h_wnd, &sc_desc1, nullptr, nullptr, &swapchain1));
	swapchain1.As(&swapchain4);

	const D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = sc_desc1.BufferCount,
	};
	DX_CHECK("create rtv desc heap", device10->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_desc_heap)));

	const D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		.NumDescriptors = sc_desc1.BufferCount,
	};

	D3D12_CPU_DESCRIPTOR_HANDLE rtv_desc_heap_hnd = rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();

	for (UINT f = 0; f < RENDER_TARGET_COUNT; ++f)
	{
		DX_CHECK("get render target buffer", swapchain4->GetBuffer(f, IID_PPV_ARGS(&sc_rt[f])));

		const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
		};

		rtv_desc_heap_hnd.ptr += SIZE_T(f * device10->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		device10->CreateRenderTargetView(sc_rt[f].Get(), &rtv_desc, rtv_desc_heap_hnd);

		const D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
			.Format = DXGI_FORMAT_D32_FLOAT,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		};

		DX_CHECK("create sc cmd allocator", device10->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&sc_rt_cmd_allocs[f])));
		DX_CHECK("create sc cmd list", device10->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&sc_rt_cmd_lists[f])));
		DX_CHECK("create sc fence", device10->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&sc_rt_fncs[f])));

		sc_rt_fnc_vals[f] = 0;
	}

	DX_CHECK("create gnrl cmd alloc", device10->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gnrl_cmd_alloc)));
	DX_CHECK("create gnrl cmd list", device10->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&gnrl_cmd_list)));
	DX_CHECK("create grnl fnc", device10->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gnrl_fnc)));

	img_idx = 0;
	gnrl_fnc_val = 0;

	is_inited = true;
}

void dx12_renderer::resize(const RECT& rect)
{
	wnd_rect = SANITIZE_RECT_FOR_RENDER(rect);
	viewport = RECT_TO_VIEWPORT(wnd_rect);

	for (UINT f = 0; f < RENDER_TARGET_COUNT; ++f)
	{
		if (sc_rt_fncs[f]->GetCompletedValue() < sc_rt_fnc_vals[f])
		{
			HANDLE wait_idle_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);

			if (wait_idle_event != nullptr)
			{
				DX_CHECK("set wait idle event", sc_rt_fncs[f]->SetEventOnCompletion(sc_rt_fnc_vals[f], wait_idle_event));

				WaitForSingleObject(wait_idle_event, DWORD_MAX);
				CloseHandle(wait_idle_event);
			}
		}
		sc_rt[f].Reset();
	}

	DX_CHECK("swapchain resize buffers", swapchain4->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

	D3D12_CPU_DESCRIPTOR_HANDLE rtv_desc_heap_hnd = rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
	const D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
	};

	for (UINT f = 0; f < RENDER_TARGET_COUNT; ++f)
	{
		DX_CHECK("swapchain get buffer", swapchain4->GetBuffer(f, IID_PPV_ARGS(&sc_rt[f])));

		rtv_desc_heap_hnd.ptr += SIZE_T(f * device10->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		device10->CreateRenderTargetView(sc_rt[f].Get(), &rtv_desc, rtv_desc_heap_hnd);
	}
}

void dx12_renderer::begin_frame()
{
	img_idx = swapchain4->GetCurrentBackBufferIndex();

	if (sc_rt_fncs[img_idx]->GetCompletedValue() < sc_rt_fnc_vals[img_idx])
	{
		HANDLE wait_idle_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);

		if (wait_idle_event != nullptr)
		{
			DX_CHECK("set wait idle event", sc_rt_fncs[img_idx]->SetEventOnCompletion(sc_rt_fnc_vals[img_idx], wait_idle_event));

			WaitForSingleObject(wait_idle_event, DWORD_MAX);
			CloseHandle(wait_idle_event);
		}
	}

	DX_CHECK("reset command allocator", sc_rt_cmd_allocs[img_idx]->Reset());
	DX_CHECK("reset command list", sc_rt_cmd_lists[img_idx]->Reset(sc_rt_cmd_allocs[img_idx].Get(), nullptr));

	const D3D12_RESOURCE_BARRIER prsnt_to_rt_barr = {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Transition = {
			.pResource = sc_rt[img_idx].Get(),
			.Subresource = 0,
			.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
			.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		},
	};

	sc_rt_cmd_lists[img_idx]->ResourceBarrier(1, &prsnt_to_rt_barr);
}

void dx12_renderer::clear_frame(const float color[])
{
	D3D12_CPU_DESCRIPTOR_HANDLE curr_rtv_hnd = rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
	curr_rtv_hnd.ptr += (img_idx * device10->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	sc_rt_cmd_lists[img_idx]->OMSetRenderTargets(1, &curr_rtv_hnd, FALSE, nullptr);
	sc_rt_cmd_lists[img_idx]->ClearRenderTargetView(curr_rtv_hnd, color, 1, &wnd_rect);
}

void dx12_renderer::render_world()
{
	sc_rt_cmd_lists[img_idx]->RSSetScissorRects(1, &wnd_rect);
	sc_rt_cmd_lists[img_idx]->RSSetViewports(1, &viewport);
	sc_rt_cmd_lists[img_idx]->SetGraphicsRootSignature(pipelines[0].root_signature.Get());
	sc_rt_cmd_lists[img_idx]->SetPipelineState(pipelines[0].state.Get());

	auto P = DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(60), viewport.Width / viewport.Height, 0.1, 100.0);
	auto V = DirectX::XMMatrixLookAtRH(DirectX::FXMVECTOR{ 0, 0, 10 }, DirectX::FXMVECTOR{ 0, 0, 0 }, DirectX::FXMVECTOR{ 0, 1, 0 });
	auto VP = V * P;

	for (auto const& mi : sd->material_infos)
	{
		for (auto const& pd : mi.prims_data)
		{
			auto MVP = pd.xform * VP;

			sc_rt_cmd_lists[img_idx]->SetGraphicsRoot32BitConstants(0, 16, MVP.r, 0);
			sc_rt_cmd_lists[img_idx]->SetGraphicsRootShaderResourceView(1, pd.geometry_buffers[0]->GetGPUVirtualAddress());
			sc_rt_cmd_lists[img_idx]->SetGraphicsRootShaderResourceView(2, pd.geometry_buffers[1]->GetGPUVirtualAddress());
			sc_rt_cmd_lists[img_idx]->SetGraphicsRootShaderResourceView(3, pd.geometry_buffers[2]->GetGPUVirtualAddress());
			sc_rt_cmd_lists[img_idx]->SetGraphicsRootShaderResourceView(4, pd.geometry_buffers[3]->GetGPUVirtualAddress());
			sc_rt_cmd_lists[img_idx]->DispatchMesh((UINT)pd.meshlets_count, 1, 1);
		}
	}
}

void dx12_renderer::end_frame()
{
	const D3D12_RESOURCE_BARRIER rt_to_prsnt_barr = {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Transition = {
			.pResource = sc_rt[img_idx].Get(),
			.Subresource = 0,
			.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
			.StateAfter = D3D12_RESOURCE_STATE_PRESENT,
		},
	};
	sc_rt_cmd_lists[img_idx]->ResourceBarrier(1, &rt_to_prsnt_barr);
	DX_CHECK("close cmd list", sc_rt_cmd_lists[img_idx]->Close());

	ID3D12CommandList* cmd_lists[] = {
		sc_rt_cmd_lists[img_idx].Get(),
	};

	sc_cmd_queue->ExecuteCommandLists(1, cmd_lists);
	DX_CHECK("swapchain present", swapchain4->Present(0, 0));

	++sc_rt_fnc_vals[img_idx];
	sc_cmd_queue->Signal(sc_rt_fncs[img_idx].Get(), sc_rt_fnc_vals[img_idx]);
}

void dx12_renderer::import_scene_data(const cgltf_data* data)
{
	sd = std::make_unique<dx12_renderer::scene_data>();

	for (cgltf_size n = 0; n < data->nodes_count; ++n)
	{
		cgltf_node* curr_node = data->nodes + n;

		if (curr_node->mesh == nullptr)
			continue;

		cgltf_mesh* curr_mesh = curr_node->mesh;

		for (cgltf_size p = 0; p < curr_mesh->primitives_count; ++p)
		{
			primitive_data pd = {
				.xform = DirectX::XMMatrixIdentity(),
			};

			if (curr_node->has_matrix)
			{
				pd.xform = DirectX::XMMatrixTranspose(DirectX::XMMATRIX(curr_node->matrix));
			}
			else {
				if (curr_node->has_translation)
				{
					pd.xform *= DirectX::XMMatrixTranslation(curr_node->translation[0], curr_node->translation[1], curr_node->translation[2]);
				}

				if (curr_node->has_rotation)
				{
					auto quat = DirectX::FXMVECTOR();
					quat.m128_f32[0] = curr_node->rotation[0];
					quat.m128_f32[1] = curr_node->rotation[1];
					quat.m128_f32[2] = curr_node->rotation[2];
					quat.m128_f32[3] = curr_node->rotation[3];

					pd.xform *= DirectX::XMMatrixRotationQuaternion(quat);
				}

				if (curr_node->has_scale)
				{
					pd.xform *= DirectX::XMMatrixScaling(curr_node->scale[0], curr_node->scale[1], curr_node->scale[2]);
				}
			}

			std::vector<uint32_t> indices;
			std::vector<float3> positions;

			cgltf_primitive* curr_prim = curr_mesh->primitives + p;
			if (curr_prim->material == nullptr)
				continue;

			if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
			{
				size_t curr_ind_size = indices.size();
				indices.resize(indices.size() + curr_prim->indices->count);
				std::memcpy(indices.data() + curr_ind_size, (void*)((ULONG_PTR)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->offset + curr_prim->indices->buffer_view->offset), curr_prim->indices->buffer_view->size);
			}
			else if (curr_prim->indices->component_type == cgltf_component_type_r_16u)
			{
				indices.reserve(indices.size() + curr_prim->indices->count);
				uint16_t* idx_ptr = (uint16_t*)((ULONG_PTR)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->offset + curr_prim->indices->buffer_view->offset);

				for (cgltf_size i = 0; i < curr_prim->indices->count; ++i)
				{
					indices.push_back(idx_ptr[i]);
				}
			}

			for (cgltf_size a = 0; a < curr_prim->attributes_count; ++a)
			{
				cgltf_attribute* curr_attr = curr_prim->attributes + a;

				if (std::strcmp(curr_attr->name, "POSITION") == 0)
				{
					size_t curr_geom_size = positions.size();
					positions.resize(positions.size() + curr_attr->data->count);

					std::memcpy(&positions[curr_geom_size], (void*)((ULONG_PTR)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * sizeof(float3));
				}
			}

			size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), MAX_VERTICES, MAX_TRIANGLES);

			std::vector<meshopt_Meshlet> meshlets(max_meshlets);
			std::vector<uint32_t> meshlets_vertices(max_meshlets * MAX_VERTICES);
			std::vector<uint8_t> meshlet_triangles(max_meshlets * MAX_TRIANGLES);

			pd.meshlets_count = meshopt_buildMeshlets(meshlets.data(),
				meshlets_vertices.data(),
				meshlet_triangles.data(),
				indices.data(),
				indices.size(),
				reinterpret_cast<float*>(positions.data()),
				positions.size(),
				sizeof(float3),
				MAX_VERTICES,
				MAX_TRIANGLES,
				0.0);

			auto last_meshlet = meshlets[pd.meshlets_count - 1];
			meshlets_vertices.resize(last_meshlet.vertex_offset + last_meshlet.vertex_count);
			meshlet_triangles.resize(last_meshlet.triangle_offset + ((last_meshlet.triangle_count * 3 + 3) & ~3));
			meshlets.resize(pd.meshlets_count);

			std::vector<uint32_t> meshlet_triangles_32;
			size_t t_32_idx = 0;

			for (auto& meshlet : meshlets)
			{
				uint32_t triangle_offset = static_cast<uint32_t>(meshlet_triangles_32.size());

				for (size_t t = 0; t < meshlet.triangle_count; ++t)
				{
					uint32_t curr_tri = (static_cast<uint32_t>(meshlet_triangles[meshlet.triangle_offset + (t * 3)]) << 0) |
						(static_cast<uint32_t>(meshlet_triangles[meshlet.triangle_offset + (t * 3 + 1)]) << 8) |
						(static_cast<uint32_t>(meshlet_triangles[meshlet.triangle_offset + (t * 3 + 2)]) << 16);

					meshlet_triangles_32.push_back(curr_tri);
				};

				meshlet.triangle_offset = triangle_offset;
			}

			size_t meshlets_data_size = meshlets.size() * sizeof(meshlets[0]);
			std::vector<uint8_t> meshlets_data(meshlets_data_size);
			std::memcpy(meshlets_data.data(), meshlets.data(), meshlets_data_size);

			size_t positions_data_size = positions.size() * sizeof(positions[0]);
			std::vector<uint8_t> positions_data(positions_data_size);
			std::memcpy(positions_data.data(), positions.data(), positions_data_size);

			size_t meshlets_vertices_data_size = meshlets_vertices.size() * sizeof(meshlets_vertices[0]);
			std::vector<uint8_t> meshlets_vertices_data(meshlets_vertices_data_size);
			std::memcpy(meshlets_vertices_data.data(), meshlets_vertices.data(), meshlets_vertices_data_size);

			size_t meshlets_triangles_data_size = meshlet_triangles_32.size() * sizeof(meshlet_triangles_32[0]);
			std::vector<uint8_t> meshlets_triangles_data(meshlets_triangles_data_size);
			std::memcpy(meshlets_triangles_data.data(), meshlet_triangles_32.data(), meshlets_triangles_data_size);

			auto buffers_heap = CreateDefaultBuffersAndHeapFromData(
				{
					CD3DX12_RESOURCE_DESC1::Buffer(positions_data_size),
					CD3DX12_RESOURCE_DESC1::Buffer(meshlets_data_size),
					CD3DX12_RESOURCE_DESC1::Buffer(meshlets_vertices_data_size),
					CD3DX12_RESOURCE_DESC1::Buffer(meshlets_triangles_data_size),
				},
				{
					positions_data,
					meshlets_data,
					meshlets_vertices_data,
					meshlets_triangles_data,
				}
				);
			pd.geometry_buffers = buffers_heap.first;
			pd.geometry_heap = buffers_heap.second;

			uint64_t id = std::hash<std::string>{}(curr_prim->material->name);
			auto it = std::find(sd->material_infos.begin(), sd->material_infos.end(), id);
			if (it == sd->material_infos.end())
			{
				material_info mi = {
					.id = id,
					.prims_data = std::vector<primitive_data> {pd},
				};
				sd->material_infos.push_back(mi);
			}
			else
			{
				it->prims_data.push_back(pd);
			}
		}
	}

	create_pipelines();
}

void dx12_renderer::clear_scene_data()
{
	if (sc_rt_fncs[img_idx]->GetCompletedValue() < sc_rt_fnc_vals[img_idx])
	{
		HANDLE wait_idle_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);

		if (wait_idle_event != nullptr)
		{
			DX_CHECK("set wait idle event", sc_rt_fncs[img_idx]->SetEventOnCompletion(sc_rt_fnc_vals[img_idx], wait_idle_event));

			WaitForSingleObject(wait_idle_event, DWORD_MAX);
			CloseHandle(wait_idle_event);
		}
	}

	sd.reset();
}

dx12_renderer::~dx12_renderer()
{
	if (sc_rt_fncs[img_idx]->GetCompletedValue() < sc_rt_fnc_vals[img_idx])
	{
		HANDLE wait_idle_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);

		if (wait_idle_event != nullptr)
		{
			DX_CHECK("set wait idle event", sc_rt_fncs[img_idx]->SetEventOnCompletion(sc_rt_fnc_vals[img_idx], wait_idle_event));

			WaitForSingleObject(wait_idle_event, DWORD_MAX);
			CloseHandle(wait_idle_event);
		}
	}
}
