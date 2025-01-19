#![allow(non_snake_case)]
#![allow(non_camel_case_types)]
#![allow(non_upper_case_globals)]
#![allow(dead_code)]
#![allow(unused_variables)]
#![allow(unused_assignments)]

use std::{ffi::CStr, os::raw::c_void};

include!(concat!(env!("OUT_DIR"), "/vk_wrapper.rs"));

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
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for Instance {
    fn default() -> Self {
        Self {
            instance: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for Instance {
    fn drop(&mut self) {
        if self.instance != std::ptr::null_mut() {
            println!("Dropping instance");
            unsafe {
                vkDestroyInstance(
                    self.instance,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl Instance {
    pub fn create(
        create_info: &VkInstanceCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut instance = std::ptr::null_mut();

        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateInstance(
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut instance,
            );
        };

        if result >= VK_SUCCESS {
            Ok(Self {
                instance,
                allocator,
            })
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

pub struct Surface {
    pub surface: VkSurfaceKHR,
    instance: VkInstance,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for Surface {
    fn default() -> Self {
        Self {
            surface: std::ptr::null_mut(),
            instance: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for Surface {
    fn drop(&mut self) {
        if self.surface != std::ptr::null_mut() {
            println!("Dropping surface");
            unsafe {
                vkDestroySurfaceKHR(
                    self.instance,
                    self.surface,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl Surface {
    pub fn create(
        instance: VkInstance,
        create_info: &VkWin32SurfaceCreateInfoKHR,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut surface = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateWin32SurfaceKHR(
                instance,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut surface,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                surface,
                instance,
                allocator,
            })
        } else {
            Err(result)
        }
    }
}

#[derive(Copy, Clone)]
pub struct PhysicalDevice {
    pub phy_dev: VkPhysicalDevice,
}

impl Default for PhysicalDevice {
    fn default() -> Self {
        Self {
            phy_dev: std::ptr::null_mut(),
        }
    }
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

impl VkDeviceQueueCreateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        flags: VkDeviceQueueCreateFlags,
        q_fly_idx: u32,
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
            queueCount: priorities.len() as u32,
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
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for Device {
    fn default() -> Self {
        Self {
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for Device {
    fn drop(&mut self) {
        if self.device != std::ptr::null_mut() {
            println!("Dropping device");
            unsafe {
                vkDestroyDevice(
                    self.device,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl Device {
    pub fn create(
        phy_dev: &PhysicalDevice,
        create_info: &VkDeviceCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut device = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateDevice(
                phy_dev.phy_dev,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut device,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self { device, allocator })
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
    ) -> Result<CommandBuffers, VkResult> {
        let mut result = VK_SUCCESS;
        let mut vk_cmd_buffs = vec![std::ptr::null_mut(); alloc_info.commandBufferCount as usize];

        unsafe {
            result = vkAllocateCommandBuffers(self.device, alloc_info, vk_cmd_buffs.as_mut_ptr());
        }

        if result >= VK_SUCCESS {
            let mut cmd_buff = CommandBuffers {
                device: self.device,
                cmd_buffs: Vec::with_capacity(alloc_info.commandBufferCount as usize),
                command_pool: alloc_info.commandPool,
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

    pub fn wait_idle(&self) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkDeviceWaitIdle(self.device);
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
    }

    pub fn free_command_buffers(&self, cmd_pool: VkCommandPool, cmd_buff: CommandBuffers) {
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

pub struct Swapchain {
    pub swapchain: VkSwapchainKHR,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for Swapchain {
    fn default() -> Self {
        Self {
            swapchain: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for Swapchain {
    fn drop(&mut self) {
        if self.swapchain != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping swapchain");
            unsafe {
                vkDestroySwapchainKHR(
                    self.device,
                    self.swapchain,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl Swapchain {
    pub fn create(
        device: VkDevice,
        create_info: &VkSwapchainCreateInfoKHR,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut swapchain = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateSwapchainKHR(
                device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut swapchain,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                swapchain,
                device,
                allocator,
            })
        } else {
            Err(result)
        }
    }

    pub fn get_images(&self) -> Result<Vec<VkImage>, VkResult> {
        let mut images = vec![];
        let mut result = VK_SUCCESS;

        unsafe {
            let mut image_count = 0u32;

            result = vkGetSwapchainImagesKHR(
                self.device,
                self.swapchain,
                &mut image_count,
                std::ptr::null_mut(),
            );

            images.resize(image_count as usize, std::ptr::null_mut());

            result = vkGetSwapchainImagesKHR(
                self.device,
                self.swapchain,
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

pub struct Buffer {
    pub buffer: VkBuffer,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for Buffer {
    fn default() -> Self {
        Self {
            buffer: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for Buffer {
    fn drop(&mut self) {
        if self.buffer != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping buffer");

            unsafe {
                vkDestroyBuffer(
                    self.device,
                    self.buffer,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null_mut(),
                    },
                );
            }
        }
    }
}

impl Buffer {
    pub fn create(
        device: VkDevice,
        create_info: &VkBufferCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut buffer = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateBuffer(
                device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut buffer,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                device,
                buffer,
                allocator,
            })
        } else {
            Err(result)
        }
    }

    pub fn memory_requirements(&self) -> VkMemoryRequirements {
        let mut mem_reqs = VkMemoryRequirements::default();

        unsafe {
            vkGetBufferMemoryRequirements(self.device, self.buffer, &mut mem_reqs);
        }

        mem_reqs
    }

    pub fn bind_memory(
        &self,
        memory: VkDeviceMemory,
        offset: VkDeviceSize,
    ) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkBindBufferMemory(self.device, self.buffer, memory, offset);
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
        }
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

pub struct DeviceMemory {
    pub device_memory: VkDeviceMemory,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for DeviceMemory {
    fn default() -> Self {
        Self {
            device_memory: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for DeviceMemory {
    fn drop(&mut self) {
        if self.device_memory != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping device memory");

            unsafe {
                vkFreeMemory(
                    self.device,
                    self.device_memory,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl DeviceMemory {
    pub fn allocate(
        device: VkDevice,
        allocate_info: &VkMemoryAllocateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut device_memory = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkAllocateMemory(
                device,
                allocate_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut device_memory,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                device_memory,
                device,
                allocator,
            })
        } else {
            Err(result)
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

pub struct ShaderModule {
    pub shader_module: VkShaderModule,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for ShaderModule {
    fn default() -> Self {
        Self {
            shader_module: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for ShaderModule {
    fn drop(&mut self) {
        if self.shader_module != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping shader module");
            unsafe {
                vkDestroyShaderModule(
                    self.device,
                    self.shader_module,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl ShaderModule {
    pub fn create(
        device: VkDevice,
        create_info: &VkShaderModuleCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<ShaderModule, VkResult> {
        let mut shader_module = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateShaderModule(
                device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut shader_module,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                shader_module,
                device,
                allocator,
            })
        } else {
            Err(result)
        }
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

pub struct DescriptorPool {
    pub descriptor_pool: VkDescriptorPool,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for DescriptorPool {
    fn default() -> Self {
        Self {
            descriptor_pool: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for DescriptorPool {
    fn drop(&mut self) {
        if self.descriptor_pool != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping descriptor pool");

            unsafe {
                vkDestroyDescriptorPool(
                    self.device,
                    self.descriptor_pool,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl DescriptorPool {
    pub fn create(
        device: VkDevice,
        create_info: &VkDescriptorPoolCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<DescriptorPool, VkResult> {
        let mut result = VK_SUCCESS;
        let mut descriptor_pool = std::ptr::null_mut();

        unsafe {
            result = vkCreateDescriptorPool(
                device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut descriptor_pool,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                descriptor_pool,
                device,
                allocator,
            })
        } else {
            Err(result)
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

pub struct DescriptorSetLayouts {
    pub descriptor_set_layouts: Vec<VkDescriptorSetLayout>,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for DescriptorSetLayouts {
    fn default() -> Self {
        Self {
            descriptor_set_layouts: vec![],
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for DescriptorSetLayouts {
    fn drop(&mut self) {
        if self.descriptor_set_layouts.len() > 0 && self.device != std::ptr::null_mut() {
            println!("Dropping descriptor set layouts");

            for descriptor_set_layout in &self.descriptor_set_layouts {
                unsafe {
                    vkDestroyDescriptorSetLayout(
                        self.device,
                        *descriptor_set_layout,
                        match self.allocator {
                            Some(x) => &x,
                            None => std::ptr::null(),
                        },
                    );
                }
            }
        }
    }
}

impl DescriptorSetLayouts {
    pub fn create(
        device: VkDevice,
        create_infos: &Vec<VkDescriptorSetLayoutCreateInfo>,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<DescriptorSetLayouts, VkResult> {
        let mut result = VK_SUCCESS;
        let mut descriptor_set_layouts = Vec::with_capacity(create_infos.len());

        for create_info in create_infos {
            let mut descriptor_set_layout = std::ptr::null_mut();

            unsafe {
                result = vkCreateDescriptorSetLayout(
                    device,
                    create_info,
                    match allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                    &mut descriptor_set_layout,
                );
            }

            if result < VK_SUCCESS {
                return Err(result);
            }

            descriptor_set_layouts.push(descriptor_set_layout);
        }

        Ok(Self {
            descriptor_set_layouts,
            device,
            allocator,
        })
    }
}

impl VkDescriptorSetAllocateInfo {
    pub fn new(
        p_next: Option<*const c_void>,
        desc_pool: &VkDescriptorPool,
        desc_set_lyts: &Vec<VkDescriptorSetLayout>,
    ) -> Self {
        Self {
            sType: VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            pNext: match p_next {
                Some(x) => x,
                None => std::ptr::null(),
            },
            descriptorPool: *desc_pool,
            descriptorSetCount: desc_set_lyts.len() as u32,
            pSetLayouts: desc_set_lyts.as_ptr(),
        }
    }
}

pub struct DescriptorSets {
    pub descriptor_sets: Vec<VkDescriptorSet>,
    descriptor_pool: VkDescriptorPool,
    device: VkDevice,
}

impl Default for DescriptorSets {
    fn default() -> Self {
        Self {
            descriptor_sets: vec![],
            descriptor_pool: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
        }
    }
}

impl Drop for DescriptorSets {
    fn drop(&mut self) {
        // if self.descriptor_sets.len() > 0 && self.device != std::ptr::null_mut() {
        //     println!("Dropping descriptor sets");

        //     unsafe {
        //         vkFreeDescriptorSets(
        //             self.device,
        //             self.descriptor_pool,
        //             self.descriptor_sets.len() as u32,
        //             self.descriptor_sets.as_ptr(),
        //         );
        //     }
        // }
    }
}

impl DescriptorSets {
    pub fn allocate(
        device: VkDevice,
        allocate_info: &VkDescriptorSetAllocateInfo,
    ) -> Result<DescriptorSets, VkResult> {
        let mut result = VK_SUCCESS;
        let mut descriptor_sets =
            vec![std::ptr::null_mut(); allocate_info.descriptorSetCount as usize];

        unsafe {
            result = vkAllocateDescriptorSets(device, allocate_info, descriptor_sets.as_mut_ptr());
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                descriptor_sets,
                descriptor_pool: allocate_info.descriptorPool,
                device,
            })
        } else {
            Err(result)
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

pub struct PipelineLayout {
    pub pipeline_layout: VkPipelineLayout,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for PipelineLayout {
    fn default() -> Self {
        Self {
            pipeline_layout: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for PipelineLayout {
    fn drop(&mut self) {
        if self.pipeline_layout != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping pipeline layout");

            unsafe {
                vkDestroyPipelineLayout(
                    self.device,
                    self.pipeline_layout,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl PipelineLayout {
    pub fn create(
        device: VkDevice,
        create_info: &VkPipelineLayoutCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut result = VK_SUCCESS;
        let mut pipeline_layout = std::ptr::null_mut();

        unsafe {
            result = vkCreatePipelineLayout(
                device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut pipeline_layout,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                pipeline_layout,
                device,
                allocator,
            })
        } else {
            Err(result)
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

pub struct CommandPool {
    pub command_pool: VkCommandPool,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for CommandPool {
    fn default() -> Self {
        Self {
            command_pool: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for CommandPool {
    fn drop(&mut self) {
        if self.command_pool != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping commmand pool");
            unsafe {
                vkDestroyCommandPool(
                    self.device,
                    self.command_pool,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl CommandPool {
    pub fn create(
        device: VkDevice,
        create_info: &VkCommandPoolCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut result = VK_SUCCESS;
        let mut command_pool = std::ptr::null_mut();

        unsafe {
            result = vkCreateCommandPool(
                device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut command_pool,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                device,
                command_pool,
                allocator,
            })
        } else {
            Err(result)
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

#[derive(Clone)]
pub struct Image {
    pub image: VkImage,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for Image {
    fn default() -> Self {
        Self {
            image: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for Image {
    fn drop(&mut self) {
        if self.image != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping image");
            unsafe {
                vkDestroyImage(
                    self.device,
                    self.image,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl Image {
    pub fn create(
        device: VkDevice,
        create_info: &VkImageCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut image = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateImage(
                device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut image,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                image,
                device,
                allocator,
            })
        } else {
            Err(result)
        }
    }

    pub fn memory_requirements(&self) -> VkMemoryRequirements {
        let mut mem_reqs = VkMemoryRequirements::default();

        unsafe {
            vkGetImageMemoryRequirements(self.device, self.image, &mut mem_reqs);
        }

        mem_reqs
    }

    pub fn bind_memory(
        &self,
        memory: VkDeviceMemory,
        offset: VkDeviceSize,
    ) -> Result<(), VkResult> {
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkBindImageMemory(self.device, self.image, memory, offset);
        }

        if result >= VK_SUCCESS {
            Ok(())
        } else {
            Err(result)
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

pub struct ImageView {
    pub image_view: VkImageView,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for ImageView {
    fn default() -> Self {
        Self {
            image_view: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for ImageView {
    fn drop(&mut self) {
        if self.image_view != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping image view");

            unsafe {
                vkDestroyImageView(
                    self.device,
                    self.image_view,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl ImageView {
    pub fn create(
        device: VkDevice,
        create_info: &VkImageViewCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<ImageView, VkResult> {
        let mut image_view = std::ptr::null_mut();
        let mut result = VK_SUCCESS;

        unsafe {
            result = vkCreateImageView(
                device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut image_view,
            );
        }

        if result >= VK_SUCCESS {
            Ok(ImageView {
                image_view,
                device,
                allocator,
            })
        } else {
            Err(result)
        }
    }
}

pub struct CommandBuffers {
    pub cmd_buffs: Vec<VkCommandBuffer>,
    command_pool: VkCommandPool,
    device: VkDevice,
}

impl Default for CommandBuffers {
    fn default() -> Self {
        Self {
            cmd_buffs: vec![],
            command_pool: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
        }
    }
}

impl Drop for CommandBuffers {
    fn drop(&mut self) {
        if self.cmd_buffs.len() > 0 && self.device != std::ptr::null_mut() {
            println!("Dropping command buffers");
            unsafe {
                vkFreeCommandBuffers(
                    self.device,
                    self.command_pool,
                    self.cmd_buffs.len() as u32,
                    self.cmd_buffs.as_ptr(),
                );
            }
        }
    }
}

impl CommandBuffers {
    pub fn allocate(
        device: VkDevice,
        allocate_info: &VkCommandBufferAllocateInfo,
    ) -> Result<CommandBuffers, VkResult> {
        let mut result = VK_SUCCESS;
        let mut vk_cmd_buffs =
            vec![std::ptr::null_mut(); allocate_info.commandBufferCount as usize];

        unsafe {
            result = vkAllocateCommandBuffers(device, allocate_info, vk_cmd_buffs.as_mut_ptr());
        }

        if result >= VK_SUCCESS {
            let mut cmd_buff = CommandBuffers {
                cmd_buffs: Vec::with_capacity(allocate_info.commandBufferCount as usize),
                device,
                command_pool: allocate_info.commandPool,
            };

            for vk_cmd_buff in vk_cmd_buffs {
                cmd_buff.cmd_buffs.push(vk_cmd_buff);
            }

            Ok(cmd_buff)
        } else {
            Err(result)
        }
    }

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

impl Default for Queue {
    fn default() -> Self {
        Self {
            q: std::ptr::null_mut(),
        }
    }
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

pub struct GraphicsPipelines {
    pub pipelines: Vec<VkPipeline>,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for GraphicsPipelines {
    fn default() -> Self {
        Self {
            pipelines: vec![],
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for GraphicsPipelines {
    fn drop(&mut self) {
        if self.pipelines.len() > 0 && self.device != std::ptr::null_mut() {
            println!("Dropping graphics pipelines");

            for pipeline in &self.pipelines {
                unsafe {
                    vkDestroyPipeline(
                        self.device,
                        *pipeline,
                        match self.allocator {
                            Some(x) => &x,
                            None => std::ptr::null(),
                        },
                    );
                }
            }
        }
    }
}

impl GraphicsPipelines {
    pub fn create(
        device: VkDevice,
        pipeline_cache: Option<VkPipelineCache>,
        create_infos: &Vec<VkGraphicsPipelineCreateInfo>,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<GraphicsPipelines, VkResult> {
        let mut result = VK_SUCCESS;
        let mut pipelines = vec![std::ptr::null_mut(); create_infos.len() as usize];

        unsafe {
            result = vkCreateGraphicsPipelines(
                device,
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
            Ok(Self {
                pipelines,
                device,
                allocator,
            })
        } else {
            Err(result)
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

pub struct Fence {
    pub fence: VkFence,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for Fence {
    fn default() -> Self {
        Self {
            fence: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for Fence {
    fn drop(&mut self) {
        if self.fence != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping fence");

            unsafe {
                vkDestroyFence(
                    self.device,
                    self.fence,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl Fence {
    pub fn create(
        device: VkDevice,
        create_info: &VkFenceCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut result = VK_SUCCESS;
        let mut fence = std::ptr::null_mut();

        unsafe {
            result = vkCreateFence(
                device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut fence,
            )
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                fence,
                device,
                allocator,
            })
        } else {
            Err(result)
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

pub struct Semaphore {
    pub semaphore: VkSemaphore,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for Semaphore {
    fn default() -> Self {
        Self {
            semaphore: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for Semaphore {
    fn drop(&mut self) {
        if self.semaphore != std::ptr::null_mut() && self.device != std::ptr::null_mut() {
            println!("Dropping semaphore");

            unsafe {
                vkDestroySemaphore(
                    self.device,
                    self.semaphore,
                    match self.allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                );
            }
        }
    }
}

impl Semaphore {
    pub fn create(
        device: VkDevice,
        create_info: &VkSemaphoreCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut result = VK_SUCCESS;
        let mut semaphore: VkSemaphore = std::ptr::null_mut();

        unsafe {
            result = vkCreateSemaphore(
                device,
                create_info,
                match allocator {
                    Some(x) => &x,
                    None => std::ptr::null(),
                },
                &mut semaphore,
            );
        }

        if result >= VK_SUCCESS {
            Ok(Self {
                device,
                semaphore,
                allocator,
            })
        } else {
            Err(result)
        }
    }
}
pub struct Semaphores {
    pub semaphores: Vec<VkSemaphore>,
    device: VkDevice,
    allocator: Option<VkAllocationCallbacks>,
}

impl Default for Semaphores {
    fn default() -> Self {
        Self {
            semaphores: vec![],
            device: std::ptr::null_mut(),
            allocator: Option::default(),
        }
    }
}

impl Drop for Semaphores {
    fn drop(&mut self) {
        if self.semaphores.len() > 0 && self.device != std::ptr::null_mut() {
            println!("Dropping semaphores");

            for semaphore in &self.semaphores {
                unsafe {
                    vkDestroySemaphore(
                        self.device,
                        *semaphore,
                        match self.allocator {
                            Some(x) => &x,
                            None => std::ptr::null(),
                        },
                    );
                }
            }
        }
    }
}

impl Semaphores {
    pub fn create(
        device: VkDevice,
        semaphore_count: usize,
        create_info: &VkSemaphoreCreateInfo,
        allocator: Option<VkAllocationCallbacks>,
    ) -> Result<Self, VkResult> {
        let mut result = VK_SUCCESS;
        let mut semaphores = vec![std::ptr::null_mut(); semaphore_count];

        for semaphore in &mut semaphores {
            unsafe {
                result = vkCreateSemaphore(
                    device,
                    create_info,
                    match allocator {
                        Some(x) => &x,
                        None => std::ptr::null(),
                    },
                    semaphore,
                );

                if result < VK_SUCCESS {
                    return Err(result);
                }
            }
        }

        Ok(Self {
            device,
            semaphores,
            allocator,
        })
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
