use core::panic;
use std::{ffi::CStr, os::raw::c_void, u64};

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

impl Default for PhysicalDevice {
    fn default() -> Self {
        Self {
            phy_dev: std::ptr::null_mut(),
        }
    }
}

impl Default for VkPhysicalDeviceProperties {
    fn default() -> Self {
        Self {
            apiVersion: Default::default(),
            driverVersion: Default::default(),
            vendorID: Default::default(),
            deviceID: Default::default(),
            deviceType: Default::default(),
            deviceName: [0; 256],
            pipelineCacheUUID: Default::default(),
            limits: Default::default(),
            sparseProperties: Default::default(),
        }
    }
}

impl Default for VkPhysicalDeviceLimits {
    fn default() -> Self {
        Self {
            maxImageDimension1D: Default::default(),
            maxImageDimension2D: Default::default(),
            maxImageDimension3D: Default::default(),
            maxImageDimensionCube: Default::default(),
            maxImageArrayLayers: Default::default(),
            maxTexelBufferElements: Default::default(),
            maxUniformBufferRange: Default::default(),
            maxStorageBufferRange: Default::default(),
            maxPushConstantsSize: Default::default(),
            maxMemoryAllocationCount: Default::default(),
            maxSamplerAllocationCount: Default::default(),
            bufferImageGranularity: Default::default(),
            sparseAddressSpaceSize: Default::default(),
            maxBoundDescriptorSets: Default::default(),
            maxPerStageDescriptorSamplers: Default::default(),
            maxPerStageDescriptorUniformBuffers: Default::default(),
            maxPerStageDescriptorStorageBuffers: Default::default(),
            maxPerStageDescriptorSampledImages: Default::default(),
            maxPerStageDescriptorStorageImages: Default::default(),
            maxPerStageDescriptorInputAttachments: Default::default(),
            maxPerStageResources: Default::default(),
            maxDescriptorSetSamplers: Default::default(),
            maxDescriptorSetUniformBuffers: Default::default(),
            maxDescriptorSetUniformBuffersDynamic: Default::default(),
            maxDescriptorSetStorageBuffers: Default::default(),
            maxDescriptorSetStorageBuffersDynamic: Default::default(),
            maxDescriptorSetSampledImages: Default::default(),
            maxDescriptorSetStorageImages: Default::default(),
            maxDescriptorSetInputAttachments: Default::default(),
            maxVertexInputAttributes: Default::default(),
            maxVertexInputBindings: Default::default(),
            maxVertexInputAttributeOffset: Default::default(),
            maxVertexInputBindingStride: Default::default(),
            maxVertexOutputComponents: Default::default(),
            maxTessellationGenerationLevel: Default::default(),
            maxTessellationPatchSize: Default::default(),
            maxTessellationControlPerVertexInputComponents: Default::default(),
            maxTessellationControlPerVertexOutputComponents: Default::default(),
            maxTessellationControlPerPatchOutputComponents: Default::default(),
            maxTessellationControlTotalOutputComponents: Default::default(),
            maxTessellationEvaluationInputComponents: Default::default(),
            maxTessellationEvaluationOutputComponents: Default::default(),
            maxGeometryShaderInvocations: Default::default(),
            maxGeometryInputComponents: Default::default(),
            maxGeometryOutputComponents: Default::default(),
            maxGeometryOutputVertices: Default::default(),
            maxGeometryTotalOutputComponents: Default::default(),
            maxFragmentInputComponents: Default::default(),
            maxFragmentOutputAttachments: Default::default(),
            maxFragmentDualSrcAttachments: Default::default(),
            maxFragmentCombinedOutputResources: Default::default(),
            maxComputeSharedMemorySize: Default::default(),
            maxComputeWorkGroupCount: Default::default(),
            maxComputeWorkGroupInvocations: Default::default(),
            maxComputeWorkGroupSize: Default::default(),
            subPixelPrecisionBits: Default::default(),
            subTexelPrecisionBits: Default::default(),
            mipmapPrecisionBits: Default::default(),
            maxDrawIndexedIndexValue: Default::default(),
            maxDrawIndirectCount: Default::default(),
            maxSamplerLodBias: Default::default(),
            maxSamplerAnisotropy: Default::default(),
            maxViewports: Default::default(),
            maxViewportDimensions: Default::default(),
            viewportBoundsRange: Default::default(),
            viewportSubPixelBits: Default::default(),
            minMemoryMapAlignment: Default::default(),
            minTexelBufferOffsetAlignment: Default::default(),
            minUniformBufferOffsetAlignment: Default::default(),
            minStorageBufferOffsetAlignment: Default::default(),
            minTexelOffset: Default::default(),
            maxTexelOffset: Default::default(),
            minTexelGatherOffset: Default::default(),
            maxTexelGatherOffset: Default::default(),
            minInterpolationOffset: Default::default(),
            maxInterpolationOffset: Default::default(),
            subPixelInterpolationOffsetBits: Default::default(),
            maxFramebufferWidth: Default::default(),
            maxFramebufferHeight: Default::default(),
            maxFramebufferLayers: Default::default(),
            framebufferColorSampleCounts: Default::default(),
            framebufferDepthSampleCounts: Default::default(),
            framebufferStencilSampleCounts: Default::default(),
            framebufferNoAttachmentsSampleCounts: Default::default(),
            maxColorAttachments: Default::default(),
            sampledImageColorSampleCounts: Default::default(),
            sampledImageIntegerSampleCounts: Default::default(),
            sampledImageDepthSampleCounts: Default::default(),
            sampledImageStencilSampleCounts: Default::default(),
            storageImageSampleCounts: Default::default(),
            maxSampleMaskWords: Default::default(),
            timestampComputeAndGraphics: Default::default(),
            timestampPeriod: Default::default(),
            maxClipDistances: Default::default(),
            maxCullDistances: Default::default(),
            maxCombinedClipAndCullDistances: Default::default(),
            discreteQueuePriorities: Default::default(),
            pointSizeRange: Default::default(),
            lineWidthRange: Default::default(),
            pointSizeGranularity: Default::default(),
            lineWidthGranularity: Default::default(),
            strictLines: Default::default(),
            standardSampleLocations: Default::default(),
            optimalBufferCopyOffsetAlignment: Default::default(),
            optimalBufferCopyRowPitchAlignment: Default::default(),
            nonCoherentAtomSize: Default::default(),
        }
    }
}

