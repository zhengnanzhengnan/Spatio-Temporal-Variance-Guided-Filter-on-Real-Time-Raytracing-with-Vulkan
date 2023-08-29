#include "RenderPass.hpp"
#include "DepthBuffer.hpp"
#include "Device.hpp"
#include "SwapChain.hpp"
#include <array>

namespace Vulkan {

	RenderPass::RenderPass(
		const class SwapChain& swapChain,
		const class DepthBuffer& depthBuffer,
		const VkAttachmentLoadOp colorBufferLoadOp,
		const VkAttachmentLoadOp depthBufferLoadOp) :
		swapChain_(swapChain),
		depthBuffer_(depthBuffer)
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChain.Format();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = colorBufferLoadOp;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = colorBufferLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = depthBuffer.Format();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = depthBufferLoadOp;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = depthBufferLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//将motion vector输出到一个图像颜色附件
		VkAttachmentDescription motionVectorAttachment = {};
		motionVectorAttachment.format = VK_FORMAT_R32G32_SFLOAT;// motion vector图像格式选择VK_FORMAT_R32G32_SFLOAT
		motionVectorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		motionVectorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		motionVectorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		motionVectorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		motionVectorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		motionVectorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		motionVectorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//添加motion vector的图像颜色附件引用
		VkAttachmentReference motionVectorAttachmentRef = {};
		motionVectorAttachmentRef.attachment = 2; // 第三个附件
		motionVectorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//更新子通道数
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 2;  // 现在有两个颜色附件
		VkAttachmentReference colorAttachments[] = { colorAttachmentRef, motionVectorAttachmentRef };
		subpass.pColorAttachments = colorAttachments;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 3> attachments =
		{
			colorAttachment,
			depthAttachment,
			motionVectorAttachment
		};

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		Check(vkCreateRenderPass(swapChain_.Device().Handle(), &renderPassInfo, nullptr, &renderPass_),
			"create render pass");
	}

	RenderPass::~RenderPass()
	{
		if (renderPass_ != nullptr)
		{
			vkDestroyRenderPass(swapChain_.Device().Handle(), renderPass_, nullptr);
			renderPass_ = nullptr;
		}
	}

}
