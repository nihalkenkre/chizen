#![allow(non_snake_case)]
#![allow(non_camel_case_types)]
#![allow(non_upper_case_globals)]
#![allow(unused_variables)]
#![allow(unused_assignments)]

use core::panic;
use std::{ffi::CStr, os::raw::c_void, process::Command, u64};

mod backend;
use backend::*;

#[macro_export]
macro_rules! VK_MAKE_API_VERSION {
    ($variant:literal, $major:literal, $minor:literal, $patch:literal) => {
        $variant << 29_u32 | $major << 22_u32 | $minor << 12_u32 | $patch
    };
}

#[macro_export]
macro_rules! VK_API_VERSION_VARIANT {
    ($x:expr) => {
        $x >> 29_u32
    };
}

#[macro_export]
macro_rules! VK_API_VERSION_MAJOR {
    ($x:expr) => {
        $x >> 22_u32 & 0x7F_u32
    };
}

#[macro_export]
macro_rules! VK_API_VERSION_MINOR {
    ($x:expr) => {
        $x >> 12_u32 & 0x3FF_u32
    };
}

#[macro_export]
macro_rules! VK_API_VERSION_PATCH {
    ($x:expr) => {
        $x & 0xFFF_u32
    };
}

pub const VK_KHR_SURFACE_EXTENSION_NAME: &CStr = c"VK_KHR_surface";
pub const VK_KHR_WIN32_SURFACE_EXTENSION_NAME: &CStr = c"VK_KHR_win32_surface";
pub const VK_KHR_SWAPCHAIN_EXTENSION_NAME: &CStr = c"VK_KHR_swapchain";

#[derive(Clone)]
pub struct ImageInfo {
    req_mem_types: VkMemoryPropertyFlags,
    extent: VkExtent3D,
    format: VkFormat,
    usage: VkImageUsageFlags,
    aspect: VkImageAspectFlags,
    img_type: VkImageType,
    view_type: VkImageViewType,
    sharing_mode: VkSharingMode,
    samples: VkSampleCountFlags,
    tiling: VkImageTiling,
}

pub struct Vk {
    pub lyt_chng_cmd_buff: CommandBuffer,
    pub sc_cmd_buff: CommandBuffer,
    pub sc_cmd_pool: CommandPool,
    pub vert_sh_mod: ShaderModule,
    pub frag_sh_mod: ShaderModule,
    pub sc_depth_imgs: Vec<Image>,
    pub sc_depth_img_vs: Vec<ImageView>,
    pub sc_image_views: Vec<ImageView>,
    pub swapchain: Swapchain,
    pub device: Device,
    pub surface: Surface,
    pub instance: Instance,
    pub phy_dev: PhysicalDevice,
    pub surf_extent: VkExtent2D,
    pub sc_images: Vec<VkImage>,
    pub sc_depth_img_mems: Vec<VkDeviceMemory>,
    pub desc_set_lyts: Vec<VkDescriptorSetLayout>,
    pub desc_pool: VkDescriptorPool,
    pub pipe_lyt: VkPipelineLayout,
    pub view_pipes: Vec<VkPipeline>,
    pub img_lyt_chng_fnc: VkFence,
    pub acq_sem: VkSemaphore,
    pub sc_img_lyt_chng_sems: Vec<VkSemaphore>,
    pub rndr_sems: Vec<VkSemaphore>,
    pub uni_buff: VkBuffer,
    pub uni_mem: VkDeviceMemory,
    pub min_uni_buff_align: VkDeviceSize,
    pub q_fly_idx: u32,
    pub gfx_q: Queue,
    pub xfer_q: Queue,
}

impl Default for Vk {
    fn default() -> Self {
        Self {
            instance: Instance::default(),
            phy_dev: PhysicalDevice {
                phy_dev: std::ptr::null_mut(),
            },
            surface: Surface::default(),
            surf_extent: VkExtent2D {
                width: 0,
                height: 0,
            },
            device: Device::default(),
            swapchain: Swapchain::default(),
            sc_images: vec![],
            sc_image_views: vec![],
            sc_depth_imgs: vec![],
            sc_depth_img_mems: vec![],
            sc_depth_img_vs: vec![],
            vert_sh_mod: ShaderModule::default(),
            frag_sh_mod: ShaderModule::default(),
            desc_set_lyts: vec![],
            desc_pool: std::ptr::null_mut(),
            pipe_lyt: std::ptr::null_mut(),
            view_pipes: vec![],
            sc_cmd_pool: CommandPool::default(),
            sc_cmd_buff: CommandBuffer::default(),
            lyt_chng_cmd_buff: CommandBuffer::default(),
            img_lyt_chng_fnc: std::ptr::null_mut(),
            acq_sem: std::ptr::null_mut(),
            sc_img_lyt_chng_sems: vec![],
            rndr_sems: vec![],
            uni_buff: std::ptr::null_mut(),
            uni_mem: std::ptr::null_mut(),
            min_uni_buff_align: 0,
            q_fly_idx: 0,
            gfx_q: Queue {
                q: std::ptr::null_mut(),
            },
            xfer_q: Queue {
                q: std::ptr::null_mut(),
            },
        }
    }
}