impl Default for VkPhysicalDeviceSparseProperties {
    fn default() -> Self {
        Self {
            residencyStandard2DBlockShape: Default::default(),
            residencyStandard2DMultisampleBlockShape: Default::default(),
            residencyStandard3DBlockShape: Default::default(),
            residencyAlignedMipSize: Default::default(),
            residencyNonResidentStrict: Default::default(),
        }
    }
}

impl Default for VkQueueFamilyProperties {
    fn default() -> Self {
        Self {
            queueFlags: Default::default(),
            queueCount: Default::default(),
            timestampValidBits: Default::default(),
            minImageTransferGranularity: VkExtent3D {
                width: 0,
                height: 0,
                depth: 0,
            },
        }
    }
}

impl Default for VkPhysicalDeviceFeatures {
    fn default() -> Self {
        Self {
            robustBufferAccess: Default::default(),
            fullDrawIndexUint32: Default::default(),
            imageCubeArray: Default::default(),
            independentBlend: Default::default(),
            geometryShader: Default::default(),
            tessellationShader: Default::default(),
            sampleRateShading: Default::default(),
            dualSrcBlend: Default::default(),
            logicOp: Default::default(),
            multiDrawIndirect: Default::default(),
            drawIndirectFirstInstance: Default::default(),
            depthClamp: Default::default(),
            depthBiasClamp: Default::default(),
            fillModeNonSolid: Default::default(),
            depthBounds: Default::default(),
            wideLines: Default::default(),
            largePoints: Default::default(),
            alphaToOne: Default::default(),
            multiViewport: Default::default(),
            samplerAnisotropy: Default::default(),
            textureCompressionETC2: Default::default(),
            textureCompressionASTC_LDR: Default::default(),
            textureCompressionBC: Default::default(),
            occlusionQueryPrecise: Default::default(),
            pipelineStatisticsQuery: Default::default(),
            vertexPipelineStoresAndAtomics: Default::default(),
            fragmentStoresAndAtomics: Default::default(),
            shaderTessellationAndGeometryPointSize: Default::default(),
            shaderImageGatherExtended: Default::default(),
            shaderStorageImageExtendedFormats: Default::default(),
            shaderStorageImageMultisample: Default::default(),
            shaderStorageImageReadWithoutFormat: Default::default(),
            shaderStorageImageWriteWithoutFormat: Default::default(),
            shaderUniformBufferArrayDynamicIndexing: Default::default(),
            shaderSampledImageArrayDynamicIndexing: Default::default(),
            shaderStorageBufferArrayDynamicIndexing: Default::default(),
            shaderStorageImageArrayDynamicIndexing: Default::default(),
            shaderClipDistance: Default::default(),
            shaderCullDistance: Default::default(),
            shaderFloat64: Default::default(),
            shaderInt64: Default::default(),
            shaderInt16: Default::default(),
            shaderResourceResidency: Default::default(),
            shaderResourceMinLod: Default::default(),
            sparseBinding: Default::default(),
            sparseResidencyBuffer: Default::default(),
            sparseResidencyImage2D: Default::default(),
            sparseResidencyImage3D: Default::default(),
            sparseResidency2Samples: Default::default(),
            sparseResidency4Samples: Default::default(),
            sparseResidency8Samples: Default::default(),
            sparseResidency16Samples: Default::default(),
            sparseResidencyAliased: Default::default(),
            variableMultisampleRate: Default::default(),
            inheritedQueries: Default::default(),
        }
    }
}

