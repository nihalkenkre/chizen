#include "vulkan_functions.hpp"

PFN_vkSetDebugUtilsObjectNameEXT vk_SetDebugUtilsObjectNameEXT = nullptr;
PFN_vkQueueSubmit2KHR vk_QueueSubmit2KHR = nullptr;
PFN_vkSignalSemaphoreKHR vk_SignalSemaphoreKHR = nullptr;
PFN_vkCmdPipelineBarrier2KHR vk_CmdPipelineBarrier2KHR = nullptr;
PFN_vkCmdCopyBuffer2KHR vk_CmdCopyBuffer2KHR = nullptr;
PFN_vkCmdCopyBufferToImage2KHR vk_CmdCopyBufferToImage2KHR = nullptr;
PFN_vkWaitSemaphoresKHR vk_WaitSemaphoresKHR = nullptr;
PFN_vkCmdBeginRenderingKHR vk_CmdBeginRenderingKHR = nullptr;
PFN_vkCmdBindDescriptorSets2KHR vk_CmdBindDescriptorSets2KHR = nullptr;
PFN_vkCmdBindVertexBuffers2EXT vk_CmdBindVertexBuffers2EXT = nullptr;
PFN_vkCmdBindIndexBuffer2KHR vk_CmdBindIndexBuffer2KHR = nullptr;
PFN_vkCmdPushConstants2KHR vk_CmdPushConstants2KHR = nullptr;
PFN_vkCmdEndRenderingKHR vk_CmdEndRenderingKHR = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesKHR vk_GetRayTracingShaderGroupHandlesKHR = nullptr;
PFN_vkCreateRayTracingPipelinesKHR vk_CreateRayTracingPipelinesKHR = nullptr;
PFN_vkCmdTraceRaysKHR vk_CmdTraceRaysKHR = nullptr;
PFN_vkGetBufferDeviceAddressKHR vk_GetBufferDeviceAddressKHR = nullptr;
PFN_vkCreateAccelerationStructureKHR vk_CreateAccelerationStructureKHR = nullptr;
PFN_vkDestroyAccelerationStructureKHR vk_DestroyAccelerationStructureKHR = nullptr;
PFN_vkCmdBuildAccelerationStructuresKHR vk_CmdBuildAccelerationStructuresKHR = nullptr;
PFN_vkGetAccelerationStructureBuildSizesKHR vk_GetAccelerationStructureBuildSizesKHR = nullptr;
PFN_vkGetAccelerationStructureDeviceAddressKHR vk_GetAccelerationStructureDeviceAddressKHR = nullptr;

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	return vk_SetDebugUtilsObjectNameEXT(device, pNameInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier2KHR(VkCommandBuffer commandBuffer, const VkDependencyInfo* pDependencyInfo)
{
	return vk_CmdPipelineBarrier2KHR(commandBuffer, pDependencyInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSignalSemaphoreKHR(VkDevice device, const VkSemaphoreSignalInfo* pSignalInfo)
{
	return vk_SignalSemaphoreKHR(device, pSignalInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2* pCopyBufferInfo)
{
	return vk_CmdCopyBuffer2KHR(commandBuffer, pCopyBufferInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2* pCopyBufferToImageInfo)
{
	return vk_CmdCopyBufferToImage2KHR(commandBuffer, pCopyBufferToImageInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit2KHR(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2* pSubmits, VkFence fence)
{
	return vk_QueueSubmit2KHR(queue, submitCount, pSubmits, fence);
}

VKAPI_ATTR VkResult VKAPI_CALL vkWaitSemaphoresKHR(VkDevice device, const VkSemaphoreWaitInfo* pWaitInfo, uint64_t timeout)
{
	return vk_WaitSemaphoresKHR(device, pWaitInfo, timeout);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderingKHR(VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo)
{
	return vk_CmdBeginRenderingKHR(commandBuffer, pRenderingInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets2KHR(VkCommandBuffer commandBuffer, const VkBindDescriptorSetsInfo* pBindDescriptorSetsInfo)
{
	return vk_CmdBindDescriptorSets2KHR(commandBuffer, pBindDescriptorSetsInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers2EXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets, const VkDeviceSize* pSizes, const VkDeviceSize* pStrides)
{
	return vk_CmdBindVertexBuffers2EXT(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer2KHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, VkIndexType indexType)
{
	return vk_CmdBindIndexBuffer2KHR(commandBuffer, buffer, offset, size, indexType);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants2KHR(VkCommandBuffer commandBuffer, const VkPushConstantsInfo* pPushConstantsInfo)
{
	return vk_CmdPushConstants2KHR(commandBuffer, pPushConstantsInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderingKHR(VkCommandBuffer commandBuffer)
{
	return vk_CmdEndRenderingKHR(commandBuffer);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR(
	VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount,
	size_t dataSize, void* pData)
{
	return vk_GetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(
	VkDevice device, VkDeferredOperationKHR deferredOperation,
	VkPipelineCache pipelineCache, uint32_t createInfoCount,
	const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
	const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	return vk_CreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR(
	VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
	const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
	const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth)
{
	return vk_CmdTraceRaysKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
}

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo* pInfo)
{
	return vk_GetBufferDeviceAddressKHR(device, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkAccelerationStructureKHR* pAccelerationStructure)
{
	return vk_CreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks* pAllocator)
{
	return vk_DestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
{
	return vk_CmdBuildAccelerationStructuresKHR(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo, const uint32_t* pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
{
	return vk_GetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
{
	return vk_GetAccelerationStructureDeviceAddressKHR(device, pInfo);
}