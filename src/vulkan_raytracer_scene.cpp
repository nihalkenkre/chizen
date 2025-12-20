#include "vulkan_raytracer_scene.hpp"
#include "vulkan_objects.hpp"
#include "scene.hpp"
#include "resources.hpp"
#include "utils.hpp"

VulkanRaytracerScene::VulkanRaytracerScene(const Scene& scene, const VkDevice device, const VmaAllocator allocator, const size_t uniform_buffer_alignment, ComputeHelpers* compute_helpers)
	: mDevice(device)
{
	for (const auto& mesh : scene.GetMeshes())
	{
		for (const auto& prim : mesh.GetPrimitives())
		{
			mBLASes.push_back(
				std::make_unique<BLAccelerationStructure>(
					device, allocator, prim, scene.GetVertexData(), compute_helpers, "BLAS"
				)
			);
		}
	}

	std::vector<VkAccelerationStructureInstanceKHR> instances;
	instances.reserve(scene.GetMeshInstances().size());

	for (const auto& mesh_instance : scene.GetMeshInstances())
	{
		const VkAccelerationStructureDeviceAddressInfoKHR blas_addr_info = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
			.accelerationStructure = mBLASes[mesh_instance.GetMeshIndex()]->GetAS(),
		};

		glm::mat4 xform = {};
		std::memcpy(&xform, scene.GetUniformData().data() + mesh_instance.GetModelMatrixOffset(), sizeof(xform));

		xform = glm::transpose(xform);
		VkTransformMatrixKHR inst_xform;
		std::memcpy(&inst_xform, &xform, sizeof(inst_xform));

		VkAccelerationStructureInstanceKHR inst = {
			.transform = inst_xform,
			.mask = 0xFF,
			.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(device, &blas_addr_info),
		};

		instances.push_back(inst);
	}

	mTLAS = std::make_unique<TLAccelerationStructure>(
		device, allocator, instances, compute_helpers, "TLAS"
	);
}

VulkanRaytracerScene::~VulkanRaytracerScene() noexcept
{
}

VkAccelerationStructureKHR VulkanRaytracerScene::GetTLAS() const
{
	return mTLAS->GetAS();;
}
