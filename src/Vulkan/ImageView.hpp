#pragma once

#include "Vulkan.hpp"

namespace Vulkan
{
	class Device;

	class ImageView final
	{
	public:

		VULKAN_NON_COPIABLE(ImageView)

		explicit ImageView(const Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		explicit ImageView(const Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, bool isManaged);

		~ImageView();

		const class Device& Device() const { return device_; }

	private:

		bool isManaged_;

		const class Device& device_;
		const VkImage image_;
		const VkFormat format_;

		VULKAN_HANDLE(VkImageView, imageView_)
	};

}
