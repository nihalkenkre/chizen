use std::{
    ffi::{CStr, CString},
    option,
    os::raw::c_void,
    u64,
};

use glam::mat2;

include!(concat!(env!("OUT_DIR"), "/vk_wrapper.rs"));

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

// impl Default for VkApplicationInfo {
//     fn default() -> Self {
//         Self {
//             sType: VK_STRUCTURE_TYPE_APPLICATION_INFO,
//             pNext: std::ptr::null(),
//             pApplicationName: c"Default Name".as_ptr() as *const i8,
//             applicationVersion: Default::default(),
//             pEngineName: c"Default Name".as_ptr() as *const i8,
//             engineVersion: Default::default(),
//             apiVersion: Default::default(),
//         }
//     }
// }

// impl Default for PhysicalDevice {
//     fn default() -> Self {
//         Self {
//             phy_dev: std::ptr::null_mut(),
//         }
//     }
// }

// impl Default for VkPhysicalDeviceProperties {
//     fn default() -> Self {
//         Self {
//             apiVersion: Default::default(),
//             driverVersion: Default::default(),
//             vendorID: Default::default(),
//             deviceID: Default::default(),
//             deviceType: Default::default(),
//             deviceName: [0; 256],
//             pipelineCacheUUID: Default::default(),
//             limits: Default::default(),
//             sparseProperties: Default::default(),
//         }
//     }
// }

// impl Default for VkPhysicalDeviceLimits {
//     fn default() -> Self {
//         Self {
//             maxImageDimension1D: Default::default(),
//             maxImageDimension2D: Default::default(),
//             maxImageDimension3D: Default::default(),
//             maxImageDimensionCube: Default::default(),
//             maxImageArrayLayers: Default::default(),
//             maxTexelBufferElements: Default::default(),
//             maxUniformBufferRange: Default::default(),
//             maxStorageBufferRange: Default::default(),
//             maxPushConstantsSize: Default::default(),
//             maxMemoryAllocationCount: Default::default(),
//             maxSamplerAllocationCount: Default::default(),
//             bufferImageGranularity: Default::default(),
//             sparseAddressSpaceSize: Default::default(),
//             maxBoundDescriptorSets: Default::default(),
//             maxPerStageDescriptorSamplers: Default::default(),
//             maxPerStageDescriptorUniformBuffers: Default::default(),
//             maxPerStageDescriptorStorageBuffers: Default::default(),
//             maxPerStageDescriptorSampledImages: Default::default(),
//             maxPerStageDescriptorStorageImages: Default::default(),
//             maxPerStageDescriptorInputAttachments: Default::default(),
//             maxPerStageResources: Default::default(),
//             maxDescriptorSetSamplers: Default::default(),
//             maxDescriptorSetUniformBuffers: Default::default(),
//             maxDescriptorSetUniformBuffersDynamic: Default::default(),
//             maxDescriptorSetStorageBuffers: Default::default(),
//             maxDescriptorSetStorageBuffersDynamic: Default::default(),
//             maxDescriptorSetSampledImages: Default::default(),
//             maxDescriptorSetStorageImages: Default::default(),
//             maxDescriptorSetInputAttachments: Default::default(),
//             maxVertexInputAttributes: Default::default(),
//             maxVertexInputBindings: Default::default(),
//             maxVertexInputAttributeOffset: Default::default(),
//             maxVertexInputBindingStride: Default::default(),
//             maxVertexOutputComponents: Default::default(),
//             maxTessellationGenerationLevel: Default::default(),
//             maxTessellationPatchSize: Default::default(),
//             maxTessellationControlPerVertexInputComponents: Default::default(),
//             maxTessellationControlPerVertexOutputComponents: Default::default(),
//             maxTessellationControlPerPatchOutputComponents: Default::default(),
//             maxTessellationControlTotalOutputComponents: Default::default(),
//             maxTessellationEvaluationInputComponents: Default::default(),
//             maxTessellationEvaluationOutputComponents: Default::default(),
//             maxGeometryShaderInvocations: Default::default(),
//             maxGeometryInputComponents: Default::default(),
//             maxGeometryOutputComponents: Default::default(),
//             maxGeometryOutputVertices: Default::default(),
//             maxGeometryTotalOutputComponents: Default::default(),
//             maxFragmentInputComponents: Default::default(),
//             maxFragmentOutputAttachments: Default::default(),
//             maxFragmentDualSrcAttachments: Default::default(),
//             maxFragmentCombinedOutputResources: Default::default(),
//             maxComputeSharedMemorySize: Default::default(),
//             maxComputeWorkGroupCount: Default::default(),
//             maxComputeWorkGroupInvocations: Default::default(),
//             maxComputeWorkGroupSize: Default::default(),
//             subPixelPrecisionBits: Default::default(),
//             subTexelPrecisionBits: Default::default(),
//             mipmapPrecisionBits: Default::default(),
//             maxDrawIndexedIndexValue: Default::default(),
//             maxDrawIndirectCount: Default::default(),
//             maxSamplerLodBias: Default::default(),
//             maxSamplerAnisotropy: Default::default(),
//             maxViewports: Default::default(),
//             maxViewportDimensions: Default::default(),
//             viewportBoundsRange: Default::default(),
//             viewportSubPixelBits: Default::default(),
//             minMemoryMapAlignment: Default::default(),
//             minTexelBufferOffsetAlignment: Default::default(),
//             minUniformBufferOffsetAlignment: Default::default(),
//             minStorageBufferOffsetAlignment: Default::default(),
//             minTexelOffset: Default::default(),
//             maxTexelOffset: Default::default(),
//             minTexelGatherOffset: Default::default(),
//             maxTexelGatherOffset: Default::default(),
//             minInterpolationOffset: Default::default(),
//             maxInterpolationOffset: Default::default(),
//             subPixelInterpolationOffsetBits: Default::default(),
//             maxFramebufferWidth: Default::default(),
//             maxFramebufferHeight: Default::default(),
//             maxFramebufferLayers: Default::default(),
//             framebufferColorSampleCounts: Default::default(),
//             framebufferDepthSampleCounts: Default::default(),
//             framebufferStencilSampleCounts: Default::default(),
//             framebufferNoAttachmentsSampleCounts: Default::default(),
//             maxColorAttachments: Default::default(),
//             sampledImageColorSampleCounts: Default::default(),
//             sampledImageIntegerSampleCounts: Default::default(),
//             sampledImageDepthSampleCounts: Default::default(),
//             sampledImageStencilSampleCounts: Default::default(),
//             storageImageSampleCounts: Default::default(),
//             maxSampleMaskWords: Default::default(),
//             timestampComputeAndGraphics: Default::default(),
//             timestampPeriod: Default::default(),
//             maxClipDistances: Default::default(),
//             maxCullDistances: Default::default(),
//             maxCombinedClipAndCullDistances: Default::default(),
//             discreteQueuePriorities: Default::default(),
//             pointSizeRange: Default::default(),
//             lineWidthRange: Default::default(),
//             pointSizeGranularity: Default::default(),
//             lineWidthGranularity: Default::default(),
//             strictLines: Default::default(),
//             standardSampleLocations: Default::default(),
//             optimalBufferCopyOffsetAlignment: Default::default(),
//             optimalBufferCopyRowPitchAlignment: Default::default(),
//             nonCoherentAtomSize: Default::default(),
//         }
//     }
// }

// impl Default for VkPhysicalDeviceSparseProperties {
//     fn default() -> Self {
//         Self {
//             residencyStandard2DBlockShape: Default::default(),
//             residencyStandard2DMultisampleBlockShape: Default::default(),
//             residencyStandard3DBlockShape: Default::default(),
//             residencyAlignedMipSize: Default::default(),
//             residencyNonResidentStrict: Default::default(),
//         }
//     }
// }

// impl Default for VkQueueFamilyProperties {
//     fn default() -> Self {
//         Self {
//             queueFlags: Default::default(),
//             queueCount: Default::default(),
//             timestampValidBits: Default::default(),
//             minImageTransferGranularity: VkExtent3D {
//                 width: 0,
//                 height: 0,
//                 depth: 0,
//             },
//         }
//     }
// }

// impl Default for VkPhysicalDeviceFeatures {
//     fn default() -> Self {
//         Self {
//             robustBufferAccess: Default::default(),
//             fullDrawIndexUint32: Default::default(),
//             imageCubeArray: Default::default(),
//             independentBlend: Default::default(),
//             geometryShader: Default::default(),
//             tessellationShader: Default::default(),
//             sampleRateShading: Default::default(),
//             dualSrcBlend: Default::default(),
//             logicOp: Default::default(),
//             multiDrawIndirect: Default::default(),
//             drawIndirectFirstInstance: Default::default(),
//             depthClamp: Default::default(),
//             depthBiasClamp: Default::default(),
//             fillModeNonSolid: Default::default(),
//             depthBounds: Default::default(),
//             wideLines: Default::default(),
//             largePoints: Default::default(),
//             alphaToOne: Default::default(),
//             multiViewport: Default::default(),
//             samplerAnisotropy: Default::default(),
//             textureCompressionETC2: Default::default(),
//             textureCompressionASTC_LDR: Default::default(),
//             textureCompressionBC: Default::default(),
//             occlusionQueryPrecise: Default::default(),
//             pipelineStatisticsQuery: Default::default(),
//             vertexPipelineStoresAndAtomics: Default::default(),
//             fragmentStoresAndAtomics: Default::default(),
//             shaderTessellationAndGeometryPointSize: Default::default(),
//             shaderImageGatherExtended: Default::default(),
//             shaderStorageImageExtendedFormats: Default::default(),
//             shaderStorageImageMultisample: Default::default(),
//             shaderStorageImageReadWithoutFormat: Default::default(),
//             shaderStorageImageWriteWithoutFormat: Default::default(),
//             shaderUniformBufferArrayDynamicIndexing: Default::default(),
//             shaderSampledImageArrayDynamicIndexing: Default::default(),
//             shaderStorageBufferArrayDynamicIndexing: Default::default(),
//             shaderStorageImageArrayDynamicIndexing: Default::default(),
//             shaderClipDistance: Default::default(),
//             shaderCullDistance: Default::default(),
//             shaderFloat64: Default::default(),
//             shaderInt64: Default::default(),
//             shaderInt16: Default::default(),
//             shaderResourceResidency: Default::default(),
//             shaderResourceMinLod: Default::default(),
//             sparseBinding: Default::default(),
//             sparseResidencyBuffer: Default::default(),
//             sparseResidencyImage2D: Default::default(),
//             sparseResidencyImage3D: Default::default(),
//             sparseResidency2Samples: Default::default(),
//             sparseResidency4Samples: Default::default(),
//             sparseResidency8Samples: Default::default(),
//             sparseResidency16Samples: Default::default(),
//             sparseResidencyAliased: Default::default(),
//             variableMultisampleRate: Default::default(),
//             inheritedQueries: Default::default(),
//         }
//     }
// }