impl Vk {
    pub fn init(window_handle: winit::raw_window_handle::RawWindowHandle) -> Self {
        // let ai = VkApplicationInfo::new(
        //     None,
        //     c"Chizen",
        //     VK_MAKE_API_VERSION!(0, 1, 0, 0),
        //     c"Chizen",
        //     VK_MAKE_API_VERSION!(0, 1, 0, 0),
        //     VK_MAKE_API_VERSION!(0, 1, 3, 280),
        // );

        let inst_ext_names = vec![
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        ];

        // let inst_ci = VkInstanceCreateInfo::new(None, 0, ai, &inst_ext_names);

        let instance = match Instance::create(
            &VkInstanceCreateInfo::new(
                None,
                0,
                VkApplicationInfo::new(
                    None,
                    c"Chizen",
                    VK_MAKE_API_VERSION!(0, 1, 0, 0),
                    c"Chizen",
                    VK_MAKE_API_VERSION!(0, 1, 0, 0),
                    VK_MAKE_API_VERSION!(0, 1, 3, 280),
                ),
                &inst_ext_names,
            ),
            None,
        ) {
            Ok(instance) => instance,
            Err(result) => {
                panic!("ERR Create instance {}", result);
            }
        };

        // let surface_ci = VkWin32SurfaceCreateInfoKHR::new(
        //     None,
        //     0,
        //     match window_handle {
        //         winit::raw_window_handle::RawWindowHandle::Win32(window) => {
        //             window.hinstance.unwrap().get() as *mut HINSTANCE__
        //         }
        //         _ => std::ptr::null_mut(),
        //     },
        //     match window_handle {
        //         winit::raw_window_handle::RawWindowHandle::Win32(window) => {
        //             window.hwnd.get() as *mut HWND__
        //         }
        //         _ => std::ptr::null_mut(),
        //     },
        // );

        // let surface = match instance.create_win32_surface_khr(&surface_ci, None) {
        //     Ok(surface) => surface,
        //     Err(result) => {
        //         panic!("ERR create surface {}", result);
        //     }
        // };

        let surface = match Surface::create(
            instance.instance,
            &VkWin32SurfaceCreateInfoKHR::new(
                None,
                0,
                match window_handle {
                    winit::raw_window_handle::RawWindowHandle::Win32(window) => {
                        window.hinstance.unwrap().get() as *mut HINSTANCE__
                    }
                    _ => std::ptr::null_mut(),
                },
                match window_handle {
                    winit::raw_window_handle::RawWindowHandle::Win32(window) => {
                        window.hwnd.get() as *mut HWND__
                    }
                    _ => std::ptr::null_mut(),
                },
            ),
            None,
        ) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR creating surface {}", err);
            }
        };

        let (phy_dev, q_fly_q_cnt, q_fly_idx, min_sto_buff_align, min_uni_buff_align) =
            match instance.get_physical_devices().iter().find(|pd| {
                let phy_dev_props = pd.properties();
                phy_dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                    || phy_dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
            }) {
                Some(pd) => {
                    match pd.queue_family_properties().iter().enumerate().find(
                        |(q_fly_idx, q_fly_prop)| {
                            (q_fly_prop.queueFlags & VK_QUEUE_GRAPHICS_BIT as u32)
                                == VK_QUEUE_GRAPHICS_BIT as u32
                                && match pd.surface_support_khr(*q_fly_idx, surface.surface) {
                                    Ok(x) => x,
                                    Err(err) => {
                                        panic!(
                                            "ERR get physical device surface support khr {}",
                                            err
                                        )
                                    }
                                }
                                && pd.win32_presentation_support_khr(*q_fly_idx)
                        },
                    ) {
                        Some((q_idx, q_fly_prop)) => {
                            let phy_dev_props = pd.properties();

                            (
                                *pd,
                                q_fly_prop.queueCount,
                                q_idx as u32,
                                phy_dev_props.limits.minStorageBufferOffsetAlignment,
                                phy_dev_props.limits.minUniformBufferOffsetAlignment,
                            )
                        }
                        None => {
                            panic!("ERR could not find suitable physical device");
                        }
                    }
                }
                None => {
                    panic!("ERR could not find suitable physical device");
                }
            };

        let surf_caps = match phy_dev.surface_capabilities_khr(&surface.surface) {
            Ok(caps) => caps,
            Err(result) => {
                panic!("ERR get surface capabilities {}", result);
            }
        };

        let surf_format = match phy_dev.surface_formats_khr(&surface.surface) {
            Ok(forms) => match forms
                .iter()
                .find(|sf| sf.format == VK_FORMAT_R8G8B8A8_UNORM)
            {
                Some(x) => *x,
                None => VkSurfaceFormatKHR::default(),
            },
            Err(result) => {
                panic!("ERR get surface formats {}", result);
            }
        };

        let present_mode = match phy_dev.surface_present_modes_khr(&surface.surface) {
            Ok(present_modes) => match present_modes
                .iter()
                .find(|pm| **pm == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                Some(x) => *x,
                None => VK_PRESENT_MODE_FIFO_KHR,
            },

            Err(result) => {
                panic!("ERR get surface present modes {}", result);
            }
        };

        // let priorities: Vec<f32> = vec![1.0; q_fly_q_cnt as usize];
        // let q_ci = VkDeviceQueueCreateInfo::new(None, 0, q_fly_idx, &priorities);
        // let dev_exts = vec![VK_KHR_SWAPCHAIN_EXTENSION_NAME];
        let feats2 = phy_dev.features2();
        let dyn_rend_feats = VkPhysicalDeviceDynamicRenderingFeatures::new(Some(
            std::ptr::addr_of!(feats2) as *mut c_void,
        ));
        // let q_cis = vec![q_ci];
        // let device_ci = VkDeviceCreateInfo::new(
        //     Some(std::ptr::addr_of!(dyn_rend_feats) as *const c_void),
        //     0,
        //     &q_cis,
        //     &dev_exts,
        //     None,
        // );

        let device = match Device::create(
            &phy_dev,
            &VkDeviceCreateInfo::new(
                Some(std::ptr::addr_of!(dyn_rend_feats) as *const c_void),
                0,
                &vec![VkDeviceQueueCreateInfo::new(
                    None,
                    0,
                    q_fly_idx,
                    &vec![1.0; q_fly_q_cnt as usize],
                )],
                &vec![VK_KHR_SWAPCHAIN_EXTENSION_NAME],
                None,
            ),
            None,
        ) {
            Ok(device) => device,
            Err(result) => {
                panic!("ERR Create device {}", result);
            }
        };

        // let q_fly_idxs = vec![q_fly_idx];
        // let sc_ci = VkSwapchainCreateInfoKHR::new(
        //     None,
        //     0,
        //     surface.surface,
        //     surf_caps.minImageCount + 1,
        //     surf_format.format,
        //     surf_format.colorSpace,
        //     surf_caps.currentExtent,
        //     1,
        //     surf_caps.supportedUsageFlags,
        //     VK_SHARING_MODE_EXCLUSIVE,
        //     &q_fly_idxs,
        //     surf_caps.currentTransform,
        //     VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        //     present_mode,
        //     1,
        //     None,
        // );

        // let swapchain = match device.create_swapchain_khr(
        //     &VkSwapchainCreateInfoKHR::new(
        //         None,
        //         0,
        //         surface.surface,
        //         surf_caps.minImageCount + 1,
        //         surf_format.format,
        //         surf_format.colorSpace,
        //         surf_caps.currentExtent,
        //         1,
        //         surf_caps.supportedUsageFlags,
        //         VK_SHARING_MODE_EXCLUSIVE,
        //         &vec![q_fly_idx],
        //         surf_caps.currentTransform,
        //         VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        //         present_mode,
        //         1,
        //         None,
        //     ),
        //     None,
        // ) {
        //     Ok(sc) => sc,
        //     Err(result) => {
        //         panic!("ERR create swapchain {}", result);
        //     }
        // };

        let swapchain = match Swapchain::create(
            device.device,
            &VkSwapchainCreateInfoKHR::new(
                None,
                0,
                surface.surface,
                surf_caps.minImageCount + 1,
                surf_format.format,
                surf_format.colorSpace,
                surf_caps.currentExtent,
                1,
                surf_caps.supportedUsageFlags,
                VK_SHARING_MODE_EXCLUSIVE,
                &vec![q_fly_idx],
                surf_caps.currentTransform,
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                present_mode,
                1,
                None,
            ),
            None,
        ) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR create swapchain {}", err);
            }
        };

        let sc_images = match swapchain.get_images() {
            Ok(imgs) => imgs,
            Err(result) => {
                panic!("ERR get swapchain images {}", result);
            }
        };

        let mut sc_image_views = Vec::with_capacity(sc_images.len());

        // let mut iv_ci = VkImageViewCreateInfo::new(
        //     None,
        //     0,
        //     None,
        //     VK_IMAGE_VIEW_TYPE_2D,
        //     surf_format.format,
        //     VkComponentMapping::default(),
        //     VkImageSubresourceRange {
        //         aspectMask: VK_IMAGE_ASPECT_COLOR_BIT as u32,
        //         levelCount: 1,
        //         layerCount: 1,
        //         ..Default::default()
        //     },
        // );

        for sc_img in &sc_images {
            sc_image_views.push(
                match ImageView::create(
                    device.device,
                    &VkImageViewCreateInfo::new(
                        None,
                        0,
                        *sc_img,
                        VK_IMAGE_VIEW_TYPE_2D,
                        surf_format.format,
                        VkComponentMapping::default(),
                        VkImageSubresourceRange {
                            aspectMask: VK_IMAGE_ASPECT_COLOR_BIT as u32,
                            levelCount: 1,
                            layerCount: 1,
                            ..Default::default()
                        },
                    ),
                    None,
                ) {
                    Ok(iv) => iv,
                    Err(result) => {
                        panic!("ERR create image view {}", result);
                    }
                },
            );
        }

        let sc_d_img_info = ImageInfo {
            aspect: VK_IMAGE_ASPECT_DEPTH_BIT as u32,
            req_mem_types: VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT as u32,
            extent: VkExtent3D {
                width: surf_caps.currentExtent.width,
                height: surf_caps.currentExtent.height,
                depth: 1,
            },
            samples: VK_SAMPLE_COUNT_1_BIT as u32,
            tiling: VK_IMAGE_TILING_OPTIMAL,
            format: VK_FORMAT_D24_UNORM_S8_UINT,
            img_type: VK_IMAGE_TYPE_2D,
            view_type: VK_IMAGE_VIEW_TYPE_2D,
            usage: VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT as u32,
            sharing_mode: VK_SHARING_MODE_EXCLUSIVE,
        };

        let sc_d_img_infos = vec![sc_d_img_info; sc_images.len()];

        let (sc_depth_imgs, sc_depth_img_vs, sc_depth_img_mems) =
            create_imgs_and_memory(&phy_dev, &device, &sc_d_img_infos, &vec![q_fly_idx]);

        // let vfd = match std::fs::read("shaders/scene_vert.spv") {
        //     Ok(x) => x,
        //     Err(err) => {
        //         panic!("ERR read vert shader {}", err);
        //     }
        // };

        // let ffd = match std::fs::read("shaders/scene_frag.spv") {
        //     Ok(x) => x,
        //     Err(err) => {
        //         panic!("ERR read frag shader {}", err);
        //     }
        // };

        // let vert_sh_mod_ci = VkShaderModuleCreateInfo::new(
        //     None,
        //     0,
        //     &match std::fs::read("shaders/scene_vert.spv") {
        //         Ok(x) => x,
        //         Err(err) => {
        //             panic!("ERR read vert shader {}", err);
        //         }
        //     },
        // );
        // let frag_sh_mod_ci = VkShaderModuleCreateInfo::new(
        //     None,
        //     0,
        //     &match std::fs::read("shaders/scene_frag.spv") {
        //         Ok(x) => x,
        //         Err(err) => {
        //             panic!("ERR read frag shader {}", err);
        //         }
        //     },
        // );

        let vert_sh_mod = match ShaderModule::create(
            device.device,
            &VkShaderModuleCreateInfo::new(
                None,
                0,
                &match std::fs::read("shaders/scene_vert.spv") {
                    Ok(x) => x,
                    Err(err) => {
                        panic!("ERR read vert shader {}", err);
                    }
                },
            ),
            None,
        ) {
            Ok(x) => x,
            Err(result) => {
                panic!("ERR create shader module {}", result);
            }
        };

        let frag_sh_mod = match ShaderModule::create(
            device.device,
            &VkShaderModuleCreateInfo::new(
                None,
                0,
                &match std::fs::read("shaders/scene_frag.spv") {
                    Ok(x) => x,
                    Err(err) => {
                        panic!("ERR read frag shader {}", err);
                    }
                },
            ),
            None,
        ) {
            Ok(x) => x,
            Err(result) => {
                panic!("ERR create shader module {}", result);
            }
        };

        let pipe_stages = vec![
            VkPipelineShaderStageCreateInfo::new(
                None,
                0,
                VK_SHADER_STAGE_VERTEX_BIT,
                vert_sh_mod.shader_module,
                c"main",
                None,
            ),
            VkPipelineShaderStageCreateInfo::new(
                None,
                0,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                frag_sh_mod.shader_module,
                c"main",
                None,
            ),
        ];

        let vert_attr_descs = vec![
            VkVertexInputAttributeDescription::new(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),
            VkVertexInputAttributeDescription::new(1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0),
            VkVertexInputAttributeDescription::new(2, 2, VK_FORMAT_R32G32_SFLOAT, 0),
        ];

        let vert_bind_descs = vec![
            VkVertexInputBindingDescription::new(
                0,
                (size_of::<f32>() * 3) as u32,
                VK_VERTEX_INPUT_RATE_VERTEX,
            ),
            VkVertexInputBindingDescription::new(
                1,
                (size_of::<f32>() * 3) as u32,
                VK_VERTEX_INPUT_RATE_VERTEX,
            ),
            VkVertexInputBindingDescription::new(
                2,
                (size_of::<f32>() * 2) as u32,
                VK_VERTEX_INPUT_RATE_VERTEX,
            ),
        ];

        let pvis_ci =
            VkPipelineVertexInputStateCreateInfo::new(None, 0, &vert_bind_descs, &vert_attr_descs);

        let pias_ci = VkPipelineInputAssemblyStateCreateInfo::new(
            None,
            0,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            0,
        );

        let viewport = VkViewport {
            x: 0.0,
            y: 0.0,
            width: surf_caps.currentExtent.width as f32,
            height: surf_caps.currentExtent.height as f32,
            minDepth: 0.0,
            maxDepth: 1.0,
        };

        let scissor = VkRect2D {
            extent: surf_caps.currentExtent,
            offset: VkOffset2D { x: 0, y: 0 },
        };

        let pipe_viewports = vec![viewport];
        let pipe_scissors = vec![scissor];

        let pvs_ci =
            VkPipelineViewportStateCreateInfo::new(None, 0, &pipe_viewports, &pipe_scissors);

        let prs_ci = VkPipelineRasterizationStateCreateInfo::new(
            None,
            0,
            0,
            0,
            VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_BACK_BIT as u32,
            VK_FRONT_FACE_COUNTER_CLOCKWISE,
            0,
            0.0,
            0.0,
            0.0,
            1.0,
        );

        let pms_ci = VkPipelineMultisampleStateCreateInfo::new(
            None,
            0,
            VK_SAMPLE_COUNT_1_BIT,
            0,
            0.0,
            &0,
            0,
            0,
        );

        let pcbas = vec![VkPipelineColorBlendAttachmentState {
            blendEnable: 1,
            srcColorBlendFactor: VK_BLEND_FACTOR_SRC_ALPHA,
            dstColorBlendFactor: VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            colorBlendOp: VK_BLEND_OP_ADD,
            srcAlphaBlendFactor: VK_BLEND_FACTOR_ONE,
            dstAlphaBlendFactor: VK_BLEND_FACTOR_ZERO,
            alphaBlendOp: VK_BLEND_OP_ADD,
            colorWriteMask: (VK_COLOR_COMPONENT_R_BIT
                | VK_COLOR_COMPONENT_G_BIT
                | VK_COLOR_COMPONENT_B_BIT
                | VK_COLOR_COMPONENT_A_BIT) as u32,
        }];

        let pcbs_ci = VkPipelineColorBlendStateCreateInfo::new(None, 0, 0, 0, &pcbas, [0.0; 4]);

        let desc_set_0_lyt_binds = vec![VkDescriptorSetLayoutBinding::new(
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT as u32,
            None,
        )];

        let desc_set_1_lyt_binds = vec![VkDescriptorSetLayoutBinding::new(
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_VERTEX_BIT as u32,
            None,
        )];

        let desc_set_lyt_cis = vec![
            VkDescriptorSetLayoutCreateInfo::new(None, 0, &desc_set_0_lyt_binds),
            VkDescriptorSetLayoutCreateInfo::new(None, 0, &desc_set_1_lyt_binds),
        ];

        let mut desc_set_lyts = vec![];
        for desc_set_lyt_ci in desc_set_lyt_cis {
            desc_set_lyts.push(
                match device.create_descriptor_set_layout(&desc_set_lyt_ci, None) {
                    Ok(x) => x,
                    Err(result) => {
                        panic!("ERR create descriptor set layout {}", result);
                    }
                },
            );
        }

        let desc_pool_sizes = vec![
            VkDescriptorPoolSize {
                type_: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                descriptorCount: 1,
            },
            VkDescriptorPoolSize {
                type_: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                descriptorCount: 1,
            },
        ];

        let desc_pool_ci = VkDescriptorPoolCreateInfo::new(None, 0, 10, &desc_pool_sizes);

        let desc_pool = match device.create_descriptor_pool(&desc_pool_ci, None) {
            Ok(x) => x,
            Err(result) => {
                panic!("ERR create descriptor pool {}", result);
            }
        };

        let pl_ci = VkPipelineLayoutCreateInfo::new(None, 0, &desc_set_lyts, &vec![]);

        let pipe_lyt = match device.create_pipeline_layout(&pl_ci, None) {
            Ok(x) => x,
            Err(result) => {
                panic!("ERR create pipeline layout {}", result);
            }
        };

        let dss_ci = VkPipelineDepthStencilStateCreateInfo::new(
            None,
            0,
            1,
            0,
            VK_COMPARE_OP_LESS_OR_EQUAL,
            0,
            0,
            VkStencilOpState::default(),
            VkStencilOpState::default(),
            0.0,
            0.0,
        );

        let col_attch_frmts = vec![surf_format.format];

        let view_pipe_dyn_rend_ci = VkPipelineRenderingCreateInfo::new(
            None,
            0,
            &col_attch_frmts,
            VK_FORMAT_UNDEFINED,
            VK_FORMAT_UNDEFINED,
        );

        let view_pipe_ci = VkGraphicsPipelineCreateInfo::new(
            Some(std::ptr::addr_of!(view_pipe_dyn_rend_ci) as *const c_void),
            0,
            &pipe_stages,
            &pvis_ci,
            &pias_ci,
            None,
            Some(&pvs_ci),
            Some(&prs_ci),
            Some(&pms_ci),
            Some(&dss_ci),
            Some(&pcbs_ci),
            None,
            pipe_lyt,
            None,
            None,
            None,
            None,
        );

        let view_pipes = match device.create_graphics_pipelines(None, &vec![view_pipe_ci], None) {
            Ok(x) => x,
            Err(result) => {
                panic!("ERR create graphics pipeline {}", result);
            }
        };

        let sc_cmd_pool_ci = VkCommandPoolCreateInfo::new(
            None,
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT as u32,
            q_fly_idx as usize,
        );

        let sc_cmd_pool = match CommandPool::create(
            device.device,
            &VkCommandPoolCreateInfo::new(
                None,
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT as u32,
                q_fly_idx as usize,
            ),
            None,
        ) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR create command pool {}", err);
            }
        };

        let sc_cmd_buff_ai = VkCommandBufferAllocateInfo::new(
            None,
            sc_cmd_pool.command_pool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            sc_images.len(),
        );

        let lyt_chng_cmd_buff_ai = VkCommandBufferAllocateInfo::new(
            None,
            sc_cmd_pool.command_pool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            1,
        );

        let lyt_chng_cmd_buff = match CommandBuffer::allocate(device.device, &lyt_chng_cmd_buff_ai)
        {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR allocate commmand buffers {}", err);
            }
        };

        let sc_cmd_buff = match CommandBuffer::allocate(device.device, &sc_cmd_buff_ai) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR allocate command buffers {}", err);
            }
        };

        let lyt_fnc_ci = VkFenceCreateInfo::new(None, VK_FENCE_CREATE_SIGNALED_BIT as u32);

        let img_lyt_chng_fnc = match device.create_fence(&lyt_fnc_ci, None) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR create fence {}", err);
            }
        };

        let sem_ci = VkSemaphoreCreateInfo::new(None, 0);

        let acq_sem = match device.create_semaphore(&sem_ci, None) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR create semaphore {}", err);
            }
        };

        let mut sc_img_lyt_chng_sems = vec![];
        let mut rndr_sems = vec![];

        for _ in 0..sc_images.len() {
            sc_img_lyt_chng_sems.push(match device.create_semaphore(&sem_ci, None) {
                Ok(x) => x,
                Err(err) => {
                    panic!("ERR create semaphore {}", err)
                }
            });

            rndr_sems.push(match device.create_semaphore(&sem_ci, None) {
                Ok(x) => x,
                Err(err) => {
                    panic!("ERR create semaphore {}", err)
                }
            });
        }

        let gfx_q = device.get_queue(q_fly_idx, 0);
        let xfer_q = device.get_queue(q_fly_idx, 1);

        let (uni_buff, uni_mem) = create_buffer_and_memory(
            &phy_dev,
            &device,
            &vec![q_fly_idx],
            (size_of::<glam::Mat4>() * 3 + size_of::<glam::Vec4>()) as VkDeviceSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT as u32,
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) as u32,
        );

        let desc_set_ai = VkDescriptorSetAllocateInfo::new(None, desc_pool, &desc_set_lyts);
        let desc_sets = match device.allocate_descriptor_sets(&desc_set_ai) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR allocate descriptor sets {}", err);
            }
        };

        let desc_buff_info = VkDescriptorBufferInfo {
            buffer: uni_buff,
            offset: 0,
            range: (size_of::<glam::Mat4>() * 2) as VkDeviceSize,
        };

        let desc_buff_infos = vec![desc_buff_info];
        let w_desc_set_0_bind_0 = VkWriteDescriptorSet::new(
            None,
            desc_sets[0],
            0,
            0,
            1,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            &vec![],
            &desc_buff_infos,
            None,
        );

        device.update_descriptor_sets(&vec![w_desc_set_0_bind_0], &vec![]);

        Self {
            instance,
            surface,
            surf_extent: surf_caps.currentExtent,
            phy_dev,
            device,
            swapchain,
            sc_images,
            sc_depth_imgs,
            sc_depth_img_mems,
            sc_image_views,
            sc_depth_img_vs,
            vert_sh_mod,
            frag_sh_mod,
            desc_set_lyts,
            desc_pool,
            pipe_lyt,
            view_pipes,
            sc_cmd_pool,
            sc_cmd_buff,
            lyt_chng_cmd_buff,
            img_lyt_chng_fnc,
            acq_sem,
            sc_img_lyt_chng_sems,
            rndr_sems,
            uni_buff,
            uni_mem,
            min_uni_buff_align,
            q_fly_idx,
            gfx_q,
            xfer_q,
        }
    }

    pub fn scene_init(&self, path: &str) {
        let (gltf, buffers, images) = match gltf::import(path) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR opening GLB {}", err);
            }
        };

        for mesh in gltf.meshes() {
            println!("{}", mesh.name().unwrap());

            for prim in mesh.primitives() {}
        }
    }

    pub fn draw_gui(
        &self,
        clipped_primitives: &Vec<egui::ClippedPrimitive>,
        full_output: egui::FullOutput,
        img_idx: usize,
    ) {
        // self.sc_cmd_buff.
    }

    pub fn draw_scene(&self) {}

    pub fn render(
        &self,
        // clipped_primitives: &Vec<egui::ClippedPrimitive>,
        // full_output: egui::FullOutput,
    ) {
        let img_idx_res = self.device.acquire_next_image_khr(
            self.swapchain.swapchain,
            u64::MAX,
            Some(self.acq_sem),
            None,
        );

        match img_idx_res {
            Ok(_) => (),
            Err(err) => match err {
                VK_ERROR_OUT_OF_DATE_KHR => {
                    println!("recreate swapchain");
                    return;
                }
                _ => {
                    panic!("ERR acquire next image khr {}", err)
                }
            },
        }

        let img_idx = img_idx_res.ok().unwrap();

        let mut wait_sems = vec![self.acq_sem];
        let mut sig_sems = vec![self.sc_img_lyt_chng_sems[img_idx]];
        let mut cmd_buffs = vec![];

        change_image_layout(
            &self.device,
            self.sc_images[img_idx],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT as u32,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT as u32,
            0,
            0,
            &self.lyt_chng_cmd_buff,
            0,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT as u32,
            &wait_sems,
            &sig_sems,
            self.img_lyt_chng_fnc,
            self.q_fly_idx,
            &self.xfer_q,
        );

        let color_attachments = vec![VkRenderingAttachmentInfo::new(
            None,
            self.sc_image_views[img_idx].image_view,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            0,
            None,
            0,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VkClearValue {
                color: VkClearColorValue {
                    float32: [0.0, 0.0, 0.0, 1.0],
                },
            },
        )];

        let depth_attachment = VkRenderingAttachmentInfo::new(
            None,
            self.sc_depth_img_vs[img_idx].image_view,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            0,
            None,
            0,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VkClearValue {
                depthStencil: VkClearDepthStencilValue {
                    depth: 0.0,
                    stencil: 0,
                },
            },
        );

        let dyn_rend_info = VkRenderingInfo::new(
            None,
            0,
            VkRect2D {
                extent: self.surf_extent,
                offset: VkOffset2D { x: 0, y: 0 },
            },
            1,
            0,
            &vec![VkRenderingAttachmentInfo::new(
                None,
                self.sc_image_views[img_idx].image_view,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                0,
                None,
                0,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VkClearValue {
                    color: VkClearColorValue {
                        float32: [0.0, 0.0, 0.0, 1.0],
                    },
                },
            )],
            Some(&depth_attachment),
            None,
        );

        let cmd_buff_bi = VkCommandBufferBeginInfo::new(
            None,
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT as u32,
            None,
        );

        match self.sc_cmd_buff.begin(img_idx, &cmd_buff_bi) {
            Ok(_) => (),
            Err(err) => {
                panic!("ERR begin command buffer {}", err);
            }
        };

        match self.sc_cmd_buff.end(img_idx) {
            Ok(_) => (),
            Err(err) => {
                panic!("ERR begin command buffer {}", err);
            }
        };

        let wait_stage_msk = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT as u32;

        wait_sems = vec![self.sc_img_lyt_chng_sems[img_idx]];
        sig_sems = vec![self.rndr_sems[img_idx]];
        cmd_buffs = vec![self.sc_cmd_buff.cmd_buffs[img_idx]];

        let si = VkSubmitInfo::new(
            None,
            &wait_sems,
            Some(&wait_stage_msk),
            &cmd_buffs,
            &sig_sems,
        );

        match self.gfx_q.submit(&vec![si], None) {
            Ok(_) => (),
            Err(err) => {
                panic!("ERR queue submit {}", err);
            }
        };

        let q_wait_stg_msk = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT as u32;

        wait_sems = vec![self.rndr_sems[img_idx]];
        sig_sems = vec![self.sc_img_lyt_chng_sems[img_idx]];

        change_image_layout(
            &self.device,
            self.sc_images[img_idx],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT as u32,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT as u32,
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT as u32,
            0,
            &self.lyt_chng_cmd_buff,
            0,
            q_wait_stg_msk,
            &wait_sems,
            &sig_sems,
            self.img_lyt_chng_fnc,
            self.q_fly_idx,
            &self.xfer_q,
        );

        wait_sems = vec![self.sc_img_lyt_chng_sems[img_idx]];
        let swapchains = vec![self.swapchain.swapchain];
        let img_idxs = vec![img_idx as u32];
        let pi = VkPresentInfoKHR::new(None, &wait_sems, &swapchains, &img_idxs, None);

        match self.gfx_q.present(&pi) {
            Ok(_) => (),
            Err(err) => match err {
                VK_ERROR_OUT_OF_DATE_KHR => {
                    println!("recreate swapchain");
                }
                _ => {
                    panic!("ERR acquire next image khr {}", err)
                }
            },
        };
    }

    pub fn shutdown(&self) {
        self.gfx_q.wait_idle();
        self.device.destroy_buffer(self.uni_buff, None);
        self.device.free_memory(self.uni_mem, None);

        for sem in &self.sc_img_lyt_chng_sems {
            self.device.destroy_semaphore(*sem, None);
        }

        for sem in &self.rndr_sems {
            self.device.destroy_semaphore(*sem, None);
        }

        self.device.destroy_semaphore(self.acq_sem, None);

        self.device.destroy_fence(self.img_lyt_chng_fnc, None);
        // self.device.destroy_command_pool(self.sc_cmd_pool, None);
        self.device.destroy_pipeline_layout(self.pipe_lyt, None);
        self.device.destroy_pipelines(&self.view_pipes, None);

        self.device.destroy_descriptor_pool(self.desc_pool, None);

        for desc_set_lyt in &self.desc_set_lyts {
            self.device
                .destroy_descriptor_set_layout(*desc_set_lyt, None);
        }

        // for sc_depth_img in &self.sc_depth_imgs {
        //     self.device.destroy_image(*sc_depth_img, None);
        // }

        // for sc_depth_iv in &self.sc_depth_img_vs {
        //     self.device.destroy_image_view(*sc_depth_iv, None);
        // }

        for sc_depth_img_mem in &self.sc_depth_img_mems {
            self.device.free_memory(*sc_depth_img_mem, None);
        }

        // for sc_image_view in &self.sc_image_views {
        //     self.device.destroy_image_view(*sc_image_view, None);
        // }

        // self.device.destroy_shader_module(self.vert_sh_mod, None);
        // self.device.destroy_shader_module(self.frag_sh_mod, None);

        // self.device.destroy_swapchain_khr(self.swapchain, None);
        // self.device.destroy(None);
        // self.instance.destroy_surface_khr(self.surface, None);
        // self.instance.destroy(None);
    }
}

