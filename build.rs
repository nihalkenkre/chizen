use std::path::PathBuf;

use bindgen;

fn main() {
    println!("Hello Build");
    let vk_sdk = std::env::var("VULKAN_SDK").unwrap();
    println!("cargo:rustc-link-search={}/Lib", vk_sdk);
    println!("cargo:rustc-link-lib=vulkan-1");

    let vk_wrapper = bindgen::Builder::default()
        .header("vk_wrapper.h")
        .allowlist_type("VkStructureType")
        .allowlist_type("VkInstanceCreateFlags")
        .allowlist_type("VkQueueFlagBits")
        .allowlist_type("VkApplicationCreateInfo")
        .allowlist_type("VkInstanceCreateInfo")
        .allowlist_type("VkDeviceCreateInfo")
        .allowlist_type("VkAllocationCallbacks")
        .allowlist_type("VkPhysicalDevice")
        .allowlist_type("VkPhysicalDeviceProperties")
        .allowlist_type("VkPhysicalDeviceLimits")
        .allowlist_type("VkPhysicalDeviceFeatures2")
        .allowlist_type("VkPhysicalDeviceVulkan12Features")
        .allowlist_type("VkPhysicalDeviceDynamicRenderingFeatures")
        .allowlist_type("VkPhysicalDeviceSparseProperties")
        .allowlist_type("VkQueueFamilyProperties")
        .allowlist_type("VkDevice")
        .allowlist_type("VkDeviceCreateFlags")
        .allowlist_type("VkWin32SurfaceCreateInfoKHR")
        .allowlist_type("VkWin32SurfaceCreateFlagBitsKHR")
        .allowlist_type("VkSurfaceCapabilitiesKHR")
        .allowlist_type("VkSurfaceFormatKHR")
        .allowlist_type("VkPresentModeKHR")
        .allowlist_type("VkSwapchainCreateFlagsKHR")
        .allowlist_type("VkSwapchainCreateInfoKHR")
        .allowlist_type("VkSwapchainKHR")
        .allowlist_type("VkImageUsageFlagBits")
        .allowlist_type("VkImageAspectFlagBits")
        .allowlist_type("VkImageCreateInfo")
        .allowlist_type("VkImage")
        .allowlist_type("VkImageViewCreateInfo")
        .allowlist_type("VkImageViewType")
        .allowlist_type("VkImageView")
        .allowlist_type("VkBufferCreateInfo")
        .allowlist_type("VkBufferUsageFlagBits")
        .allowlist_type("VkBuffer")
        .allowlist_type("VkDeviceMemory")
        .allowlist_type("VkPhysicalDeviceMemoryProperties")
        .allowlist_type("VkMemoryRequirements")
        .allowlist_type("VkMemoryAllocateInfo")
        .allowlist_type("VkMemoryPropertyFlagBits")
        .allowlist_type("VkShaderModuleCreateInfo")
        .allowlist_type("VkShaderModuleCreateFlagBits")
        .allowlist_type("VkDescriptorPoolCreateInfo")
        .allowlist_type("VkDescriptorPoolCreateFlagBits")
        .allowlist_type("VkDescriptorSetLayoutBinding")
        .allowlist_type("VkDescriptorSetLayoutCreateInfo")
        .allowlist_type("VkShaderStageFlagBits")
        .allowlist_type("VkDescriptorSetAllocateInfo")
        .allowlist_type("VkPipelineLayoutCreateInfo")
        .allowlist_type("VkPipelineRenderingCreateInfo")
        .allowlist_type("VkPipelineStageCreateInfo")
        .allowlist_type("VkComputePipelineCreateInfo")
        .allowlist_type("VkPipelineshaderStageCreateFlagBits")
        .allowlist_type("VkWriteDescriptorSet")
        .allowlist_type("VkCommandPoolCreateInfo")
        .allowlist_type("VkCommandPoolCreateFlags")
        .allowlist_type("VkCommandPoolCreateFlagBits")
        .allowlist_type("VkCommandBufferAllocateInfo")
        .allowlist_type("VkCommandBufferBeginInfo")
        .allowlist_type("VkCommandBufferUsageFlagBits")
        .allowlist_type("VkFenceCreateInfo")
        .allowlist_type("VkFenceCreateFlagBits")
        .allowlist_type("VkSubmitInfo")
        .allowlist_type("VkVertexInputAttributeDescription")
        .allowlist_type("VkVertexInputBindingDescription")
        .allowlist_type("VkVertexInputRate")
        .allowlist_type("VkPipelineVertexInputStateCreateInfo")
        .allowlist_type("VkPipelineInputAssemblyStateCreateInfo")
        .allowlist_type("VkViewport")
        .allowlist_type("VkRect2D")
        .allowlist_type("VkPipelineViewportStateCreateInfo")
        .allowlist_type("VkPipelineRasterizationStateCreateInfo")
        .allowlist_type("VkCullModeFlagBits")
        .allowlist_type("VkPipelineMultisampleStateCreateInfo")
        .allowlist_type("VkPipelineColorBlendAttachmentState")
        .allowlist_type("VkPipelineColorBlendStateCreateInfo")
        .allowlist_type("VkColorComponentFlagBits")
        .allowlist_type("VkPipelineDepthStencilStateCreateInfo")
        .allowlist_type("VkPipelineRenderingCreateInfo")
        .allowlist_type("VkGraphicsPipelineCreateInfo")
        .allowlist_type("VkSemaphoreCreateInfo")
        .allowlist_type("VkAccessFlagBits")
        .allowlist_type("VkImageMemoryBarrier")
        .allowlist_type("VkBufferMemoryBarrier")
        .allowlist_type("VkPipelineStageFlagBits")
        .allowlist_type("VkRenderingAttachmentInfo")
        .allowlist_type("VkRenderingInfo")
        .allowlist_type("VkPresentInfoKHR")
        .allowlist_type("VkPipelineShaderStageFlagBits")
        .allowlist_function("vkCreateInstance")
        .allowlist_function("vkDestroyInstance")
        .allowlist_function("vkEnumeratePhysicalDevices")
        .allowlist_function("vkGetPhysicalDeviceProperties")
        .allowlist_function("vkGetPhysicalDeviceQueueFamilyProperties")
        .allowlist_function("vkGetPhysicalDeviceFeatures")
        .allowlist_function("vkGetPhysicalDeviceFeatures2")
        .allowlist_function("vkCreateDevice")
        .allowlist_function("vkDestroyDevice")
        .allowlist_function("vkCreateWin32SurfaceKHR")
        .allowlist_function("vkDestroySurfaceKHR")
        .allowlist_function("vkGetPhysicalDeviceSurfaceCapabilitiesKHR")
        .allowlist_function("vkGetPhysicalDeviceSurfaceFormatsKHR")
        .allowlist_function("vkGetPhysicalDeviceSurfacePresentModesKHR")
        .allowlist_function("vkGetPhysicalDeviceSurfaceSupportKHR")
        .allowlist_function("vkGetPhysicalDeviceWin32PresentationSupportKHR")
        .allowlist_function("vkCreateSwapchainKHR")
        .allowlist_function("vkGetSwapchainImagesKHR")
        .allowlist_function("vkDestroySwapchainKHR")
        .allowlist_function("vkCreateImage")
        .allowlist_function("vkDestroyImage")
        .allowlist_function("vkCreateImageView")
        .allowlist_function("vkDestroyImageView")
        .allowlist_function("vkCreateBuffer")
        .allowlist_function("vkDestroyBuffer")
        .allowlist_function("vkGetPhysicalDeviceMemoryProperties")
        .allowlist_function("vkGetBufferMemoryRequirements")
        .allowlist_function("vkGetImageMemoryRequirements")
        .allowlist_function("vkAllocateMemory")
        .allowlist_function("vkBindBufferMemory")
        .allowlist_function("vkBindImageMemory")
        .allowlist_function("vkFreeMemory")
        .allowlist_function("vkMapMemory")
        .allowlist_function("vkUnmapMemory")
        .allowlist_function("vkCreateShaderModule")
        .allowlist_function("vkDestroyShaderModule")
        .allowlist_function("vkCreateDescriptorPool")
        .allowlist_function("vkDestroyDescriptorPool")
        .allowlist_function("vkCreateDescriptorSetLayout")
        .allowlist_function("vkDestroyDescriptorSetLayout")
        .allowlist_function("vkAllocateDescriptorSets")
        .allowlist_function("vkFreeDescriptorSets")
        .allowlist_function("vkCreatePipelineLayout")
        .allowlist_function("vkDestroyPipelineLayout")
        .allowlist_function("vkCreateComputePipelines")
        .allowlist_function("vkDestroyPipeline")
        .allowlist_function("vkCreateGraphicsPipelines")
        .allowlist_function("vkUpdateDescriptorSets")
        .allowlist_function("vkCreateCommandPool")
        .allowlist_function("vkDestroyCommandPool")
        .allowlist_function("vkAllocateCommandBuffers")
        .allowlist_function("vkFreeCommandBuffers")
        .allowlist_function("vkBeginCommandBuffer")
        .allowlist_function("vkEndCommandBuffer")
        .allowlist_function("vkCmdBindPipeline")
        .allowlist_function("vkCmdBindDescriptorSets")
        .allowlist_function("vkCmdDispatch")
        .allowlist_function("vkGetDeviceQueue")
        .allowlist_function("vkQueueSubmit")
        .allowlist_function("vkCreateFence")
        .allowlist_function("vkDestroyFence")
        .allowlist_function("vkWaitForFences")
        .allowlist_function("vkCreateSemaphore")
        .allowlist_function("vkDestroySemaphore")
        .allowlist_function("vkAcquireNextImageKHR")
        .allowlist_function("vkResetFences")
        .allowlist_function("vkCmdPipelineBarrier")
        .allowlist_function("vkCmdBeginRendering")
        .allowlist_function("vkCmdBindIndexBuffer")
        .allowlist_function("vkCmdDrawIndexed")
        .allowlist_function("vkCmdEndRendering")
        .allowlist_function("vkQueuePresentKHR")
        .allowlist_function("vkQueueWaitIdle")
        .derive_default(true)
        .prepend_enum_name(false)
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .clang_arg("-I".to_owned() + &vk_sdk + "/Include")
        .generate()
        .expect("Could not generate vulkan bindings");

    let out_path = PathBuf::from(std::env::var("OUT_DIR").unwrap());
    vk_wrapper
        .write_to_file(out_path.join("vk_wrapper.rs"))
        .expect("Could not write vulkan bindings");

    println!("Bye Build");
}