// impl Default for VkPhysicalDeviceVulkan12Features {
//     fn default() -> Self {
//         Self {
//             sType: VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
//             pNext: std::ptr::null_mut(),
//             samplerMirrorClampToEdge: Default::default(),
//             drawIndirectCount: Default::default(),
//             storageBuffer8BitAccess: Default::default(),
//             uniformAndStorageBuffer8BitAccess: Default::default(),
//             storagePushConstant8: Default::default(),
//             shaderBufferInt64Atomics: Default::default(),
//             shaderSharedInt64Atomics: Default::default(),
//             shaderFloat16: Default::default(),
//             shaderInt8: Default::default(),
//             descriptorIndexing: Default::default(),
//             shaderInputAttachmentArrayDynamicIndexing: Default::default(),
//             shaderUniformTexelBufferArrayDynamicIndexing: Default::default(),
//             shaderStorageTexelBufferArrayDynamicIndexing: Default::default(),
//             shaderUniformBufferArrayNonUniformIndexing: Default::default(),
//             shaderSampledImageArrayNonUniformIndexing: Default::default(),
//             shaderStorageBufferArrayNonUniformIndexing: Default::default(),
//             shaderStorageImageArrayNonUniformIndexing: Default::default(),
//             shaderInputAttachmentArrayNonUniformIndexing: Default::default(),
//             shaderUniformTexelBufferArrayNonUniformIndexing: Default::default(),
//             shaderStorageTexelBufferArrayNonUniformIndexing: Default::default(),
//             descriptorBindingUniformBufferUpdateAfterBind: Default::default(),
//             descriptorBindingSampledImageUpdateAfterBind: Default::default(),
//             descriptorBindingStorageImageUpdateAfterBind: Default::default(),
//             descriptorBindingStorageBufferUpdateAfterBind: Default::default(),
//             descriptorBindingUniformTexelBufferUpdateAfterBind: Default::default(),
//             descriptorBindingStorageTexelBufferUpdateAfterBind: Default::default(),
//             descriptorBindingUpdateUnusedWhilePending: Default::default(),
//             descriptorBindingPartiallyBound: Default::default(),
//             descriptorBindingVariableDescriptorCount: Default::default(),
//             runtimeDescriptorArray: Default::default(),
//             samplerFilterMinmax: Default::default(),
//             scalarBlockLayout: Default::default(),
//             imagelessFramebuffer: Default::default(),
//             uniformBufferStandardLayout: Default::default(),
//             shaderSubgroupExtendedTypes: Default::default(),
//             separateDepthStencilLayouts: Default::default(),
//             hostQueryReset: Default::default(),
//             timelineSemaphore: Default::default(),
//             bufferDeviceAddress: Default::default(),
//             bufferDeviceAddressCaptureReplay: Default::default(),
//             bufferDeviceAddressMultiDevice: Default::default(),
//             vulkanMemoryModel: Default::default(),
//             vulkanMemoryModelDeviceScope: Default::default(),
//             vulkanMemoryModelAvailabilityVisibilityChains: Default::default(),
//             shaderOutputViewportIndex: Default::default(),
//             shaderOutputLayer: Default::default(),
//             subgroupBroadcastDynamicId: Default::default(),
//         }
//     }
// }

// impl Default for VkPhysicalDeviceFeatures2 {
//     fn default() -> Self {
//         Self {
//             sType: VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
//             pNext: std::ptr::null_mut(),
//             features: Default::default(),
//         }
//     }
// }

// impl Default for VkMemoryType {
//     fn default() -> Self {
//         Self {
//             propertyFlags: Default::default(),
//             heapIndex: Default::default(),
//         }
//     }
// }

// impl Default for VkMemoryHeap {
//     fn default() -> Self {
//         Self {
//             size: Default::default(),
//             flags: Default::default(),
//         }
//     }
// }

// impl Default for VkPhysicalDeviceMemoryProperties {
//     fn default() -> Self {
//         Self {
//             memoryTypeCount: Default::default(),
//             memoryTypes: [VkMemoryType::default(); 32 as usize],
//             memoryHeapCount: Default::default(),
//             memoryHeaps: [VkMemoryHeap::default(); 16 as usize],
//         }
//     }
// }

// impl Default for VkMemoryRequirements {
//     fn default() -> Self {
//         Self {
//             size: Default::default(),
//             alignment: Default::default(),
//             memoryTypeBits: Default::default(),
//         }
//     }
// }

// impl Default for VkFenceCreateInfo {
//     fn default() -> Self {
//         Self {
//             sType: VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
//             pNext: std::ptr::null(),
//             flags: Default::default(),
//         }
//     }
// }

// impl Default for VkSurfaceCapabilitiesKHR {
//     fn default() -> Self {
//         Self {
//             minImageCount: Default::default(),
//             maxImageCount: Default::default(),
//             currentExtent: VkExtent2D {
//                 width: 0,
//                 height: 0,
//             },
//             minImageExtent: VkExtent2D {
//                 width: 0,
//                 height: 0,
//             },
//             maxImageExtent: VkExtent2D {
//                 width: 0,
//                 height: 0,
//             },
//             maxImageArrayLayers: Default::default(),
//             supportedTransforms: Default::default(),
//             currentTransform: Default::default(),
//             supportedCompositeAlpha: Default::default(),
//             supportedUsageFlags: Default::default(),
//         }
//     }
// }

// impl Default for VkSurfaceFormatKHR {
//     fn default() -> Self {
//         Self {
//             format: Default::default(),
//             colorSpace: Default::default(),
//         }
//     }
// }

// impl Default for VkComponentMapping {
//     fn default() -> Self {
//         Self {
//             r: VK_COMPONENT_SWIZZLE_R,
//             g: VK_COMPONENT_SWIZZLE_G,
//             b: VK_COMPONENT_SWIZZLE_B,
//             a: VK_COMPONENT_SWIZZLE_A,
//         }
//     }
// }

// impl Default for VkStencilOpState {
//     fn default() -> Self {
//         Self {
//             failOp: Default::default(),
//             passOp: Default::default(),
//             depthFailOp: Default::default(),
//             compareOp: Default::default(),
//             compareMask: Default::default(),
//             writeMask: Default::default(),
//             reference: Default::default(),
//         }
//     }
// }

// impl Default for VkPipelineTessellationStateCreateInfo {
//     fn default() -> Self {
//         Self {
//             sType: VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
//             pNext: std::ptr::null(),
//             flags: Default::default(),
//             patchControlPoints: Default::default(),
//         }
//     }
// }

// impl Default for VkPipelineDynamicStateCreateInfo {
//     fn default() -> Self {
//         Self {
//             sType: VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
//             pNext: std::ptr::null(),
//             flags: Default::default(),
//             dynamicStateCount: Default::default(),
//             pDynamicStates: std::ptr::null(),
//         }
//     }
// }

impl VkApplicationInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        application_name: &CStr,
        application_version: u32,
        engine_name: &CStr,
        engine_version: u32,
        api_version: u32,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_APPLICATION_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            pApplicationName: application_name.as_ptr() as *const i8,
            applicationVersion: application_version,
            pEngineName: engine_name.as_ptr() as *const i8,
            engineVersion: engine_version,
            apiVersion: api_version,
        }
    }
}

#[derive(Default)]
pub struct VkApplicationInfoBuilder {
    p_next: Option<*const c_void>,
    application_name: Option<String>,
    application_version: Option<u32>,
    engine_name: Option<String>,
    engine_version: Option<u32>,
    api_version: Option<u32>,
}

impl VkApplicationInfoBuilder {
    pub fn p_next(&mut self, p_next: *const c_void) -> &mut Self {
        self.p_next = Some(p_next);

        self
    }

    pub fn application_name(&mut self, application_name: &CStr) -> &mut Self {
        self.application_name = Some(application_name.to_str().unwrap().to_owned());

        self
    }

    pub fn application_version(&mut self, application_version: u32) -> &mut Self {
        self.application_version = Some(application_version);

        self
    }

    pub fn engine_name(&mut self, engine_name: &CStr) -> &mut Self {
        self.engine_name = Some(engine_name.to_str().unwrap().to_owned());

        self
    }

    pub fn engine_version(&mut self, engine_version: u32) -> &mut Self {
        self.engine_version = Some(engine_version);

        self
    }

    pub fn api_version(&mut self, api_version: u32) -> &mut Self {
        self.api_version = Some(api_version);

        self
    }

    pub fn build(&self) -> VkApplicationInfo {
        VkApplicationInfo {
            sType: VK_STRUCTURE_TYPE_APPLICATION_INFO,
            pNext: match self.p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            pApplicationName: match &self.application_name {
                Some(x) => x.as_ptr() as *const i8,
                None => "Default Name".as_ptr() as *const i8,
            },
            applicationVersion: match self.application_version {
                Some(x) => x,
                None => 0,
            },
            pEngineName: match &self.engine_name {
                Some(x) => x.as_ptr() as *const i8,
                None => "Default Name".as_ptr() as *const i8,
            },
            engineVersion: match self.engine_version {
                Some(x) => x,
                None => 0,
            },
            apiVersion: match self.api_version {
                Some(x) => x,
                None => 0,
            },
        }
    }
}

impl VkInstanceCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkInstanceCreateFlags,
        application_info: VkApplicationInfo,
        enabled_extensions: &Vec<&CStr>,
    ) -> Self {
        let mut i8: Vec<*const i8> = Vec::with_capacity(enabled_extensions.len());

        for ext in enabled_extensions {
            i8.push(ext.as_ptr());
        }

        let this = Self {
            sType: VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            pApplicationInfo: &application_info,
            enabledLayerCount: 0,
            ppEnabledLayerNames: std::ptr::null(),
            enabledExtensionCount: enabled_extensions.len() as u32,
            ppEnabledExtensionNames: i8.as_ptr(),
        };

        std::mem::forget(i8);

        this
    }
}

#[derive(Default)]
pub struct VkInstanceCreateInfoBuilder<'a> {
    p_next: Option<*const c_void>,
    flags: Option<VkInstanceCreateFlags>,
    application_info: Option<&'a VkApplicationInfo>,
    enabled_layer_names: Option<Vec<CString>>,
    enabled_extension_names: Option<Vec<CString>>,
}

impl<'a> VkInstanceCreateInfoBuilder<'a> {
    pub fn p_next(&mut self, p_next: *const c_void) -> &mut Self {
        self.p_next = Some(p_next);

        self
    }

    pub fn flags(&mut self, flags: VkInstanceCreateFlags) -> &mut Self {
        self.flags = Some(flags);

        self
    }

    pub fn application_info(&mut self, application_info: &'a VkApplicationInfo) -> &mut Self {
        self.application_info = Some(application_info);

        self
    }

    pub fn layer_names(&mut self, layer_names: Vec<CString>) -> &mut Self {
        self.enabled_layer_names = Some(layer_names);

        self
    }

    pub fn extension_names(&mut self, extension_names: Vec<CString>) -> &mut Self {
        self.enabled_extension_names = Some(extension_names);

        self
    }

    pub fn build(&self) -> VkInstanceCreateInfo {
        let i8_exts = match &self.enabled_extension_names {
            Some(exts) => {
                let mut ret_vec = vec![];
                for ext in exts {
                    ret_vec.push(ext.as_ptr() as *const i8);
                }

                ret_vec
            }

            None => vec![],
        };

        let ret_val = VkInstanceCreateInfo {
            sType: VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            pNext: match self.p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags: match self.flags {
                Some(x) => x,
                None => 0,
            },
            pApplicationInfo: match self.application_info {
                Some(x) => x,
                None => std::ptr::null(),
            },
            enabledExtensionCount: i8_exts.len() as u32,
            ppEnabledExtensionNames: i8_exts.as_ptr(),
            enabledLayerCount: 0,
            ppEnabledLayerNames: std::ptr::null(),
        };

        std::mem::forget(i8_exts);

        ret_val
    }
}