pub fn get_memory_id(
    mem_props: &VkPhysicalDeviceMemoryProperties,
    mem_reqs: &VkMemoryRequirements,
    req_types: VkMemoryPropertyFlags,
) -> Result<u32, i32> {
    match mem_props
        .memoryTypes
        .iter()
        .enumerate()
        .find(|(id, mem_type)| {
            (mem_reqs.memoryTypeBits & (1 << id)) == (1 << id)
                && mem_type.propertyFlags == req_types
                && mem_props.memoryHeaps[mem_props.memoryTypes[*id].heapIndex as usize].size
                    > mem_reqs.size
        }) {
        Some((id, _)) => Ok(id as u32),
        None => Err(-1),
    }
}

pub fn create_imgs_and_memory(
    phy_dev: &PhysicalDevice,
    device: &Device,
    img_infos: &Vec<ImageInfo>,
    q_fly_idx: &Vec<u32>,
) -> (Vec<Image>, Vec<ImageView>, Vec<VkDeviceMemory>) {
    let mut imgs = vec![];
    let mut img_views = vec![];
    let mut mem_ais: Vec<VkMemoryAllocateInfo> = Vec::new();
    let mut mems = vec![];

    let mut mem_ais_imgs = Vec::<Vec<(VkImage, u64)>>::new();

    for img_info in img_infos {
        let img_ci = VkImageCreateInfo::new(
            None,
            0,
            img_info.img_type,
            img_info.format,
            img_info.extent,
            1,
            1,
            img_info.samples,
            img_info.tiling,
            img_info.usage,
            img_info.sharing_mode,
            q_fly_idx,
            VK_IMAGE_LAYOUT_UNDEFINED,
        );

        let image = match Image::create(
            device.device,
            &VkImageCreateInfo::new(
                None,
                0,
                img_info.img_type,
                img_info.format,
                img_info.extent,
                1,
                1,
                img_info.samples,
                img_info.tiling,
                img_info.usage,
                img_info.sharing_mode,
                q_fly_idx,
                VK_IMAGE_LAYOUT_UNDEFINED,
            ),
            None,
        ) {
            Ok(img) => img,
            Err(result) => {
                panic!("ERR create image {}", result);
            }
        };

        let mem_props = phy_dev.memory_properties();
        let mem_reqs = image.memory_requirements();
        let mem_id = match get_memory_id(&mem_props, &mem_reqs, img_info.req_mem_types) {
            Ok(id) => id,
            Err(result) => {
                panic!("ERR get memory id {}", result);
            }
        };

        let mut mem_ai_found = false;

        for (i, mem_ai) in mem_ais.iter_mut().enumerate() {
            if mem_ai.memoryTypeIndex == mem_id {
                mem_ai.allocationSize += mem_reqs.size;
                mem_ai_found = true;

                mem_ais_imgs[i].push((image.image, mem_reqs.size));

                break;
            }
        }

        if !mem_ai_found {
            mem_ais.push(VkMemoryAllocateInfo::new(None, mem_reqs.size, mem_id));
            mem_ais_imgs.push(vec![(image.image, mem_reqs.size)]);
        }

        imgs.push(image);
    }

    for mem_ai in &mem_ais {
        mems.push(match device.allocate_memory(mem_ai, None) {
            Ok(mem) => mem,
            Err(result) => {
                panic!("ERR allocate memory {}", result);
            }
        });
    }

    for (m, mem) in mems.iter().enumerate() {
        let mut offset = 0;

        for img in &mem_ais_imgs[m] {
            match device.bind_image_memory(img.0, *mem, offset) {
                Ok(()) => (),
                Err(result) => {
                    panic!("ERR bind image memory {}", result);
                }
            }

            offset += img.1;
        }
    }

    for (i, img_info) in img_infos.iter().enumerate() {
        // let img_v_ci = VkImageViewCreateInfo::new(
        //     None,
        //     0,
        //     imgs[i],
        //     img_info.view_type,
        //     img_info.format,
        //     VkComponentMapping::default(),
        //     VkImageSubresourceRange {
        //         aspectMask: img_info.aspect,
        //         baseMipLevel: 0,
        //         levelCount: 1,
        //         baseArrayLayer: 0,
        //         layerCount: 1,
        //     },
        // );

        let img_view = match ImageView::create(
            device.device,
            &VkImageViewCreateInfo::new(
                None,
                0,
                imgs[i].image,
                img_info.view_type,
                img_info.format,
                VkComponentMapping::default(),
                VkImageSubresourceRange {
                    aspectMask: img_info.aspect,
                    levelCount: 1,
                    layerCount: 1,
                    ..Default::default()
                },
            ),
            None,
        ) {
            Ok(img_v) => img_v,
            Err(result) => {
                panic!("ERR create image view {}", result);
            }
        };

        img_views.push(img_view);
    }

    (imgs, img_views, mems)
}

