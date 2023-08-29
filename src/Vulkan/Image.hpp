#pragma once

#include "Vulkan.hpp"
#include "DeviceMemory.hpp"

namespace Vulkan
{
	class Buffer;
	class CommandPool;
	class Device;

	class Image final
	{
	public:

		Image(const Image&) = delete;
		Image& operator = (const Image&) = delete;
		Image& operator = (Image&&) = delete;

		Image(const Device& device, VkExtent2D extent, VkFormat format);
		Image(const Device& device, VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage);
		Image(const Device& device, VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, bool isManaged_);
		Image(const Device& device, const VkExtent2D extent, const VkFormat format, const VkImageTiling tiling, const VkImageUsageFlags usage, const VkImage image, bool isManaged_);
		Image(Image&& other) noexcept;
		~Image();

		const class Device& Device() const { return device_; }
		VkExtent2D Extent() const { return extent_; }
		VkFormat Format() const { return format_; }

		DeviceMemory AllocateMemory(VkMemoryPropertyFlags properties) const;
		VkMemoryRequirements GetMemoryRequirements() const;

		void TransitionImageLayout(CommandPool& commandPool, VkImageLayout newLayout, bool depth);//转换图像格式
		void CopyFrom(CommandPool& commandPool, const Buffer& buffer);//将一个缓冲区的数据复制到图像中

	private:

		bool isManaged_;

		const class Device& device_;
		const VkExtent2D extent_;
		const VkFormat format_;
		VkImageLayout imageLayout_;

		VULKAN_HANDLE(VkImage, image_)

	};

}