pub struct Instance {
    pub instance: VkInstance,
}

impl Instance {
    pub fn create(
        create_info: VkInstanceCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut this = Self {
            instance: std::ptr::null_mut(),
        };

        let mut result = VK_SUCCESS;
        unsafe {
            result = vkCreateInstance(
                &create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut this.instance,
            );
        };

        if result >= VK_SUCCESS {
            Ok(this)
        } else {
            Err(result)
        }
    }

    pub fn destroy(&self, allocator: Option<VkAllocationCallbacks>) {
        unsafe {
            vkDestroyInstance(
                self.instance,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn create_win32_surface_khr(
        &self,
        create_info: VkWin32SurfaceCreateInfoKHR,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkSurfaceKHR, VkResult> {
        let mut surface = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateWin32SurfaceKHR(
                self.instance,
                &create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut surface,
            );
        }

        if result >= VK_SUCCESS {
            Ok(surface)
        } else {
            Err(result)
        }
    }

    pub fn destroy_surface_khr(
        &self,
        surface: VkSurfaceKHR,
        allocator: Option<VkAllocationCallbacks>,
    ) {
        unsafe {
            vkDestroySurfaceKHR(
                self.instance,
                surface,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn get_physical_devices(&self) -> Vec<PhysicalDevice> {
        let mut phy_dev_cnt: u32 = 0;
        let mut vk_phy_devs = vec![];

        unsafe {
            vkEnumeratePhysicalDevices(self.instance, &mut phy_dev_cnt, std::ptr::null_mut());
            vk_phy_devs.resize(phy_dev_cnt as usize, std::ptr::null_mut());
            vkEnumeratePhysicalDevices(self.instance, &mut phy_dev_cnt, vk_phy_devs.as_mut_ptr());
        }

        let mut phy_devs: Vec<PhysicalDevice> = Vec::with_capacity(phy_dev_cnt as usize);

        for vk_phy_dev in vk_phy_devs {
            let phy_dev = PhysicalDevice {
                phy_dev: vk_phy_dev,
            };

            phy_devs.push(phy_dev);
        }

        phy_devs
    }
}

#[derive(Copy, Clone)]
pub struct PhysicalDevice {
    pub phy_dev: VkPhysicalDevice,
}

impl PhysicalDevice {
    pub fn properties(&self) -> VkPhysicalDeviceProperties {
        let mut phy_dev_props = VkPhysicalDeviceProperties::default();

        unsafe {
            vkGetPhysicalDeviceProperties(self.phy_dev, &mut phy_dev_props);
        }

        phy_dev_props
    }

    pub fn queue_family_properties(&self) -> Vec<VkQueueFamilyProperties> {
        let mut q_fly_prop_cnt = 0;
        let mut phy_dev_q_props = vec![];

        unsafe {
            vkGetPhysicalDeviceQueueFamilyProperties(
                self.phy_dev,
                &mut q_fly_prop_cnt,
                std::ptr::null_mut(),
            );
            phy_dev_q_props.resize(q_fly_prop_cnt as usize, VkQueueFamilyProperties::default());
            vkGetPhysicalDeviceQueueFamilyProperties(
                self.phy_dev,
                &mut q_fly_prop_cnt,
                phy_dev_q_props.as_mut_ptr(),
            );
        }

        phy_dev_q_props
    }

    pub fn surface_capabilities_khr(
        &self,
        surface: &VkSurfaceKHR,
    ) -> Result<VkSurfaceCapabilitiesKHR, VkResult> {
        let mut surf_caps = VkSurfaceCapabilitiesKHR::default();
        let mut result = VK_SUCCESS;

        unsafe {
            result =
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(self.phy_dev, *surface, &mut surf_caps);
        }

        if result >= VK_SUCCESS {
            Ok(surf_caps)
        } else {
            Err(result)
        }
    }

    pub fn surface_formats_khr(
        &self,
        surface: &VkSurfaceKHR,
    ) -> Result<Vec<VkSurfaceFormatKHR>, VkResult> {
        let mut surface_formats = vec![];
        let mut result = VK_SUCCESS;

        unsafe {
            let mut format_count: u32 = 0;
            result = vkGetPhysicalDeviceSurfaceFormatsKHR(
                self.phy_dev,
                *surface,
                &mut format_count,
                std::ptr::null_mut(),
            );

            surface_formats.resize(format_count as usize, VkSurfaceFormatKHR::default());
            result = vkGetPhysicalDeviceSurfaceFormatsKHR(
                self.phy_dev,
                *surface,
                &mut format_count,
                surface_formats.as_mut_ptr(),
            );
        }

        if result >= VK_SUCCESS {
            Ok(surface_formats)
        } else {
            Err(result)
        }
    }

    pub fn surface_present_modes_khr(
        &self,
        surface: &VkSurfaceKHR,
    ) -> Result<Vec<VkPresentModeKHR>, VkResult> {
        let mut present_modes = vec![];
        let mut result = VK_SUCCESS;

        unsafe {
            let mut present_mode_count = 0;
            result = vkGetPhysicalDeviceSurfacePresentModesKHR(
                self.phy_dev,
                *surface,
                &mut present_mode_count,
                std::ptr::null_mut(),
            );

            present_modes.resize(present_mode_count as usize, VkPresentModeKHR::default());
            result = vkGetPhysicalDeviceSurfacePresentModesKHR(
                self.phy_dev,
                *surface,
                &mut present_mode_count,
                present_modes.as_mut_ptr(),
            );
        }

        if result >= VK_SUCCESS {
            Ok(present_modes)
        } else {
            Err(result)
        }
    }

    pub fn surface_support_khr(
        &self,
        q_fly_idx: usize,
        surface: VkSurfaceKHR,
    ) -> Result<bool, i32> {
        let mut result = VK_SUCCESS;
        let mut is_supported = 0;

        unsafe {
            result = vkGetPhysicalDeviceSurfaceSupportKHR(
                self.phy_dev,
                q_fly_idx as u32,
                surface,
                &mut is_supported,
            );
        }

        if result >= VK_SUCCESS {
            Ok(is_supported != 0)
        } else {
            Err(result)
        }
    }

    pub fn win32_presentation_support_khr(&self, q_fly_idx: usize) -> bool {
        unsafe {
            vkGetPhysicalDeviceWin32PresentationSupportKHR(self.phy_dev, q_fly_idx as u32) != 0
        }
    }

    pub fn features(&self) -> VkPhysicalDeviceFeatures {
        let mut features = VkPhysicalDeviceFeatures::default();

        unsafe {
            vkGetPhysicalDeviceFeatures(self.phy_dev, &mut features);
        }

        features
    }

    pub fn features2(&self) -> VkPhysicalDeviceFeatures2 {
        let mut features2 = VkPhysicalDeviceFeatures2::default();
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        unsafe {
            vkGetPhysicalDeviceFeatures2(self.phy_dev, &mut features2);
        }

        features2
    }

    pub fn memory_properties(&self) -> VkPhysicalDeviceMemoryProperties {
        let mut mem_props = VkPhysicalDeviceMemoryProperties::default();

        unsafe {
            vkGetPhysicalDeviceMemoryProperties(self.phy_dev, &mut mem_props);
        }

        mem_props
    }
}

impl VkPhysicalDeviceDynamicRenderingFeatures {
    pub fn new(p_next: Option<*mut c_void>) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
            dynamicRendering: 1,
        }
    }
}

#[derive(Default)]
pub struct VkPhysicalDeviceDynamicRenderingFeaturesBuilder {
    p_next: Option<*mut c_void>,
    dynamic_rendering: Option<u32>,
}

impl VkPhysicalDeviceDynamicRenderingFeaturesBuilder {
    pub fn p_next(&mut self, p_next: *mut c_void) -> &mut Self {
        self.p_next = Some(p_next);

        self
    }

    pub fn dynamic_rendering(&mut self, dynamic_rendering: u32) -> &mut Self {
        self.dynamic_rendering = Some(dynamic_rendering);

        self
    }

    pub fn build(&self) -> VkPhysicalDeviceDynamicRenderingFeatures {
        VkPhysicalDeviceDynamicRenderingFeatures {
            sType: VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            pNext: match self.p_next {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
            dynamicRendering: match self.dynamic_rendering {
                Some(x) => x,
                None => 0,
            },
        }
    }
}

impl VkDeviceQueueCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkDeviceQueueCreateFlags,
        q_fly_idx: u32,
        q_cnt: u32,
        priorities: &Vec<f32>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            queueFamilyIndex: q_fly_idx,
            queueCount: q_cnt,
            pQueuePriorities: priorities.as_ptr(),
        }
    }
}

#[derive(Default)]
pub struct VkDeviceQueueCreateInfoBuilder<'a> {
    p_next: Option<*const c_void>,
    flags: Option<VkDeviceQueueCreateFlags>,
    q_fly_idx: Option<u32>,
    priorities: Option<&'a Vec<f32>>,
}

impl<'a> VkDeviceQueueCreateInfoBuilder<'a> {
    pub fn p_next(&mut self, p_next: *const c_void) -> &mut Self {
        self.p_next = Some(p_next);

        self
    }

    pub fn flags(&mut self, flags: VkDeviceQueueCreateFlags) -> &mut Self {
        self.flags = Some(flags);

        self
    }

    pub fn q_family_index(&mut self, q_fly_idx: u32) -> &mut Self {
        self.q_fly_idx = Some(q_fly_idx);

        self
    }

    pub fn priorities(&mut self, priorities: &'a Vec<f32>) -> &mut Self {
        self.priorities = Some(priorities);

        self
    }

    pub fn build(&self) -> VkDeviceQueueCreateInfo {
        VkDeviceQueueCreateInfo {
            sType: VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            pNext: match self.p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags: match self.flags {
                Some(x) => x,
                None => 0,
            },
            queueFamilyIndex: match self.q_fly_idx {
                Some(x) => x,
                None => 0,
            },
            queueCount: match self.priorities {
                Some(x) => x.len() as u32,
                None => 0,
            },
            pQueuePriorities: match self.priorities {
                Some(x) => x.as_ptr(),
                None => std::ptr::null(),
            },
        }
    }
}

impl VkWin32SurfaceCreateInfoKHR {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkWin32SurfaceCreateFlagsKHR,
        h_instance: *mut HINSTANCE__,
        h_wnd: *mut HWND__,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            hinstance: h_instance,
            hwnd: h_wnd,
        }
    }
}

#[derive(Default)]
pub struct VkWin32SurfaceCreateInfoKHRBuilder {
    p_next: Option<*const c_void>,
    flags: Option<VkWin32SurfaceCreateFlagsKHR>,
    h_instance: Option<*mut HINSTANCE__>,
    h_wnd: Option<*mut HWND__>,
}

impl VkWin32SurfaceCreateInfoKHRBuilder {
    pub fn p_next(&mut self, p_next: *const c_void) -> &mut Self {
        self.p_next = Some(p_next);

        self
    }

    pub fn flags(&mut self, flags: VkWin32SurfaceCreateFlagsKHR) -> &mut Self {
        self.flags = Some(flags);

        self
    }

    pub fn h_instance(&mut self, h_instance: *mut HINSTANCE__) -> &mut Self {
        self.h_instance = Some(h_instance);

        self
    }