pub fn create_buffer_and_memory(
    phy_dev: &PhysicalDevice,
    device: &Device,
    q_fly_idx: &Vec<u32>,
    size: VkDeviceSize,
    usage: VkBufferUsageFlags,
    req_types: VkMemoryPropertyFlags,
) -> (VkBuffer, VkDeviceMemory) {
    let buffer_ci =
        VkBufferCreateInfo::new(None, 0, size, usage, VK_SHARING_MODE_EXCLUSIVE, q_fly_idx);
    let buff = match device.create_buffer(&buffer_ci, None) {
        Ok(x) => x,
        Err(err) => {
            panic!("ERR create buffer {}", err);
        }
    };

    let mem_props = phy_dev.memory_properties();
    let mem_req = device.buffer_memory_requirements(buff);
    let mem_id = match get_memory_id(&mem_props, &mem_req, req_types) {
        Ok(id) => id,
        Err(err) => {
            panic!("ERR get memory id {}", err);
        }
    };

    let mem_ai = VkMemoryAllocateInfo::new(None, mem_req.size, mem_id);

    let mem = match device.allocate_memory(&mem_ai, None) {
        Ok(mem) => mem,
        Err(err) => {
            panic!("ERR allocate memory {}", err);
        }
    };

    match device.bind_buffer_memory(buff, mem, 0) {
        Ok(()) => (),
        Err(err) => {
            panic!("ERR bind buffer memory {}", err);
        }
    };

    (buff, mem)
}

