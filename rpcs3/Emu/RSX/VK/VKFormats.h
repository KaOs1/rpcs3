#pragma once
#include "VKHelpers.h"

namespace vk
{
	struct gpu_formats_support
	{
		bool d24_unorm_s8 : 1;
		bool d32_sfloat_s8 : 1;
	};

	gpu_formats_support get_optimal_tiling_supported_formats(VkPhysicalDevice physical_device);
	VkFormat get_compatible_depth_surface_format(const gpu_formats_support &support, rsx::surface_depth_format format);
	VkSamplerAddressMode vk_wrap_mode(u32 gcm_wrap);
	float max_aniso(u32 gcm_aniso);
	VkComponentMapping get_component_mapping(u32 format, u8 swizzle_mask);
}