    pub fn h_wnd(&mut self, h_wnd: *mut HWND__) -> &mut Self {
        self.h_wnd = Some(h_wnd);

        self
    }

    pub fn build(&self) -> VkWin32SurfaceCreateInfoKHR {
        VkWin32SurfaceCreateInfoKHR {
            sType: VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            pNext: match self.p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags: match self.flags {
                Some(x) => x,
                None => 0,
            },
            hinstance: match self.h_instance {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
            hwnd: match self.h_wnd {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
        }
    }
}

impl VkDeviceCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkDeviceCreateFlags,
        q_cis: &Vec<VkDeviceQueueCreateInfo>,
        enabled_extensions: &Vec<&CStr>,
        enabled_features: Option<&VkPhysicalDeviceFeatures>,
    ) -> Self {
        let mut i8: Vec<*const i8> = Vec::with_capacity(enabled_extensions.len());

        for ext in enabled_extensions {
            i8.push(ext.as_ptr());
        }

        let this = Self {
            sType: VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            queueCreateInfoCount: q_cis.len() as u32,
            pQueueCreateInfos: q_cis.as_ptr(),
            enabledLayerCount: 0,
            ppEnabledLayerNames: std::ptr::null(),
            enabledExtensionCount: enabled_extensions.len() as u32,
            ppEnabledExtensionNames: i8.as_ptr(),
            pEnabledFeatures: match enabled_features {
                Some(x) => x,
                None => std::ptr::null(),
            },
        };

        std::mem::forget(i8);

        this
    }
}

#[derive(Default)]
pub struct VkDeviceCreateInfoBuilder<'a> {
    p_next: Option<*const c_void>,
    flags: Option<VkDeviceCreateFlags>,
    q_cis: Option<&'a Vec<VkDeviceQueueCreateInfo>>,
    enabled_extensions: Option<Vec<CString>>,
    enabled_features: Option<VkPhysicalDeviceFeatures>,
}

impl<'a> VkDeviceCreateInfoBuilder<'a> {
    pub fn p_next(&mut self, p_next: *const c_void) -> &mut Self {
        self.p_next = Some(p_next);

        self
    }

    pub fn flags(&mut self, flags: VkDeviceCreateFlags) -> &mut Self {
        self.flags = Some(flags);

        self
    }

    pub fn queue_create_infos(&mut self, q_cis: &'a Vec<VkDeviceQueueCreateInfo>) -> &mut Self {
        self.q_cis = Some(q_cis);

        self
    }

    pub fn enabled_extensions(&mut self, enabled_extensions: Vec<CString>) -> &mut Self {
        self.enabled_extensions = Some(enabled_extensions);

        self
    }

    pub fn enabled_features(&mut self, enabled_features: VkPhysicalDeviceFeatures) -> &mut Self {
        self.enabled_features = Some(enabled_features);

        self
    }

    pub fn build(&self) -> VkDeviceCreateInfo {
        let i8_exts = match &self.enabled_extensions {
            Some(exts) => {
                let mut ret_vec = vec![];

                for ext in exts {
                    ret_vec.push(ext.as_ptr() as *const i8);
                }

                ret_vec
            }
            None => vec![],
        };

        let ret_val = VkDeviceCreateInfo {
            sType: VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            pNext: match self.p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags: match self.flags {
                Some(x) => x,
                None => 0,
            },
            queueCreateInfoCount: match self.q_cis {
                Some(x) => x.len() as u32,
                None => 0,
            },
            pQueueCreateInfos: match self.q_cis {
                Some(x) => x.as_ptr(),
                None => std::ptr::null(),
            },
            enabledLayerCount: 0,
            ppEnabledLayerNames: std::ptr::null(),
            enabledExtensionCount: i8_exts.len() as u32,
            ppEnabledExtensionNames: i8_exts.as_ptr(),
            pEnabledFeatures: match self.enabled_features {
                Some(x) => &x,
                None => std::ptr::null(),
            },
        };

        std::mem::forget(i8_exts);

        ret_val
    }
}

impl VkSwapchainCreateInfoKHR {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkSwapchainCreateFlagsKHR,
        surface: VkSurfaceKHR,
        min_image_count: u32,
        image_format: VkFormat,
        image_color_space: VkColorSpaceKHR,
        image_extent: VkExtent2D,
        image_array_layers: u32,
        image_usage: VkImageUsageFlags,
        image_sharing_mode: VkSharingMode,
        q_fly_idx: &Vec<u32>,
        pre_transform: VkSurfaceTransformFlagsKHR,
        composite_alpha: VkCompositeAlphaFlagsKHR,
        present_mode: VkPresentModeKHR,
        clipped: u32,
        old_swapchain: Option<VkSwapchainKHR>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            surface,
            minImageCount: min_image_count,
            imageFormat: image_format,
            imageColorSpace: image_color_space,
            imageExtent: image_extent,
            imageArrayLayers: image_array_layers,
            imageUsage: image_usage,
            imageSharingMode: image_sharing_mode,
            queueFamilyIndexCount: q_fly_idx.len() as u32,
            pQueueFamilyIndices: q_fly_idx.as_ptr(),
            preTransform: pre_transform as i32,
            compositeAlpha: composite_alpha as i32,
            presentMode: present_mode,
            clipped,
            oldSwapchain: match old_swapchain {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
        }
    }
}

#[derive(Default)]
pub struct VkSwapchainCreateInfoKHRBuilder<'a> {
    p_next: Option<*const c_void>,
    flags: Option<VkSwapchainCreateFlagsKHR>,
    surface: Option<VkSurfaceKHR>,
    min_image_count: Option<u32>,
    image_format: Option<VkFormat>,
    image_color_space: Option<VkColorSpaceKHR>,
    image_extent: Option<VkExtent2D>,
    image_array_layers: Option<u32>,
    image_usage: Option<VkImageUsageFlags>,
    image_sharing_mode: Option<VkSharingMode>,
    q_fly_idxs: Option<&'a Vec<u32>>,
    pre_tranform: Option<VkSurfaceTransformFlagsKHR>,
    composite_alpha: Option<VkCompositeAlphaFlagsKHR>,
    present_mode: Option<VkPresentModeKHR>,
    clipped: Option<u32>,
    old_swapchain: Option<VkSwapchainKHR>,
}

impl<'a> VkSwapchainCreateInfoKHRBuilder<'a> {
    pub fn p_next(&mut self, p_next: *const c_void) -> &mut Self {
        self.p_next = Some(p_next);

        self
    }

    pub fn flags(&mut self, flags: VkSwapchainCreateFlagsKHR) -> &mut Self {
        self.flags = Some(flags);

        self
    }

    pub fn surface(&mut self, surface: VkSurfaceKHR) -> &mut Self {
        self.surface = Some(surface);

        self
    }

    pub fn min_image_count(&mut self, min_image_count: u32) -> &mut Self {
        self.min_image_count = Some(min_image_count);

        self
    }

    pub fn image_format(&mut self, image_format: VkFormat) -> &mut Self {
        self.image_format = Some(image_format);

        self
    }

    pub fn image_color_space(&mut self, image_color_space: VkColorSpaceKHR) -> &mut Self {
        self.image_color_space = Some(image_color_space);

        self
    }

    pub fn image_extent(&mut self, image_extent: VkExtent2D) -> &mut Self {
        self.image_extent = Some(image_extent);

        self
    }

    pub fn image_array_layers(&mut self, image_array_layers: u32) -> &mut Self {
        self.image_array_layers = Some(image_array_layers);

        self
    }

    pub fn image_usage(&mut self, image_usage: VkImageUsageFlags) -> &mut Self {
        self.image_usage = Some(image_usage);

        self
    }

    pub fn image_sharing_mode(&mut self, image_sharing_mode: VkSharingMode) -> &mut Self {
        self.image_sharing_mode = Some(image_sharing_mode);

        self
    }

    pub fn q_family_indices(&mut self, q_fly_indices: &'a Vec<u32>) -> &mut Self {
        self.q_fly_idxs = Some(q_fly_indices);

        self
    }

    pub fn pre_tranform(&mut self, pre_transform: VkSurfaceTransformFlagsKHR) -> &mut Self {
        self.pre_tranform = Some(pre_transform);

        self
    }

    pub fn composite_alpha(&mut self, composite_alpha: VkCompositeAlphaFlagsKHR) -> &mut Self {
        self.composite_alpha = Some(composite_alpha);

        self
    }

    pub fn present_mode(&mut self, present_mode: VkPresentModeKHR) -> &mut Self {
        self.present_mode = Some(present_mode);

        self
    }

    pub fn clipped(&mut self, clipped: u32) -> &mut Self {
        self.clipped = Some(clipped);

        self
    }

    pub fn old_swapchain(&mut self, old_swapchain: VkSwapchainKHR) -> &mut Self {
        self.old_swapchain = Some(old_swapchain);

        self
    }