pub fn change_image_layout(
    device: &Device,
    image: VkImage,
    old_lyt: VkImageLayout,
    new_lyt: VkImageLayout,
    src_stg_msk: VkPipelineStageFlags,
    dst_stg_msk: VkPipelineStageFlags,
    src_acc: VkAccessFlags,
    dst_acc: VkAccessFlags,
    cmd_buff: &CommandBuffer,
    cmd_buff_idx: usize,
    q_wait_stg_msk: VkPipelineStageFlags,
    wait_sems: &Vec<VkSemaphore>,
    sig_sems: &Vec<VkSemaphore>,
    lyt_chng_fence: VkFence,
    q_fly_idx: u32,
    q: &Queue,
) {
    match device.wait_for_fences(&vec![lyt_chng_fence], true, u64::MAX) {
        Ok(_) => (),
        Err(err) => {
            panic!("ERR wait for fences {}", err);
        }
    }

    match device.reset_fences(&vec![lyt_chng_fence]) {
        Ok(_) => (),
        Err(err) => {
            panic!("ERR reset fences {}", err);
        }
    }

    let sub_re_rng = VkImageSubresourceRange {
        aspectMask: VK_IMAGE_ASPECT_COLOR_BIT as u32,
        levelCount: 1,
        layerCount: 1,
        baseArrayLayer: 0,
        baseMipLevel: 0,
    };

    let img_mem_bar = VkImageMemoryBarrier::new(
        None,
        src_acc,
        dst_acc,
        old_lyt,
        new_lyt,
        q_fly_idx as u32,
        q_fly_idx as u32,
        image,
        sub_re_rng,
    );

    let cmd_buff_bi = VkCommandBufferBeginInfo::new(
        None,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT as u32,
        None,
    );

    match cmd_buff.begin(cmd_buff_idx, &cmd_buff_bi) {
        Ok(_) => (),
        Err(err) => {
            panic!("ERR begin command buffer {}", err);
        }
    };

    cmd_buff.pipeline_barrier(
        cmd_buff_idx,
        src_stg_msk,
        dst_stg_msk,
        0,
        &vec![],
        &vec![],
        &vec![img_mem_bar],
    );

    match cmd_buff.end(cmd_buff_idx) {
        Ok(_) => (),
        Err(err) => {
            panic!("ERR end command buffer {}", err);
        }
    };

    let cmd_buffs = vec![cmd_buff.cmd_buffs[cmd_buff_idx]];
    let si = VkSubmitInfo::new(None, wait_sems, Some(&q_wait_stg_msk), &cmd_buffs, sig_sems);

    match q.submit(&vec![si], Some(lyt_chng_fence)) {
        Ok(_) => (),
        Err(err) => {
            panic!("ERR queue submit {}", err);
        }
    };
}