impl Default for VkPhysicalDeviceVulkan12Features {
    fn default() -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            pNext: std::ptr::null_mut(),
            samplerMirrorClampToEdge: Default::default(),
            drawIndirectCount: Default::default(),
            storageBuffer8BitAccess: Default::default(),
            uniformAndStorageBuffer8BitAccess: Default::default(),
            storagePushConstant8: Default::default(),
            shaderBufferInt64Atomics: Default::default(),
            shaderSharedInt64Atomics: Default::default(),
            shaderFloat16: Default::default(),
            shaderInt8: Default::default(),
            descriptorIndexing: Default::default(),
            shaderInputAttachmentArrayDynamicIndexing: Default::default(),
            shaderUniformTexelBufferArrayDynamicIndexing: Default::default(),
            shaderStorageTexelBufferArrayDynamicIndexing: Default::default(),
            shaderUniformBufferArrayNonUniformIndexing: Default::default(),
            shaderSampledImageArrayNonUniformIndexing: Default::default(),
            shaderStorageBufferArrayNonUniformIndexing: Default::default(),
            shaderStorageImageArrayNonUniformIndexing: Default::default(),
            shaderInputAttachmentArrayNonUniformIndexing: Default::default(),
            shaderUniformTexelBufferArrayNonUniformIndexing: Default::default(),
            shaderStorageTexelBufferArrayNonUniformIndexing: Default::default(),
            descriptorBindingUniformBufferUpdateAfterBind: Default::default(),
            descriptorBindingSampledImageUpdateAfterBind: Default::default(),
            descriptorBindingStorageImageUpdateAfterBind: Default::default(),
            descriptorBindingStorageBufferUpdateAfterBind: Default::default(),
            descriptorBindingUniformTexelBufferUpdateAfterBind: Default::default(),
            descriptorBindingStorageTexelBufferUpdateAfterBind: Default::default(),
            descriptorBindingUpdateUnusedWhilePending: Default::default(),
            descriptorBindingPartiallyBound: Default::default(),
            descriptorBindingVariableDescriptorCount: Default::default(),
            runtimeDescriptorArray: Default::default(),
            samplerFilterMinmax: Default::default(),
            scalarBlockLayout: Default::default(),
            imagelessFramebuffer: Default::default(),
            uniformBufferStandardLayout: Default::default(),
            shaderSubgroupExtendedTypes: Default::default(),
            separateDepthStencilLayouts: Default::default(),
            hostQueryReset: Default::default(),
            timelineSemaphore: Default::default(),
            bufferDeviceAddress: Default::default(),
            bufferDeviceAddressCaptureReplay: Default::default(),
            bufferDeviceAddressMultiDevice: Default::default(),
            vulkanMemoryModel: Default::default(),
            vulkanMemoryModelDeviceScope: Default::default(),
            vulkanMemoryModelAvailabilityVisibilityChains: Default::default(),
            shaderOutputViewportIndex: Default::default(),
            shaderOutputLayer: Default::default(),
            subgroupBroadcastDynamicId: Default::default(),
        }
    }
}