    pub fn build(&self) -> VkSwapchainCreateInfoKHR {
        VkSwapchainCreateInfoKHR {
            sType: VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            pNext: match self.p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags: match self.flags {
                Some(x) => x,
                None => 0,
            },
            surface: match self.surface {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
            minImageCount: match self.min_image_count {
                Some(x) => x,
                None => 0,
            },
            imageFormat: match self.image_format {
                Some(x) => x,
                None => 0,
            },
            imageColorSpace: match self.image_color_space {
                Some(x) => x,
                None => 0,
            },
            imageExtent: match self.image_extent {
                Some(x) => x,
                None => VkExtent2D::default(),
            },
            imageArrayLayers: match self.image_array_layers {
                Some(x) => x,
                None => 0,
            },
            imageUsage: match self.image_usage {
                Some(x) => x,
                None => 0,
            },
            imageSharingMode: match self.image_sharing_mode {
                Some(x) => x,
                None => 0,
            },
            queueFamilyIndexCount: match self.q_fly_idxs {
                Some(x) => x.len() as u32,
                None => 0,
            },
            pQueueFamilyIndices: match self.q_fly_idxs {
                Some(x) => x.as_ptr(),
                None => std::ptr::null(),
            },
            preTransform: match self.pre_tranform {
                Some(x) => x as i32,
                None => 0,
            },
            compositeAlpha: match self.composite_alpha {
                Some(x) => x as i32,
                None => 0,
            },
            presentMode: match self.present_mode {
                Some(x) => x,
                None => 0,
            },
            clipped: match self.clipped {
                Some(x) => x,
                None => 0,
            },
            oldSwapchain: match self.old_swapchain {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
        }
    }
}

pub struct Device {
    pub device: VkDevice,
}

impl Device {
    pub fn create(
        phy_dev: &PhysicalDevice,
        create_info: VkDeviceCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut this = Self {
            device: std::ptr::null_mut(),
        };

        let mut result = VK_SUCCESS;
        unsafe {
            result = vkCreateDevice(
                phy_dev.phy_dev,
                &create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut this.device,
            );
        }

        if result >= VK_SUCCESS {
            Ok(this)
        } else {
            Err(result)
        }
    }

    pub fn create_swapchain_khr(
        &self,
        create_info: VkSwapchainCreateInfoKHR,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkSwapchainKHR, VkResult> {
        let mut swapchain = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateSwapchainKHR(
                self.device,
                &create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut swapchain,
            );
        }

        if result >= VK_SUCCESS {
            Ok(swapchain)
        } else {
            Err(result)
        }
    }

    pub fn get_swapchain_images_khr(
        &self,
        swapchain: VkSwapchainKHR,
    ) -> Result<Vec<VkImage>, VkResult> {
        let mut images = vec![];
        let mut result = VK_SUCCESS;

        unsafe {
            let mut image_count = 0u32;

            result = vkGetSwapchainImagesKHR(
                self.device,
                swapchain,
                &mut image_count,
                std::ptr::null_mut(),
            );

            images.resize(image_count as usize, std::ptr::null_mut());

            result = vkGetSwapchainImagesKHR(
                self.device,
                swapchain,
                &mut image_count,
                images.as_mut_ptr(),
            );
        }

        if result >= VK_SUCCESS {
            Ok(images)
        } else {
            Err(result)
        }
    }

    pub fn create_image(
        &self,
        create_info: VkImageCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkImage, VkResult> {
        let mut image = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateImage(
                self.device,
                &create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut image,
            );
        }

        if result >= VK_SUCCESS {
            Ok(image)
        } else {
            Err(result)
        }
    }

    pub fn destroy_image(&self, image: VkImage, allocator: Option<VkAllocationCallbacks>) {
        unsafe {
            vkDestroyImage(
                self.device,
                image,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn create_image_view(
        &self,
        create_info: VkImageViewCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkImageView, VkResult> {
        let mut image_view = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateImageView(
                self.device,
                &create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut image_view,
            );
        }

        if result >= VK_SUCCESS {
            Ok(image_view)
        } else {
            Err(result)
        }
    }

    pub fn destroy_image_view(
        &self,
        image_view: VkImageView,
        allocator: Option<VkAllocationCallbacks>,
    ) {
        unsafe {
            vkDestroyImageView(
                self.device,
                image_view,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn create_buffer(
        &self,
        create_info: &VkBufferCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkBuffer, VkResult> {
        let mut buffer = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateBuffer(
                self.device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut buffer,
            );
        }

        if result >= VK_SUCCESS {
            Ok(buffer)
        } else {
            Err(result)
        }
    }

    pub fn acquire_next_image_khr(
        &self,
        swapchain: VkSwapchainKHR,
        timeout: u64,
        semaphore: Option<VkSemaphore>,
        fence: Option<VkFence>,
    ) -> Result<usize, VkResult> {
        let mut result = VK_SUCCESS;
        let mut img_idx = 0;

        unsafe {
            result = vkAcquireNextImageKHR(
                self.device,
                swapchain,
                timeout,
                match semaphore {
                    Some(x) => x,
                    None => std::ptr::null_mut(),
                },
                match fence {
                    Some(x) => x,
                    None => std::ptr::null_mut(),
                },
                &mut img_idx,
            )
        }

        if result >= VK_SUCCESS {
            Ok(img_idx as usize)
        } else {
            Err(result)
        }
    }

    pub fn buffer_memory_requirements(&self, buffer: VkBuffer) -> VkMemoryRequirements {
        let mut mem_reqs = VkMemoryRequirements::default();

        unsafe {
            vkGetBufferMemoryRequirements(self.device, buffer, &mut mem_reqs);
        }

        mem_reqs
    }

    pub fn image_memory_requirements(&self, image: VkImage) -> VkMemoryRequirements {
        let mut mem_reqs = VkMemoryRequirements::default();

        unsafe {
            vkGetImageMemoryRequirements(self.device, image, &mut mem_reqs);
        }

        mem_reqs
    }

    pub fn allocate_memory(
        &self,
        allocate_info: &VkMemoryAllocateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkDeviceMemory, VkResult> {
        let mut dev_mem = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkAllocateMemory(
                self.device,
                allocate_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut dev_mem,
            );
        }

        if result >= VK_SUCCESS {
            Ok(dev_mem)
        } else {
            Err(result)
        }
    }

    pub fn bind_buffer_memory(
        &self,
        buffer: VkBuffer,
        memory: VkDeviceMemory,
        offset: VkDeviceSize,
    ) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkBindBufferMemory(self.device, buffer, memory, offset);
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn bind_image_memory(
        &self,
        image: VkImage,
        memory: VkDeviceMemory,
        offset: VkDeviceSize,
    ) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkBindImageMemory(self.device, image, memory, offset);
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn map_memory(
        &self,
        memory: VkDeviceMemory,
        offset: VkDeviceSize,
        size: VkDeviceSize,
        flags: VkMemoryMapFlags,
    ) -> Result<*mut c_void, VkResult> {
        let mut mapped_ptr = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkMapMemory(self.device, memory, offset, size, flags, &mut mapped_ptr);
        }

        if result >= VK_SUCCESS {
            Ok(mapped_ptr)
        } else {
            Err(result)
        }
    }

    pub fn create_shader_module(
        &self,
        create_info: VkShaderModuleCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkShaderModule, VkResult> {
        let mut shader_mod = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateShaderModule(
                self.device,
                &create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut shader_mod,
            );
        }

        if result >= VK_SUCCESS {
            Ok(shader_mod)
        } else {
            Err(result)
        }
    }

    pub fn create_descriptor_pool(
        &self,
        create_info: &VkDescriptorPoolCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkDescriptorPool, VkResult> {
        let mut result = VK_SUCCESS;
        let mut desc_pool = std::ptr::null_mut();

        unsafe {
            result = vkCreateDescriptorPool(
                self.device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut desc_pool,
            );
        }

        if result >= VK_SUCCESS {
            Ok(desc_pool)
        } else {
            Err(result)
        }
    }

    pub fn create_descriptor_set_layout(
        &self,
        create_info: &VkDescriptorSetLayoutCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkDescriptorSetLayout, VkResult> {
        let mut result = VK_SUCCESS;
        let mut desc_set = std::ptr::null_mut();

        unsafe {
            result = vkCreateDescriptorSetLayout(
                self.device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut desc_set,
            );
        }

        if result >= VK_SUCCESS {
            Ok(desc_set)
        } else {
            Err(result)
        }
    }

    pub fn allocate_descriptor_sets(
        &self,
        alloc_info: &VkDescriptorSetAllocateInfo,
    ) -> Result<Vec<VkDescriptorSet>, VkResult> {
        let mut result = VK_SUCCESS;
        let mut desc_sets = vec![std::ptr::null_mut(); alloc_info.descriptorSetCount as usize];

        unsafe {
            result = vkAllocateDescriptorSets(self.device, alloc_info, desc_sets.as_mut_ptr());
        }

        if result >= VK_SUCCESS {
            Ok(desc_sets)
        } else {
            Err(result)
        }
    }

    pub fn create_pipeline_layout(
        &self,
        create_info: &VkPipelineLayoutCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkPipelineLayout, VkResult> {
        let mut result = VK_SUCCESS;
        let mut pipe_lyt = std::ptr::null_mut();

        unsafe {
            result = vkCreatePipelineLayout(
                self.device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut pipe_lyt,
            );
        }

        if result >= VK_SUCCESS {
            Ok(pipe_lyt)
        } else {
            Err(result)
        }
    }

    pub fn create_compute_pipelines(
        &self,
        pipe_cache: VkPipelineCache,
        create_infos: &Vec<VkComputePipelineCreateInfo>,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Vec<VkPipeline>, VkResult> {
        let mut result = VK_SUCCESS;
        let mut pipelines = vec![std::ptr::null_mut(); create_infos.len() as usize];

        unsafe {
            result = vkCreateComputePipelines(
                self.device,
                pipe_cache,
                create_infos.len() as u32,
                create_infos.as_ptr(),
                match allocator {
                    Some(x) => std::ptr::addr_of!(x),
                    None => std::ptr::null(),
                },
                pipelines.as_mut_ptr(),
            )
        }

        if result >= VK_SUCCESS {
            Ok(pipelines)
        } else {
            Err(result)
        }
    }

    pub fn create_graphics_pipelines(
        &self,
        pipeline_cache: Option<VkPipelineCache>,
        create_infos: &Vec<VkGraphicsPipelineCreateInfo>,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Vec<VkPipeline>, VkResult> {
        let mut result = VK_SUCCESS;
        let mut pipelines = vec![std::ptr::null_mut(); create_infos.len() as usize];

        unsafe {
            result = vkCreateGraphicsPipelines(
                self.device,
                match pipeline_cache {
                    Some(x) => x,
                    None => std::ptr::null_mut(),
                },
                create_infos.len() as u32,
                create_infos.as_ptr(),
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                pipelines.as_mut_ptr(),
            );
        }

        if result >= VK_SUCCESS {
            Ok(pipelines)
        } else {
            Err(result)
        }
    }

    pub fn update_descriptor_sets(
        &self,
        desc_writes: &Vec<VkWriteDescriptorSet>,
        desc_copies: &Vec<VkCopyDescriptorSet>,
    ) {
        unsafe {
            vkUpdateDescriptorSets(
                self.device,
                desc_writes.len() as u32,
                desc_writes.as_ptr(),
                desc_copies.len() as u32,
                desc_copies.as_ptr(),
            );
        }
    }

    pub fn create_command_pool(
        &self,
        &create_info: &VkCommandPoolCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkCommandPool, VkResult> {
        let mut result = VK_SUCCESS;
        let mut cmd_pool = std::ptr::null_mut();

        unsafe {
            result = vkCreateCommandPool(
                self.device,
                &create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut cmd_pool,
            );
        }

        if result >= VK_SUCCESS {
            Ok(cmd_pool)
        } else {
            Err(result)
        }
    }

    pub fn allocate_command_buffers(
        &self,
        alloc_info: &VkCommandBufferAllocateInfo,
    ) -> Result<CommandBuffer, VkResult> {
        let mut result = VK_SUCCESS;
        let mut vk_cmd_buffs = vec![std::ptr::null_mut(); alloc_info.commandBufferCount as usize];

        unsafe {
            result = vkAllocateCommandBuffers(self.device, alloc_info, vk_cmd_buffs.as_mut_ptr());
        }

        if result >= VK_SUCCESS {
            let mut cmd_buff = CommandBuffer {
                cmd_buffs: Vec::with_capacity(alloc_info.commandBufferCount as usize),
            };

            for vk_cmd_buff in vk_cmd_buffs {
                cmd_buff.cmd_buffs.push(vk_cmd_buff);
            }

            Ok(cmd_buff)
        } else {
            Err(result)
        }
    }

    pub fn get_queue(&self, q_fly_idx: u32, q_idx: u32) -> Queue {
        let mut q: Queue = Queue {
            q: std::ptr::null_mut(),
        };

        unsafe {
            vkGetDeviceQueue(self.device, q_fly_idx, q_idx, &mut q.q);
        }

        q
    }

    pub fn create_fence(
        &self,
        create_info: &VkFenceCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkFence, VkResult> {
        let mut result = VK_SUCCESS;
        let mut f: VkFence = std::ptr::null_mut();

        unsafe {
            result = vkCreateFence(
                self.device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut f,
            )
        }

        if result >= VK_SUCCESS {
            Ok(f)
        } else {
            Err(result)
        }
    }

    pub fn wait_for_fences(
        &self,
        fences: &Vec<VkFence>,
        wait_all: bool,
        timeout: u64,
    ) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkWaitForFences(
                self.device,
                fences.len() as u32,
                fences.as_ptr(),
                wait_all as VkBool32,
                timeout,
            );
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn reset_fences(&self, fences: &Vec<VkFence>) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkResetFences(self.device, fences.len() as u32, fences.as_ptr());
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn destroy_fence(&self, fence: VkFence, allocator: Option<VkAllocationCallbacks>) {
        unsafe {
            vkDestroyFence(
                self.device,
                fence,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn create_semaphore(
        &self,
        create_info: &VkSemaphoreCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkSemaphore, VkResult> {
        let mut result = VK_SUCCESS;
        let mut semaphore: VkSemaphore = std::ptr::null_mut();

        unsafe {
            result = vkCreateSemaphore(
                self.device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut semaphore,
            );
        }

        if result >= VK_SUCCESS {
            Ok(semaphore)
        } else {
            Err(result)
        }
    }

    pub fn destroy_semaphore(
        &self,
        semaphore: VkSemaphore,
        allocator: Option<VkAllocationCallbacks>,
    ) {
        unsafe {
            vkDestroySemaphore(
                self.device,
                semaphore,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn free_command_buffers(&self, cmd_pool: VkCommandPool, cmd_buff: CommandBuffer) {
        unsafe {
            vkFreeCommandBuffers(
                self.device,
                cmd_pool,
                cmd_buff.cmd_buffs.len() as u32,
                cmd_buff.cmd_buffs.as_ptr(),
            );
        }
    }

    pub fn destroy_command_pool(
        &self,
        cmd_pool: VkCommandPool,
        allocator: Option<VkAllocationCallbacks>,
    ) {
        unsafe {
            vkDestroyCommandPool(
                self.device,
                cmd_pool,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn destroy_pipelines(
        &self,
        pipelines: &Vec<VkPipeline>,
        allocator: Option<VkAllocationCallbacks>,
    ) {
        for pipeline in pipelines {
            unsafe {
                vkDestroyPipeline(
                    self.device,
                    *pipeline,
                    match allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }

    pub fn destroy_pipeline_layout(
        &self,
        pipe_layout: VkPipelineLayout,
        allocator: Option<VkAllocationCallbacks>,
    ) {
        unsafe {
            vkDestroyPipelineLayout(
                self.device,
                pipe_layout,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn free_descriptor_sets(
        &self,
        desc_pool: VkDescriptorPool,
        desc_set_count: u32,
        desc_sets: Vec<VkDescriptorSet>,
    ) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;

        unsafe {
            result =
                vkFreeDescriptorSets(self.device, desc_pool, desc_set_count, desc_sets.as_ptr());
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn destroy_descriptor_set_layout(
        &self,
        desc_set_lyt: VkDescriptorSetLayout,
        allocator: Option<VkAllocationCallbacks>,
    ) {
        unsafe {
            vkDestroyDescriptorSetLayout(
                self.device,
                desc_set_lyt,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn destroy_descriptor_pool(
        &self,
        desc_pool: VkDescriptorPool,
        allocator: Option<VkAllocationCallbacks>,
    ) {
        unsafe {
            vkDestroyDescriptorPool(
                self.device,
                desc_pool,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn destroy_swapchain_khr(
        &self,
        swapchain: VkSwapchainKHR,
        allocator: Option<VkAllocationCallbacks>,
    ) {
        unsafe {
            vkDestroySwapchainKHR(
                self.device,
                swapchain,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }
    pub fn destroy_buffer(&self, buffer: VkBuffer, allocator: Option<VkAllocationCallbacks>) {
        unsafe {
            vkDestroyBuffer(
                self.device,
                buffer,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            )
        }
    }

    pub fn destroy_shader_module(
        &self,
        shader_module: VkShaderModule,
        allocator: Option<VkAllocationCallbacks>,
    ) {
        unsafe {
            vkDestroyShaderModule(
                self.device,
                shader_module,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn unmap_memory(&self, memory: VkDeviceMemory) {
        unsafe {
            vkUnmapMemory(self.device, memory);
        }
    }

    pub fn free_memory(&self, memory: VkDeviceMemory, allocator: Option<VkAllocationCallbacks>) {
        unsafe {
            vkFreeMemory(
                self.device,
                memory,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
            );
        }
    }

    pub fn destroy(&self, allocator: Option<VkAllocationCallbacks>) {
        if self.device != std::ptr::null_mut() {
            unsafe {
                vkDestroyDevice(
                    self.device,
                    match allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl VkBufferCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkBufferCreateFlags,
        size: VkDeviceSize,
        usage: VkBufferUsageFlags,
        sharing_mode: VkSharingMode,
        q_fly_idx: &Vec<u32>,
    ) -> Self {
        let this = Self {
            sType: VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            size,
            usage,
            sharingMode: sharing_mode,
            queueFamilyIndexCount: q_fly_idx.len() as u32,
            pQueueFamilyIndices: q_fly_idx.as_ptr(),
        };

        this
    }
}

impl VkMemoryAllocateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        allocation_size: VkDeviceSize,
        memory_type_index: u32,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            allocationSize: allocation_size,
            memoryTypeIndex: memory_type_index,
        }
    }
}

#[derive(Default)]
pub struct VkMemoryAllocateInfoBuilder {
    p_next: Option<*const c_void>,
    allocation_size: Option<VkDeviceSize>,
    memory_type_index: Option<u32>,
}

impl VkMemoryAllocateInfoBuilder {
    pub fn p_next(&mut self, p_next: *const c_void) -> &mut Self {
        self.p_next = Some(p_next);

        self
    }

    pub fn allocation_size(&mut self, allocation_size: VkDeviceSize) -> &mut Self {
        self.allocation_size = Some(allocation_size);

        self
    }

    pub fn memory_type_index(&mut self, memory_type_index: u32) -> &mut Self {
        self.memory_type_index = Some(memory_type_index);

        self
    }

    pub fn build(&self) -> VkMemoryAllocateInfo {
        VkMemoryAllocateInfo {
            sType: VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            pNext: match self.p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            allocationSize: match self.allocation_size {
                Some(x) => x,
                None => 0,
            },
            memoryTypeIndex: match self.memory_type_index {
                Some(x) => x,
                None => 0,
            },
        }
    }
}

impl VkShaderModuleCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkShaderModuleCreateFlags,
        code: &Vec<u8>,
    ) -> Self {
        let this = Self {
            sType: VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            codeSize: code.len(),
            pCode: code.as_ptr() as *const u32,
        };

        this
    }
}

impl VkDescriptorPoolCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkDescriptorPoolCreateFlags,
        max_sets: u32,
        pool_sizes: &Vec<VkDescriptorPoolSize>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            maxSets: max_sets,
            poolSizeCount: pool_sizes.len() as u32,
            pPoolSizes: pool_sizes.as_ptr(),
        }
    }
}

impl VkDescriptorSetLayoutBinding {
    pub fn new(
        binding: u32,
        descriptor_type: VkDescriptorType,
        descriptor_count: u32,
        stage_flags: VkShaderStageFlags,
        immut_samplers: Option<*mut VkSampler>,
    ) -> Self {
        Self {
            binding,
            descriptorType: descriptor_type,
            descriptorCount: descriptor_count,
            stageFlags: stage_flags,
            pImmutableSamplers: match immut_samplers {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
        }
    }
}

impl VkDescriptorSetLayoutCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkDescriptorSetLayoutCreateFlags,
        bindings: &Vec<VkDescriptorSetLayoutBinding>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            bindingCount: bindings.len() as u32,
            pBindings: bindings.as_ptr(),
        }
    }
}

impl VkDescriptorSetAllocateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        desc_pool: VkDescriptorPool,
        desc_set_lyts: &Vec<VkDescriptorSetLayout>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            descriptorPool: desc_pool,
            descriptorSetCount: desc_set_lyts.len() as u32,
            pSetLayouts: desc_set_lyts.as_ptr(),
        }
    }
}

impl VkPipelineLayoutCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineLayoutCreateFlags,
        desc_set_lyts: &Vec<VkDescriptorSetLayout>,
        push_consts_rng: &Vec<VkPushConstantRange>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            setLayoutCount: desc_set_lyts.len() as u32,
            pSetLayouts: desc_set_lyts.as_ptr(),
            pushConstantRangeCount: push_consts_rng.len() as u32,
            pPushConstantRanges: push_consts_rng.as_ptr(),
        }
    }
}

impl VkPipelineShaderStageCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineCreateFlags,
        stage: VkShaderStageFlagBits,
        module: VkShaderModule,
        name: &CStr,
        specialization_info: Option<VkSpecializationInfo>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            stage,
            module,
            pName: name.as_ptr() as *const i8,
            pSpecializationInfo: match specialization_info {
                Some(x) => &x,
                None => std::ptr::null(),
            },
        }
    }
}

impl VkComputePipelineCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineCreateFlags,
        stage: &VkPipelineShaderStageCreateInfo,
        layout: VkPipelineLayout,
        base_pipe_hnd: Option<VkPipeline>,
        base_pipe_idx: Option<i32>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            stage: *stage,
            layout,
            basePipelineHandle: match base_pipe_hnd {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
            basePipelineIndex: match base_pipe_idx {
                Some(x) => x,
                None => 0,
            },
        }
    }
}

impl VkWriteDescriptorSet {
    pub fn new(
        p_next: Option<*const c_void>,
        desc_set: VkDescriptorSet,
        desc_binding: usize,
        desc_array_element: usize,
        desc_count: usize,
        desc_type: VkDescriptorType,
        image_infos: &Vec<VkDescriptorImageInfo>,
        buffer_infos: &Vec<VkDescriptorBufferInfo>,
        texel_buffer_view: Option<VkBufferView>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            dstSet: desc_set,
            dstBinding: desc_binding as u32,
            dstArrayElement: desc_array_element as u32,
            descriptorCount: desc_count as u32,
            descriptorType: desc_type,
            pImageInfo: image_infos.as_ptr(),
            pBufferInfo: buffer_infos.as_ptr(),
            pTexelBufferView: match texel_buffer_view {
                Some(x) => &x,
                None => std::ptr::null_mut(),
            },
        }
    }
}

impl VkCommandPoolCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkCommandPoolCreateFlags,
        q_fly_idx: usize,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            queueFamilyIndex: q_fly_idx as u32,
        }
    }
}

impl VkCommandBufferAllocateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        cmd_pool: VkCommandPool,
        level: VkCommandBufferLevel,
        cmd_buffer_count: usize,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            commandPool: cmd_pool,
            level,
            commandBufferCount: cmd_buffer_count as u32,
        }
    }
}

impl VkCommandBufferBeginInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkCommandBufferUsageFlags,
        inheritance_info: Option<VkCommandBufferInheritanceInfo>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            pInheritanceInfo: match inheritance_info {
                Some(x) => &x,
                None => std::ptr::null(),
            },
        }
    }
}

impl VkImageCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkImageCreateFlags,
        image_type: VkImageType,
        format: VkFormat,
        extent: VkExtent3D,
        mip_levels: u32,
        array_layers: u32,
        samples: VkSampleCountFlags,
        tiling: VkImageTiling,
        usage: VkImageUsageFlags,
        sharing_mode: VkSharingMode,
        q_fly_idx: &Vec<u32>,
        initial_layout: VkImageLayout,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            imageType: image_type,
            format,
            extent,
            mipLevels: mip_levels,
            arrayLayers: array_layers,
            samples: samples as i32,
            tiling,
            usage,
            sharingMode: sharing_mode,
            queueFamilyIndexCount: q_fly_idx.len() as u32,
            pQueueFamilyIndices: q_fly_idx.as_ptr(),
            initialLayout: initial_layout,
        }
    }
}

#[derive(Default)]
pub struct VkImageCreateInfoBuilder<'a> {
    p_next: Option<*const c_void>,
    flags: Option<VkImageCreateFlags>,
    image_type: Option<VkImageType>,
    format: Option<VkFormat>,
    extent: Option<VkExtent3D>,
    mip_levels: Option<u32>,
    array_layers: Option<u32>,
    samples: Option<VkSampleCountFlags>,
    tiling: Option<VkImageTiling>,
    usage: Option<VkImageUsageFlags>,
    sharing_mode: Option<VkSharingMode>,
    q_fly_idxs: Option<&'a Vec<u32>>,
    initial_layout: Option<VkImageLayout>,
}

impl<'a> VkImageCreateInfoBuilder<'a> {
    pub fn p_next(&mut self, p_next: *const c_void) -> &mut Self {
        self.p_next = Some(p_next);

        self
    }
    pub fn flags(&mut self, flags: VkImageCreateFlags) -> &mut Self {
        self.flags = Some(flags);

        self
    }

    pub fn image_type(&mut self, image_type: VkImageType) -> &mut Self {
        self.image_type = Some(image_type);

        self
    }

    pub fn format(&mut self, format: VkFormat) -> &mut Self {
        self.format = Some(format);

        self
    }

    pub fn extent(&mut self, extent: VkExtent3D) -> &mut Self {
        self.extent = Some(extent);

        self
    }

    pub fn mip_levels(&mut self, mip_levels: u32) -> &mut Self {
        self.mip_levels = Some(mip_levels);

        self
    }

    pub fn array_layers(&mut self, array_layers: u32) -> &mut Self {
        self.array_layers = Some(array_layers);

        self
    }

    pub fn samples(&mut self, samples: VkSampleCountFlags) -> &mut Self {
        self.samples = Some(samples);

        self
    }

    pub fn tiling(&mut self, tiling: VkImageTiling) -> &mut Self {
        self.tiling = Some(tiling);

        self
    }

    pub fn usage(&mut self, usage: VkImageUsageFlags) -> &mut Self {
        self.usage = Some(usage);

        self
    }

    pub fn sharing_mode(&mut self, sharing_mode: VkSharingMode) -> &mut Self {
        self.sharing_mode = Some(sharing_mode);

        self
    }

    pub fn q_family_indices(&mut self, q_fly_idxs: &'a Vec<u32>) -> &mut Self {
        self.q_fly_idxs = Some(q_fly_idxs);

        self
    }

    pub fn initial_layout(&mut self, initial_layout: VkImageLayout) -> &mut Self {
        self.initial_layout = Some(initial_layout);

        self
    }

    pub fn build(&self) -> VkImageCreateInfo {
        VkImageCreateInfo {
            sType: VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            pNext: match self.p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags: match self.flags {
                Some(x) => x,
                None => 0,
            },
            imageType: match self.image_type {
                Some(x) => x,
                None => 0,
            },
            format: match self.format {
                Some(x) => x,
                None => 0,
            },
            extent: match self.extent {
                Some(x) => x,
                None => VkExtent3D::default(),
            },
            mipLevels: match self.mip_levels {
                Some(x) => x,
                None => 0,
            },
            arrayLayers: match self.array_layers {
                Some(x) => x,
                None => 0,
            },
            samples: match self.samples {
                Some(x) => x as i32,
                None => 0,
            },
            tiling: match self.tiling {
                Some(x) => x,
                None => 0,
            },
            usage: match self.usage {
                Some(x) => x,
                None => 0,
            },
            sharingMode: match self.sharing_mode {
                Some(x) => x,
                None => 0,
            },
            queueFamilyIndexCount: match self.q_fly_idxs {
                Some(x) => x.len() as u32,
                None => 0,
            },
            pQueueFamilyIndices: match self.q_fly_idxs {
                Some(x) => x.as_ptr(),
                None => std::ptr::null(),
            },
            initialLayout: match self.initial_layout {
                Some(x) => x,
                None => 0,
            },
        }
    }
}

impl VkImageViewCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkImageViewCreateFlags,
        image: Option<VkImage>,
        view_type: VkImageViewType,
        format: VkFormat,
        components: VkComponentMapping,
        subresource_range: VkImageSubresourceRange,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            image: match image {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
            viewType: view_type,
            format,
            components,
            subresourceRange: subresource_range,
        }
    }
}

#[derive(Default)]
pub struct VkImageSubresourceRangeBuilder {
    aspect_mask: Option<VkImageAspectFlags>,
    base_mip_level: Option<u32>,
    level_count: Option<u32>,
    base_array_layer: Option<u32>,
    layer_count: Option<u32>,
}

impl VkImageSubresourceRangeBuilder {
    pub fn aspect_mask(&mut self, aspect_mask: VkImageAspectFlags) -> &mut Self {
        self.aspect_mask = Some(aspect_mask);

        self
    }

    pub fn base_mip_level(&mut self, base_mip_level: u32) -> &mut Self {
        self.base_mip_level = Some(base_mip_level);

        self
    }

    pub fn level_count(&mut self, level_count: u32) -> &mut Self {
        self.level_count = Some(level_count);

        self
    }

    pub fn base_array_layer(&mut self, base_array_layer: u32) -> &mut Self {
        self.base_array_layer = Some(base_array_layer);

        self
    }

    pub fn layer_count(&mut self, layer_count: u32) -> &mut Self {
        self.layer_count = Some(layer_count);

        self
    }

    pub fn build(&self) -> VkImageSubresourceRange {
        VkImageSubresourceRange {
            aspectMask: match self.aspect_mask {
                Some(x) => x,
                None => 0,
            },

            baseMipLevel: match self.base_mip_level {
                Some(x) => x,
                None => 0,
            },
            levelCount: match self.level_count {
                Some(x) => x,
                None => 0,
            },
            baseArrayLayer: match self.base_array_layer {
                Some(x) => x,
                None => 0,
            },
            layerCount: match self.layer_count {
                Some(x) => x,
                None => 0,
            },
        }
    }
}

#[derive(Default)]
pub struct VkImageViewCreateInfoBuilder {
    p_next: Option<*const c_void>,
    flags: Option<VkImageViewCreateFlags>,
    image: Option<VkImage>,
    view_type: Option<VkImageViewType>,
    format: Option<VkFormat>,
    components: Option<VkComponentMapping>,
    subresource_range: Option<VkImageSubresourceRange>,
}

impl VkImageViewCreateInfoBuilder {
    pub fn p_next(&mut self, p_next: *const c_void) -> &mut Self {
        self.p_next = Some(p_next);

        self
    }

    pub fn flags(&mut self, flags: VkImageCreateFlags) -> &mut Self {
        self.flags = Some(flags);

        self
    }

    pub fn image(&mut self, image: VkImage) -> &mut Self {
        self.image = Some(image);

        self
    }

    pub fn view_type(&mut self, view_type: VkImageViewType) -> &mut Self {
        self.view_type = Some(view_type);

        self
    }

    pub fn format(&mut self, format: VkFormat) -> &mut Self {
        self.format = Some(format);

        self
    }

    pub fn components(&mut self, components: VkComponentMapping) -> &mut Self {
        self.components = Some(components);

        self
    }

    pub fn subresource_range(&mut self, subresource_range: VkImageSubresourceRange) -> &mut Self {
        self.subresource_range = Some(subresource_range);

        self
    }

    pub fn build(&self) -> VkImageViewCreateInfo {
        VkImageViewCreateInfo {
            sType: VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            pNext: match self.p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags: match self.flags {
                Some(x) => x,
                None => 0,
            },
            image: match self.image {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
            viewType: match self.view_type {
                Some(x) => x,
                None => 0,
            },
            format: match self.format {
                Some(x) => x,
                None => 0,
            },
            components: match self.components {
                Some(x) => x,
                None => VkComponentMapping::default(),
            },
            subresourceRange: match self.subresource_range {
                Some(x) => x,
                None => VkImageSubresourceRange::default(),
            },
        }
    }
}

pub struct CommandBuffer {
    pub cmd_buffs: Vec<VkCommandBuffer>,
}

impl CommandBuffer {
    pub fn begin(
        &self,
        buff_idx: usize,
        begin_info: &VkCommandBufferBeginInfo,
    ) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;
        unsafe {
            result = vkBeginCommandBuffer(self.cmd_buffs[buff_idx], begin_info);
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn bind_pipeline(
        &self,
        buff_idx: usize,
        bind_point: VkPipelineBindPoint,
        pipeline: VkPipeline,
    ) {
        unsafe {
            vkCmdBindPipeline(self.cmd_buffs[buff_idx], bind_point, pipeline);
        }
    }

    pub fn bind_descriptor_sets(
        &self,
        buff_idx: usize,
        bind_point: VkPipelineBindPoint,
        layout: VkPipelineLayout,
        first_set: u32,
        desc_sets: &Vec<VkDescriptorSet>,
        dynamic_offsets: &Vec<u32>,
    ) {
        unsafe {
            vkCmdBindDescriptorSets(
                self.cmd_buffs[buff_idx],
                bind_point,
                layout,
                first_set,
                desc_sets.len() as u32,
                desc_sets.as_ptr(),
                dynamic_offsets.len() as u32,
                dynamic_offsets.as_ptr(),
            );
        }
    }

    pub fn pipeline_barrier(
        &self,
        buff_idx: usize,
        src_stg_msk: VkPipelineStageFlags,
        dst_stg_msk: VkPipelineStageFlags,
        dependency_flags: VkDependencyFlags,
        memory_barriers: &Vec<VkMemoryBarrier>,
        buffer_memory_barriers: &Vec<VkBufferMemoryBarrier>,
        image_memory_barriers: &Vec<VkImageMemoryBarrier>,
    ) {
        unsafe {
            vkCmdPipelineBarrier(
                self.cmd_buffs[buff_idx],
                src_stg_msk,
                dst_stg_msk,
                dependency_flags,
                memory_barriers.len() as u32,
                memory_barriers.as_ptr(),
                buffer_memory_barriers.len() as u32,
                buffer_memory_barriers.as_ptr(),
                image_memory_barriers.len() as u32,
                image_memory_barriers.as_ptr(),
            );
        }
    }

    pub fn dispatch(&self, buff_idx: usize, group_x: usize, group_y: usize, group_z: usize) {
        unsafe {
            vkCmdDispatch(
                self.cmd_buffs[buff_idx],
                group_x as u32,
                group_y as u32,
                group_z as u32,
            );
        }
    }

    pub fn end(&self, buff_idx: usize) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;
        unsafe {
            result = vkEndCommandBuffer(self.cmd_buffs[buff_idx]);
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
    }
}

impl VkSubmitInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        wait_sems: &Vec<VkSemaphore>,
        wait_stage_msk: Option<&u32>,
        cmd_buffs: &Vec<VkCommandBuffer>,
        signal_sems: &Vec<VkSemaphore>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_SUBMIT_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            waitSemaphoreCount: wait_sems.len() as u32,
            pWaitSemaphores: wait_sems.as_ptr(),
            pWaitDstStageMask: match wait_stage_msk {
                Some(x) => x,
                None => std::ptr::null(),
            },
            commandBufferCount: cmd_buffs.len() as u32,
            pCommandBuffers: cmd_buffs.as_ptr(),
            signalSemaphoreCount: signal_sems.len() as u32,
            pSignalSemaphores: signal_sems.as_ptr(),
        }
    }
}

pub struct Queue {
    pub q: VkQueue,
}

impl Queue {
    pub fn submit(
        &self,
        submit_infos: &Vec<VkSubmitInfo>,
        fence: Option<VkFence>,
    ) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkQueueSubmit(
                self.q,
                submit_infos.len() as u32,
                submit_infos.as_ptr(),
                match fence {
                    Some(x) => x,
                    None => std::ptr::null_mut(),
                },
            )
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn present(&self, present_info: &VkPresentInfoKHR) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkQueuePresentKHR(self.q, present_info);
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn wait_idle(&self) {
        unsafe {
            vkQueueWaitIdle(self.q);
        }
    }
}

impl VkVertexInputAttributeDescription {
    pub fn new(location: u32, binding: u32, format: VkFormat, offset: u32) -> Self {
        Self {
            location,
            binding,
            format,
            offset,
        }
    }
}

impl VkVertexInputBindingDescription {
    pub fn new(binding: u32, stride: u32, input_rate: VkVertexInputRate) -> Self {
        Self {
            binding,
            stride,
            inputRate: input_rate,
        }
    }
}

impl VkPipelineVertexInputStateCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineVertexInputStateCreateFlags,
        vertex_binding_descs: &Vec<VkVertexInputBindingDescription>,
        vertex_attr_descs: &Vec<VkVertexInputAttributeDescription>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            vertexBindingDescriptionCount: vertex_binding_descs.len() as u32,
            pVertexBindingDescriptions: vertex_binding_descs.as_ptr(),
            vertexAttributeDescriptionCount: vertex_attr_descs.len() as u32,
            pVertexAttributeDescriptions: vertex_attr_descs.as_ptr(),
        }
    }
}

impl VkPipelineInputAssemblyStateCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineInputAssemblyStateCreateFlags,
        topology: VkPrimitiveTopology,
        primitive_restart_enable: u32,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            topology,
            primitiveRestartEnable: primitive_restart_enable,
        }
    }
}

impl VkPipelineViewportStateCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineViewportStateCreateFlags,
        viewports: &Vec<VkViewport>,
        scissors: &Vec<VkRect2D>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            viewportCount: viewports.len() as u32,
            pViewports: viewports.as_ptr(),
            scissorCount: scissors.len() as u32,
            pScissors: scissors.as_ptr(),
        }
    }
}

impl VkPipelineRasterizationStateCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineRasterizationStateCreateFlags,
        depth_clamp_enable: u32,
        rasterization_discard_enable: u32,
        polygon_mode: VkPolygonMode,
        cull_mode: VkCullModeFlags,
        front_face: VkFrontFace,
        depth_bias_enable: u32,
        depth_bias_constant_factor: f32,
        depth_bias_clamp: f32,
        depth_bias_slope_factor: f32,
        line_width: f32,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            depthClampEnable: depth_clamp_enable,
            rasterizerDiscardEnable: rasterization_discard_enable,
            polygonMode: polygon_mode,
            cullMode: cull_mode,
            frontFace: front_face,
            depthBiasEnable: depth_bias_enable,
            depthBiasConstantFactor: depth_bias_constant_factor,
            depthBiasClamp: depth_bias_clamp,
            depthBiasSlopeFactor: depth_bias_slope_factor,
            lineWidth: line_width,
        }
    }
}

impl VkPipelineMultisampleStateCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineMultisampleStateCreateFlags,
        rasterization_samples: VkSampleCountFlagBits,
        sample_shading_enable: u32,
        min_sample_shading: f32,
        sample_mask: &VkSampleMask,
        alpha_coverage_enable: u32,
        alpha_to_one_enable: u32,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            rasterizationSamples: rasterization_samples,
            sampleShadingEnable: sample_shading_enable,
            minSampleShading: min_sample_shading,
            pSampleMask: sample_mask,
            alphaToCoverageEnable: alpha_coverage_enable,
            alphaToOneEnable: alpha_to_one_enable,
        }
    }
}

impl VkPipelineColorBlendStateCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineColorBlendStateCreateFlags,
        logic_op_enable: u32,
        logic_op: VkLogicOp,
        attachments: &Vec<VkPipelineColorBlendAttachmentState>,
        blend_constants: [f32; 4],
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            logicOpEnable: logic_op_enable,
            logicOp: logic_op,
            attachmentCount: attachments.len() as u32,
            pAttachments: attachments.as_ptr(),
            blendConstants: blend_constants,
        }
    }
}

impl VkPipelineDepthStencilStateCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineDepthStencilStateCreateFlags,
        depth_test_enable: u32,
        depth_write_enable: u32,
        depth_compare_op: VkCompareOp,
        depth_bounds_test_enable: u32,
        stencil_test_enable: u32,
        front: VkStencilOpState,
        back: VkStencilOpState,
        min_depth_bounds: f32,
        max_depth_bounds: f32,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            depthTestEnable: depth_test_enable,
            depthWriteEnable: depth_write_enable,
            depthCompareOp: depth_compare_op,
            depthBoundsTestEnable: depth_bounds_test_enable,
            stencilTestEnable: stencil_test_enable,
            front,
            back,
            minDepthBounds: min_depth_bounds,
            maxDepthBounds: max_depth_bounds,
        }
    }
}