/*
pub fn vk_compute() -> Result<(), Box<dyn Error>> {
    let mut phy_dev = PhysicalDevice::default();
    let mut q_fly_idx = 0;
    let mut q_fly_q_cnt = 0;
    let mut min_sto_buff_align = 0;

    let ai = VkApplicationInfo::new::<i32>(
        None,
        c"Chizen",
        VK_MAKE_API_VERSION!(0, 1, 0, 0),
        c"Chizen",
        VK_MAKE_API_VERSION!(0, 1, 0, 0),
        VK_MAKE_API_VERSION!(0, 1, 3, 280),
    );

    let inst_ext_names = vec![
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    ];

    let inst_ci = VkInstanceCreateInfo::new(None, 0, ai, &inst_ext_names);

    let mut instance = match Instance::create(inst_ci, None) {
        Ok(instance) => instance,
        Err(result) => {
            panic!("ERR Create instance {}", result);
        }
    };

    let phy_devs = instance.get_physical_devices();

    for pd in phy_devs {
        let phy_dev_props = pd.properties();

        if phy_dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            || phy_dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
        {
            let q_fly_props = pd.queue_family_properties();

            for (q, q_fly_prop) in q_fly_props.iter().enumerate() {
                if (q_fly_prop.queueFlags & VK_QUEUE_GRAPHICS_BIT as u32)
                    == VK_QUEUE_GRAPHICS_BIT as u32
                {
                    phy_dev = pd;
                    q_fly_idx = q;
                    q_fly_q_cnt = q_fly_prop.queueCount;
                    min_sto_buff_align = phy_dev_props.limits.minStorageBufferOffsetAlignment;
                    break;
                }
            }
        }
    }

    let priorities: Vec<f32> = vec![1.0; q_fly_q_cnt as usize];
    let q_ci = VkDeviceQueueCreateInfo::new(None, 0, q_fly_idx as u32, q_fly_q_cnt, &priorities);
    let dev_exts = vec![VK_KHR_SWAPCHAIN_EXTENSION_NAME];
    let q_cis = vec![q_ci];
    let device_ci = VkDeviceCreateInfo::new(None, 0, &q_cis, &dev_exts, None);

    let mut device = match Device::create(&phy_dev, device_ci, None) {
        Ok(device) => device,
        Err(result) => {
            panic!("ERR Create device {}", result);
        }
    };

    let mut input_data = [0.0 as f32; 10];

    for (i, _) in input_data.into_iter().enumerate() {
        input_data[i] = i as f32 + 0.5;
    }

    let mut output_data = [0.0 as f32; 10];

    let input_data_size = (input_data.len() * size_of::<f32>()) as u64;
    let input_data_aligned_size = if input_data_size < min_sto_buff_align {
        min_sto_buff_align
    } else {
        min_sto_buff_align
            * ((input_data_size / min_sto_buff_align)
                + if input_data_size % min_sto_buff_align > 0 {
                    1
                } else {
                    0
                })
    };

    let output_data_size = (output_data.len() * size_of::<f32>()) as u64;
    let output_data_aligned_size = if output_data_size < min_sto_buff_align {
        min_sto_buff_align
    } else {
        min_sto_buff_align
            * ((output_data_size / min_sto_buff_align)
                + if output_data_size % min_sto_buff_align > 0 {
                    1
                } else {
                    0
                })
    };

    let buff_ci = VkBufferCreateInfo::new::<i32>(
        None,
        0,
        input_data_aligned_size + output_data_aligned_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT as u32,
        VK_SHARING_MODE_EXCLUSIVE,
        vec![q_fly_idx as u32],
    );

    let buffer = match device.create_buffer(buff_ci, None) {
        Ok(buffer) => buffer,
        Err(result) => {
            panic!("ERR Create buffer {}", result);
        }
    };

    let mem_props = phy_dev.memory_properties();
    let mem_reqs = device.get_buffer_memory_requirements(buffer);
    let req_types =
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) as u32;
    let mem_idx = match get_memory_type_idx(&mem_props, &mem_reqs, req_types) {
        Ok(id) => id,
        Err(id) => {
            panic!("ERR get memory type index {}", id);
        }
    };

    let mem_ai = VkMemoryAllocateInfo::new::<i32>(None, mem_reqs.size, mem_idx as u32);

    let dev_mem = match device.allocate_memory(mem_ai, None) {
        Ok(dev_mem) => dev_mem,
        Err(result) => {
            panic!("ERR Allocate memory {}", result);
        }
    };

    match device.bind_buffer_memory(buffer, dev_mem, 0) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR bind buffer memory {}", result);
        }
    };

    let in_out_mem_map = match device.map_memory(
        dev_mem,
        0,
        input_data_aligned_size + output_data_aligned_size,
        0,
    ) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR map memory {}", result);
        }
    };

    unsafe {
        std::ptr::copy_nonoverlapping(
            input_data.as_ptr() as *mut std::ffi::c_void,
            in_out_mem_map,
            input_data.len() * size_of::<f32>(),
        );
    }

    let cfd = std::fs::read("shaders/compute.spv")?;
    let shader_mod_ci = VkShaderModuleCreateInfo::new::<i32>(None, 0, &cfd);

    let shader_mod = match device.create_shader_module(shader_mod_ci, None) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR create shader module {}", result);
        }
    };

    let pool_sizes = vec![VkDescriptorPoolSize {
        type_: VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        descriptorCount: 1,
    }];
    let desc_pool_ci = VkDescriptorPoolCreateInfo::new::<i32>(None, 0, 1, &pool_sizes);

    let desc_pool = match device.create_descriptor_pool(desc_pool_ci, None) {
        Ok(desc_pool) => desc_pool,
        Err(result) => {
            panic!("ERR create descriptor pool {}", result);
        }
    };

    let desc_set_lyt_bind_0_0 = VkDescriptorSetLayoutBinding::new::<i32>(
        0,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        1,
        VK_SHADER_STAGE_COMPUTE_BIT as u32,
        None,
    );

    let desc_set_lyt_bind_0_1 = VkDescriptorSetLayoutBinding::new::<i32>(
        1,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        1,
        VK_SHADER_STAGE_COMPUTE_BIT as u32,
        None,
    );

    let desc_set_lyt_binds = vec![desc_set_lyt_bind_0_0, desc_set_lyt_bind_0_1];
    let desc_set_lyt_ci = VkDescriptorSetLayoutCreateInfo::new::<i32>(None, 0, &desc_set_lyt_binds);

    let desc_set_lyt = match device.create_descriptor_set_layout(desc_set_lyt_ci, None) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR create desctriptor set layout {}", result);
        }
    };

    let desc_set_lyts = vec![desc_set_lyt];
    let desc_set_ai = VkDescriptorSetAllocateInfo::new::<i32>(None, desc_pool, &desc_set_lyts);

    let desc_sets = match device.allocate_descriptor_sets(desc_set_ai) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR allocate descriptor sets {}", result);
        }
    };

    let pipe_lyt_ci = VkPipelineLayoutCreateInfo::new::<i32>(None, 0, &desc_set_lyts, &vec![]);
    let pipe_lyt = match device.create_pipeline_layout(pipe_lyt_ci, None) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR creating pipeline layout {}", result);
        }
    };

    let entry_point = c"main";
    let pipe_stg_ci = VkPipelineShaderStageCreateInfo::new::<i32>(
        None,
        0,
        VK_SHADER_STAGE_COMPUTE_BIT,
        shader_mod,
        entry_point,
        None,
    );

    let pipe_ci =
        VkComputePipelineCreateInfo::new::<i32>(None, 0, pipe_stg_ci, pipe_lyt, 0 as VkPipeline, 0);

    let cmp_pipes =
        match device.create_compute_pipelines(0 as VkPipelineCache, &vec![pipe_ci], None) {
            Ok(x) => x,
            Err(result) => {
                panic!("ERR create compute pipelines {}", result);
            }
        };

    let cmp_bi_0_0 = VkDescriptorBufferInfo {
        buffer: buffer,
        offset: 0 as u64,
        range: (input_data.len() * size_of::<f32>()) as VkDeviceSize,
    };

    let cmp_bi_0_1 = VkDescriptorBufferInfo {
        buffer: buffer,
        offset: input_data_aligned_size,
        range: (output_data.len() * size_of::<f32>()) as VkDeviceSize,
    };

    let cmp_write_desc_sets = vec![
        VkWriteDescriptorSet::new::<i32>(
            None,
            desc_sets[0],
            0,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            &vec![],
            &vec![cmp_bi_0_0],
            &std::ptr::null_mut(),
        ),
        VkWriteDescriptorSet::new::<i32>(
            None,
            desc_sets[0],
            1,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            &vec![],
            &vec![cmp_bi_0_1],
            &std::ptr::null_mut(),
        ),
    ];

    device.update_descriptor_sets(&cmp_write_desc_sets, &vec![]);

    let cmd_pool_ci = VkCommandPoolCreateInfo::new::<i32>(None, 0, q_fly_idx);
    let cmd_pool = match device.create_command_pool(&cmd_pool_ci, None) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR create command pool {}", result);
        }
    };

    let cmd_buff_ai =
        VkCommandBufferAllocateInfo::new::<i32>(None, cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

    let cmd_buff = match device.allocate_command_buffers(cmd_buff_ai) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR allocate command buffers {}", result)
        }
    };

    let cmd_buf_bi = VkCommandBufferBeginInfo::new::<i32>(
        None,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT as u32,
        None,
    );

    match cmd_buff.begin(0, cmd_buf_bi) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR begin command buffer {}", result);
        }
    }

    cmd_buff.bind_pipeline(0, VK_PIPELINE_BIND_POINT_COMPUTE, cmp_pipes[0]);
    cmd_buff.bind_descriptor_sets(
        0,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipe_lyt,
        0,
        &desc_sets,
        &vec![],
    );

    cmd_buff.dispatch(0, input_data.len(), 1, 1);

    match cmd_buff.end(0) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR begin command buffer {}", result);
        }
    }

    let f_ci = VkFenceCreateInfo::default();
    let f = match device.create_fence(f_ci, None) {
        Ok(x) => x,
        Err(result) => {
            panic!("Err create fence {}", result);
        }
    };

    let q = device.get_queue(q_fly_idx, 0);
    let si = VkSubmitInfo::new::<i32, VkPipelineStageFlags>(
        None,
        &vec![],
        None,
        &vec![cmd_buff.cmd_buffs[0]],
        &vec![],
    );

    match q.submit(&vec![si], f) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR queue submit {}", result);
        }
    }

    match device.wait_for_fences(&vec![f], true, u64::MAX) {
        Ok(x) => x,
        Err(result) => {
            panic!("ERR wait for fences{}", result);
        }
    }

    unsafe {
        std::ptr::copy_nonoverlapping(
            in_out_mem_map.add(input_data_aligned_size as usize),
            output_data.as_mut_ptr() as *mut std::ffi::c_void,
            output_data.len() * size_of::<f32>(),
        );
    }

    // unsafe {
    //     for i in 0..input_data.len() {
    //         println!("{:?}", *(in_out_mem_map as *mut c_float).add(i));
    //     }

    //     for o in 0..output_data.len() {
    //         println!(
    //             "{:?}",
    //             *((in_out_mem_map).add(input_data_aligned_size as usize) as *mut c_float).add(o)
    //         )
    //     }
    // }

    println!("input_data: {:?}", input_data);
    println!("output_data: {:?}", output_data);

    device.destroy_fence(f, None);
    device.free_command_buffers(cmd_pool, cmd_buff);
    device.destroy_command_pool(cmd_pool, None);
    device.destroy_pipelines(cmp_pipes, None);
    device.destroy_pipeline_layout(pipe_lyt, None);
    device.destroy_descriptor_set_layout(desc_set_lyt, None);
    device.destroy_descriptor_pool(desc_pool, None);
    device.destroy_shader_module(shader_mod, None);
    device.unmap_memory(dev_mem);
    device.destroy_buffer(buffer, None);
    device.free_memory(dev_mem, None);
    device.destroy(None);
    instance.destroy(None);

    Ok(())
}
*/