impl Default for VkPhysicalDeviceFeatures2 {
    fn default() -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            pNext: std::ptr::null_mut(),
            features: Default::default(),
        }
    }
}

impl Default for VkMemoryType {
    fn default() -> Self {
        Self {
            propertyFlags: Default::default(),
            heapIndex: Default::default(),
        }
    }
}

impl Default for VkMemoryHeap {
    fn default() -> Self {
        Self {
            size: Default::default(),
            flags: Default::default(),
        }
    }
}

impl Default for VkPhysicalDeviceMemoryProperties {
    fn default() -> Self {
        Self {
            memoryTypeCount: Default::default(),
            memoryTypes: [VkMemoryType::default(); 32 as usize],
            memoryHeapCount: Default::default(),
            memoryHeaps: [VkMemoryHeap::default(); 16 as usize],
        }
    }
}

impl Default for VkMemoryRequirements {
    fn default() -> Self {
        Self {
            size: Default::default(),
            alignment: Default::default(),
            memoryTypeBits: Default::default(),
        }
    }
}

impl Default for VkFenceCreateInfo {
    fn default() -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            pNext: std::ptr::null(),
            flags: Default::default(),
        }
    }
}

impl Default for VkSurfaceCapabilitiesKHR {
    fn default() -> Self {
        Self {
            minImageCount: Default::default(),
            maxImageCount: Default::default(),
            currentExtent: VkExtent2D {
                width: 0,
                height: 0,
            },
            minImageExtent: VkExtent2D {
                width: 0,
                height: 0,
            },
            maxImageExtent: VkExtent2D {
                width: 0,
                height: 0,
            },
            maxImageArrayLayers: Default::default(),
            supportedTransforms: Default::default(),
            currentTransform: Default::default(),
            supportedCompositeAlpha: Default::default(),
            supportedUsageFlags: Default::default(),
        }
    }
}

impl Default for VkSurfaceFormatKHR {
    fn default() -> Self {
        Self {
            format: Default::default(),
            colorSpace: Default::default(),
        }
    }
}

impl Default for VkComponentMapping {
    fn default() -> Self {
        Self {
            r: VK_COMPONENT_SWIZZLE_R,
            g: VK_COMPONENT_SWIZZLE_G,
            b: VK_COMPONENT_SWIZZLE_B,
            a: VK_COMPONENT_SWIZZLE_A,
        }
    }
}

impl Default for VkStencilOpState {
    fn default() -> Self {
        Self {
            failOp: Default::default(),
            passOp: Default::default(),
            depthFailOp: Default::default(),
            compareOp: Default::default(),
            compareMask: Default::default(),
            writeMask: Default::default(),
            reference: Default::default(),
        }
    }
}

impl Default for VkPipelineTessellationStateCreateInfo {
    fn default() -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
            pNext: std::ptr::null(),
            flags: Default::default(),
            patchControlPoints: Default::default(),
        }
    }
}