impl VkPipelineRenderingCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        view_mask: u32,
        color_attachments: &Vec<VkFormat>,
        depth_attachment_format: VkFormat,
        stencil_attachment_format: VkFormat,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            viewMask: view_mask,
            colorAttachmentCount: color_attachments.len() as u32,
            pColorAttachmentFormats: color_attachments.as_ptr(),
            depthAttachmentFormat: depth_attachment_format,
            stencilAttachmentFormat: stencil_attachment_format,
        }
    }
}

impl VkGraphicsPipelineCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkPipelineCreateFlags,
        stages: &Vec<VkPipelineShaderStageCreateInfo>,
        vertex_input_state: &VkPipelineVertexInputStateCreateInfo,
        input_assembly_state: &VkPipelineInputAssemblyStateCreateInfo,
        tessellation_state: Option<&VkPipelineTessellationStateCreateInfo>,
        viewport_state: Option<&VkPipelineViewportStateCreateInfo>,
        rasterization_state: Option<&VkPipelineRasterizationStateCreateInfo>,
        multisample_state: Option<&VkPipelineMultisampleStateCreateInfo>,
        depth_stencil_state: Option<&VkPipelineDepthStencilStateCreateInfo>,
        color_blend_state: Option<&VkPipelineColorBlendStateCreateInfo>,
        dynamic_state: Option<&VkPipelineDynamicStateCreateInfo>,
        layout: VkPipelineLayout,
        render_pass: Option<VkRenderPass>,
        subpass: Option<u32>,
        base_pipeline_handle: Option<VkPipeline>,
        base_pipeline_index: Option<i32>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            stageCount: stages.len() as u32,
            pStages: stages.as_ptr(),
            pVertexInputState: vertex_input_state,
            pInputAssemblyState: input_assembly_state,
            pTessellationState: match tessellation_state {
                Some(x) => x,
                None => std::ptr::null(),
            },
            pViewportState: match viewport_state {
                Some(x) => x,
                None => std::ptr::null(),
            },
            pRasterizationState: match rasterization_state {
                Some(x) => x,
                None => std::ptr::null(),
            },
            pMultisampleState: match multisample_state {
                Some(x) => x,
                None => std::ptr::null(),
            },
            pDepthStencilState: match depth_stencil_state {
                Some(x) => x,
                None => std::ptr::null(),
            },
            pColorBlendState: match color_blend_state {
                Some(x) => x,
                None => std::ptr::null(),
            },
            pDynamicState: match dynamic_state {
                Some(x) => x,
                None => std::ptr::null(),
            },
            layout,
            renderPass: match render_pass {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
            subpass: match subpass {
                Some(x) => x,
                None => 0,
            },
            basePipelineHandle: match base_pipeline_handle {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
            basePipelineIndex: match base_pipeline_index {
                Some(x) => x,
                None => 0,
            },
        }
    }
}

impl VkFenceCreateInfo {
    pub fn new(p_next: Option<*const c_void>, flags: VkFenceCreateFlags) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
        }
    }
}

impl VkSemaphoreCreateInfo {
    pub fn new(p_next: Option<*const c_void>, flags: VkSemaphoreCreateFlags) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
        }
    }
}

impl VkImageMemoryBarrier {
    pub fn new(
        p_next: Option<*const c_void>,
        src_acc: VkAccessFlags,
        dst_acc: VkAccessFlags,
        old_lyt: VkImageLayout,
        new_lyt: VkImageLayout,
        src_q_fly_idx: u32,
        dst_q_fly_idx: u32,
        image: VkImage,
        subresource_range: VkImageSubresourceRange,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            srcAccessMask: src_acc,
            dstAccessMask: dst_acc,
            oldLayout: old_lyt,
            newLayout: new_lyt,
            srcQueueFamilyIndex: src_q_fly_idx,
            dstQueueFamilyIndex: dst_q_fly_idx,
            image,
            subresourceRange: subresource_range,
        }
    }
}

impl VkRenderingAttachmentInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        image_view: VkImageView,
        image_layout: VkImageLayout,
        resolve_mode: VkResolveModeFlagBits,
        resolve_image_view: Option<VkImageView>,
        resolve_image_layout: VkImageLayout,
        load_op: VkAttachmentLoadOp,
        store_op: VkAttachmentStoreOp,
        clear_value: VkClearValue,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            imageView: image_view,
            imageLayout: image_layout,
            resolveMode: resolve_mode,
            resolveImageView: match resolve_image_view {
                Some(x) => x,
                None => std::ptr::null_mut(),
            },
            resolveImageLayout: resolve_image_layout,
            loadOp: load_op,
            storeOp: store_op,
            clearValue: clear_value,
        }
    }
}

impl VkRenderingInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkRenderingFlags,
        render_area: VkRect2D,
        layer_count: usize,
        view_mask: u32,
        color_attachments: &Vec<VkRenderingAttachmentInfo>,
        depth_attachment: Option<&VkRenderingAttachmentInfo>,
        stencil_attachment: Option<&VkRenderingAttachmentInfo>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_RENDERING_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            flags,
            renderArea: render_area,
            layerCount: layer_count as u32,
            viewMask: view_mask,
            colorAttachmentCount: color_attachments.len() as u32,
            pColorAttachments: color_attachments.as_ptr(),
            pDepthAttachment: match depth_attachment {
                Some(x) => x,
                None => std::ptr::null(),
            },
            pStencilAttachment: match stencil_attachment {
                Some(x) => x,
                None => std::ptr::null(),
            },
        }
    }
}

impl VkPresentInfoKHR {
    pub fn new(
        p_next: Option<*const c_void>,
        wait_sems: &Vec<VkSemaphore>,
        swapchains: &Vec<VkSwapchainKHR>,
        image_indices: &Vec<u32>,
        results: Option<&mut Vec<VkResult>>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            waitSemaphoreCount: wait_sems.len() as u32,
            pWaitSemaphores: wait_sems.as_ptr(),
            swapchainCount: swapchains.len() as u32,
            pSwapchains: swapchains.as_ptr(),
            pImageIndices: image_indices.as_ptr(),
            pResults: match results {
                Some(x) => x.as_mut_ptr(),
                None => std::ptr::null_mut(),
            },
        }
    }
}