impl Default for VkPipelineDynamicStateCreateInfo {
    fn default() -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            pNext: std::ptr::null(),
            flags: Default::default(),
            dynamicStateCount: Default::default(),
            pDynamicStates: std::ptr::null(),
        }
    }
}

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
        create_info: &VkWin32SurfaceCreateInfoKHR,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkSurfaceKHR, VkResult> {
        let mut surface = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateWin32SurfaceKHR(
                self.instance,
                create_info,
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

    pub fn features(&self) -> VkPhysicalDeviceFeatures {
        let mut features = VkPhysicalDeviceFeatures::default();

        unsafe {
            vkGetPhysicalDeviceFeatures(self.phy_dev, &mut features);
        }

        features
    }

    pub fn features2(&self) -> VkPhysicalDeviceFeatures2 {
        let mut features2 = VkPhysicalDeviceFeatures2::default();

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
        pre_transform: VkSurfaceTransformFlagBitsKHR,
        composite_alpha: VkCompositeAlphaFlagBitsKHR,
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
            preTransform: pre_transform,
            compositeAlpha: composite_alpha,
            presentMode: present_mode,
            clipped,
            oldSwapchain: match old_swapchain {
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
        create_info: &VkDeviceCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut this = Self {
            device: std::ptr::null_mut(),
        };

        let mut result = VK_SUCCESS;
        unsafe {
            result = vkCreateDevice(
                phy_dev.phy_dev,
                create_info,
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
        create_info: &VkSwapchainCreateInfoKHR,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkSwapchainKHR, VkResult> {
        let mut swapchain = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateSwapchainKHR(
                self.device,
                create_info,
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

    pub fn swapchain_images_khr(
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
        create_info: &VkImageCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkImage, VkResult> {
        let mut image = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateImage(
                self.device,
                create_info,
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
        create_info: &VkImageViewCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkImageView, VkResult> {
        let mut image_view = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateImageView(
                self.device,
                create_info,
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
        create_info: &VkShaderModuleCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<VkShaderModule, VkResult> {
        let mut shader_mod = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateShaderModule(
                self.device,
                create_info,
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

impl VkImageViewCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkImageViewCreateFlags,
        image: VkImage,
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
            image,
            viewType: view_type,
            format,
            components,
            subresourceRange: subresource_range,
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
    pub instance: Instance,
    pub phy_dev: PhysicalDevice,
    pub surface: VkSurfaceKHR,
    pub surf_extent: VkExtent2D,
    pub device: Device,
    pub swapchain: VkSwapchainKHR,
    pub sc_images: Vec<VkImage>,
    pub sc_image_views: Vec<VkImageView>,
    pub sc_depth_imgs: Vec<VkImage>,
    pub sc_depth_img_mems: Vec<VkDeviceMemory>,
    pub sc_depth_img_vs: Vec<VkImageView>,
    pub vert_sh_mod: VkShaderModule,
    pub frag_sh_mod: VkShaderModule,
    pub desc_set_lyts: Vec<VkDescriptorSetLayout>,
    pub desc_pool: VkDescriptorPool,
    pub pipe_lyt: VkPipelineLayout,
    pub view_pipes: Vec<VkPipeline>,
    pub sc_cmd_pool: VkCommandPool,
    pub sc_cmd_buff: CommandBuffer,
    pub lyt_chng_cmd_buff: CommandBuffer,
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
            instance: Instance {
                instance: std::ptr::null_mut(),
            },
            phy_dev: PhysicalDevice {
                phy_dev: std::ptr::null_mut(),
            },
            surface: std::ptr::null_mut(),
            surf_extent: VkExtent2D {
                width: 0,
                height: 0,
            },
            device: Device {
                device: std::ptr::null_mut(),
            },
            swapchain: std::ptr::null_mut(),
            sc_images: vec![],
            sc_image_views: vec![],
            sc_depth_imgs: vec![],
            sc_depth_img_mems: vec![],
            sc_depth_img_vs: vec![],
            vert_sh_mod: std::ptr::null_mut(),
            frag_sh_mod: std::ptr::null_mut(),
            desc_set_lyts: vec![],
            desc_pool: std::ptr::null_mut(),
            pipe_lyt: std::ptr::null_mut(),
            view_pipes: vec![],
            sc_cmd_pool: std::ptr::null_mut(),
            sc_cmd_buff: CommandBuffer { cmd_buffs: vec![] },
            lyt_chng_cmd_buff: CommandBuffer { cmd_buffs: vec![] },
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
        let mut phy_dev = PhysicalDevice::default();
        let mut q_fly_idx = 0u32;
        let mut q_fly_q_cnt = 0;
        let mut min_sto_buff_align = 0;
        let mut min_uni_buff_align = 0;

        let ai = VkApplicationInfo::new(
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

        let instance = match Instance::create(inst_ci, None) {
            Ok(instance) => instance,
            Err(result) => {
                panic!("ERR Create instance {}", result);
            }
        };

        let surface_ci = VkWin32SurfaceCreateInfoKHR::new(
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
        );

        let surface = match instance.create_win32_surface_khr(&surface_ci, None) {
            Ok(surface) => surface,
            Err(result) => {
                panic!("ERR create surface {}", result);
            }
        };

        let phy_devs = instance.get_physical_devices();

        for pd in phy_devs {
            let phy_dev_props = pd.properties();

            if phy_dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                || phy_dev_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
            {
                let q_fly_props = pd.queue_family_properties();

                let mut is_supported: u32 = 1;
                unsafe {
                    vkGetPhysicalDeviceSurfaceSupportKHR(
                        pd.phy_dev,
                        q_fly_q_cnt,
                        surface,
                        &mut is_supported,
                    );
                }

                for (q, q_fly_prop) in q_fly_props.iter().enumerate() {
                    if (q_fly_prop.queueFlags & VK_QUEUE_GRAPHICS_BIT as u32)
                        == VK_QUEUE_GRAPHICS_BIT as u32
                        && (is_supported == 1)
                        && (unsafe {
                            vkGetPhysicalDeviceWin32PresentationSupportKHR(pd.phy_dev, q as u32)
                        } == 1)
                    {
                        phy_dev = pd;
                        q_fly_idx = q as u32;
                        q_fly_q_cnt = q_fly_prop.queueCount;
                        min_sto_buff_align = phy_dev_props.limits.minStorageBufferOffsetAlignment;
                        min_uni_buff_align = phy_dev_props.limits.minUniformBufferOffsetAlignment;
                        break;
                    }
                }
            }
        }

        let surf_caps = match phy_dev.surface_capabilities_khr(&surface) {
            Ok(caps) => caps,
            Err(result) => {
                panic!("ERR get surface capabilities {}", result);
            }
        };

        let surf_forms = match phy_dev.surface_formats_khr(&surface) {
            Ok(forms) => forms,
            Err(result) => {
                panic!("ERR get surface formats {}", result);
            }
        };

        let mut surf_format = VkSurfaceFormatKHR::default();

        for sf in &surf_forms {
            if sf.format == VK_FORMAT_R8G8B8A8_UNORM {
                surf_format = *sf;
            }
        }

        let present_modes = match phy_dev.surface_present_modes_khr(&surface) {
            Ok(present_modes) => present_modes,
            Err(result) => {
                panic!("ERR get surface present modes {}", result);
            }
        };

        let mut present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for pm in present_modes {
            if pm == VK_PRESENT_MODE_MAILBOX_KHR {
                present_mode = pm;
            }
        }

        let priorities: Vec<f32> = vec![1.0; q_fly_q_cnt as usize];
        let q_ci = VkDeviceQueueCreateInfo::new(None, 0, q_fly_idx, q_fly_q_cnt, &priorities);
        let dev_exts = vec![VK_KHR_SWAPCHAIN_EXTENSION_NAME];
        let feats2 = phy_dev.features2();
        let dyn_rend_feats = VkPhysicalDeviceDynamicRenderingFeatures::new(Some(
            std::ptr::addr_of!(feats2) as *mut c_void,
        ));
        let q_cis = vec![q_ci];
        let device_ci = VkDeviceCreateInfo::new(
            Some(std::ptr::addr_of!(dyn_rend_feats) as *const c_void),
            0,
            &q_cis,
            &dev_exts,
            None,
        );

        let device = match Device::create(&phy_dev, &device_ci, None) {
            Ok(device) => device,
            Err(result) => {
                panic!("ERR Create device {}", result);
            }
        };

        let q_fly_idxs = vec![q_fly_idx];
        let sc_ci = VkSwapchainCreateInfoKHR::new(
            None,
            0,
            surface,
            surf_caps.minImageCount + 1,
            surf_format.format,
            surf_format.colorSpace,
            surf_caps.currentExtent,
            1,
            surf_caps.supportedUsageFlags,
            VK_SHARING_MODE_EXCLUSIVE,
            &q_fly_idxs,
            surf_caps.currentTransform,
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            present_mode,
            1,
            None,
        );

        let swapchain = match device.create_swapchain_khr(&sc_ci, None) {
            Ok(sc) => sc,
            Err(result) => {
                panic!("ERR create swapchain {}", result);
            }
        };

        let sc_images = match device.swapchain_images_khr(swapchain) {
            Ok(imgs) => imgs,
            Err(result) => {
                panic!("ERR get swapchain images {}", result);
            }
        };

        let mut sc_image_views = vec![];

        let mut iv_ci = VkImageViewCreateInfo::new(
            None,
            0,
            std::ptr::null_mut(),
            VK_IMAGE_VIEW_TYPE_2D,
            surf_format.format,
            VkComponentMapping::default(),
            VkImageSubresourceRange {
                aspectMask: VK_IMAGE_ASPECT_COLOR_BIT as u32,
                baseMipLevel: 0,
                levelCount: 1,
                baseArrayLayer: 0,
                layerCount: 1,
            },
        );

        for sw_img in &sc_images {
            iv_ci.image = *sw_img;
            sc_image_views.push(match device.create_image_view(&iv_ci, None) {
                Ok(iv) => iv,
                Err(result) => {
                    panic!("ERR create image view {}", result);
                }
            });
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
            create_imgs_and_memory(&phy_dev, &device, &sc_d_img_infos, &q_fly_idxs);

        let vfd = match std::fs::read("shaders/vert.spv") {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR read vert shader {}", err);
            }
        };

        let ffd = match std::fs::read("shaders/frag.spv") {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR read frag shader {}", err);
            }
        };

        let vert_sh_mod_ci = VkShaderModuleCreateInfo::new(None, 0, &vfd);
        let frag_sh_mod_ci = VkShaderModuleCreateInfo::new(None, 0, &ffd);

        let vert_sh_mod = match device.create_shader_module(&vert_sh_mod_ci, None) {
            Ok(x) => x,
            Err(result) => {
                panic!("ERR create shader module {}", result);
            }
        };

        let frag_sh_mod = match device.create_shader_module(&frag_sh_mod_ci, None) {
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
                vert_sh_mod,
                c"main",
                None,
            ),
            VkPipelineShaderStageCreateInfo::new(
                None,
                0,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                frag_sh_mod,
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

        let sc_cmd_pool = match device.create_command_pool(&sc_cmd_pool_ci, None) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR create command pool {}", err);
            }
        };

        let sc_cmd_buff_ai = VkCommandBufferAllocateInfo::new(
            None,
            sc_cmd_pool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            sc_ci.minImageCount as usize,
        );

        let lyt_chng_cmd_buff_ai =
            VkCommandBufferAllocateInfo::new(None, sc_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

        let lyt_chng_cmd_buff = match device.allocate_command_buffers(&lyt_chng_cmd_buff_ai) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR allocate commmand buffers {}", err);
            }
        };

        let sc_cmd_buff = match device.allocate_command_buffers(&sc_cmd_buff_ai) {
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
            &q_fly_idxs,
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

    pub fn render(&self) {
        let img_idx = match self.device.acquire_next_image_khr(
            self.swapchain,
            u64::MAX,
            Some(self.acq_sem),
            None,
        ) {
            Ok(x) => x,
            Err(err) => {
                panic!("ERR acquire next image khr {}", err);
            }
        };

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
            self.sc_image_views[img_idx],
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
            self.sc_depth_img_vs[img_idx],
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
            &color_attachments,
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
        let swapchains = vec![self.swapchain];
        let img_idxs = vec![img_idx as u32];
        let pi = VkPresentInfoKHR::new(None, &wait_sems, &swapchains, &img_idxs, None);

        match self.gfx_q.present(&pi) {
            Ok(_) => (),
            Err(err) => {
                panic!("ERR queue present {}", err);
            }
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
        self.device.destroy_command_pool(self.sc_cmd_pool, None);
        self.device.destroy_pipeline_layout(self.pipe_lyt, None);
        self.device.destroy_pipelines(&self.view_pipes, None);

        self.device.destroy_descriptor_pool(self.desc_pool, None);

        for desc_set_lyt in &self.desc_set_lyts {
            self.device
                .destroy_descriptor_set_layout(*desc_set_lyt, None);
        }

        for sc_depth_img in &self.sc_depth_imgs {
            self.device.destroy_image(*sc_depth_img, None);
        }

        for sc_depth_iv in &self.sc_depth_img_vs {
            self.device.destroy_image_view(*sc_depth_iv, None);
        }

        for sc_depth_img_mem in &self.sc_depth_img_mems {
            self.device.free_memory(*sc_depth_img_mem, None);
        }

        for sc_image_view in &self.sc_image_views {
            self.device.destroy_image_view(*sc_image_view, None);
        }

        self.device.destroy_shader_module(self.vert_sh_mod, None);
        self.device.destroy_shader_module(self.frag_sh_mod, None);

        self.device.destroy_swapchain_khr(self.swapchain, None);
        self.device.destroy(None);
        self.instance.destroy_surface_khr(self.surface, None);
        self.instance.destroy(None);
    }
}

pub fn get_memory_id(
    mem_props: &VkPhysicalDeviceMemoryProperties,
    mem_reqs: &VkMemoryRequirements,
    req_types: VkMemoryPropertyFlags,
) -> Result<u32, i32> {
    let mut idx = -1;

    for (mt, mem_type) in mem_props.memoryTypes.into_iter().enumerate() {
        if mem_reqs.memoryTypeBits & (1 << mt) == (1 << mt) {
            if mem_type.propertyFlags == req_types {
                if mem_props.memoryHeaps[mem_props.memoryTypes[mt as usize].heapIndex as usize].size
                    > mem_reqs.size
                {
                    idx = mt as i32;
                    break;
                }
            }
        }
    }

    if idx >= 0 {
        Ok(idx as u32)
    } else {
        Err(idx)
    }
}

pub fn create_imgs_and_memory(
    phy_dev: &PhysicalDevice,
    device: &Device,
    img_infos: &Vec<ImageInfo>,
    q_fly_idx: &Vec<u32>,
) -> (Vec<VkImage>, Vec<VkImageView>, Vec<VkDeviceMemory>) {
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

        let img = match device.create_image(&img_ci, None) {
            Ok(img) => img,
            Err(result) => {
                panic!("ERR create image {}", result);
            }
        };

        let mem_props = phy_dev.memory_properties();
        let mem_reqs = device.image_memory_requirements(img);
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

                mem_ais_imgs[i].push((img, mem_reqs.size));

                break;
            }
        }

        if !mem_ai_found {
            mem_ais.push(VkMemoryAllocateInfo::new(None, mem_reqs.size, mem_id));
            mem_ais_imgs.push(vec![(img, mem_reqs.size)]);
        }

        imgs.push(img);
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
        let img_v_ci = VkImageViewCreateInfo::new(
            None,
            0,
            imgs[i],
            img_info.view_type,
            img_info.format,
            VkComponentMapping::default(),
            VkImageSubresourceRange {
                aspectMask: img_info.aspect,
                baseMipLevel: 0,
                levelCount: 1,
                baseArrayLayer: 0,
                layerCount: 1,
            },
        );

        let img_view = match device.create_image_view(&img_v_ci, None) {
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
