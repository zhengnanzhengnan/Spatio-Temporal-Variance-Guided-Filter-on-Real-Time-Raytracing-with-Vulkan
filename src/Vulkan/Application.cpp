#include "Application.hpp"
#include "Buffer.hpp"
#include "CommandPool.hpp"
#include "CommandBuffers.hpp"
#include "DebugUtilsMessenger.hpp"
#include "DepthBuffer.hpp"
#include "Device.hpp"
#include "Fence.hpp"
#include "FrameBuffer.hpp"
#include "GraphicsPipeline.hpp"*
#include "Instance.hpp"
#include "PipelineLayout.hpp"
#include "RenderPass.hpp"
#include "Semaphore.hpp"
#include "Surface.hpp"
#include "SwapChain.hpp"
#include "Window.hpp"
#include "Assets/Model.hpp"
#include "Assets/Scene.hpp"
#include "Assets/UniformBuffer.hpp"
#include "Utilities/Exception.hpp"
#include <array>

namespace Vulkan {

Application::Application(const WindowConfig& windowConfig, const VkPresentModeKHR presentMode, const bool enableValidationLayers) :
	presentMode_(presentMode)//presentMode�������չʾͼ�񵽴��ڣ�������չʾ��ֱͬ��
	//��ֱͬ����������Ⱦ֡������ʾ��ˢ��Ƶ��ͬ������ֹ���ֻ���˺��
{
	const auto validationLayers = enableValidationLayers
		? std::vector<const char*>{"VK_LAYER_KHRONOS_validation"}
		: std::vector<const char*>();

	//��ʾ����
	window_.reset(new class Window(windowConfig));
	//vulkanʵ����ʹ��VulkanAPI����㣬��������Vulkan���󶼴����ﴴ��
	instance_.reset(new Instance(*window_, validationLayers, VK_API_VERSION_1_2));
	//��֤��
	debugUtilsMessenger_.reset(enableValidationLayers ? new DebugUtilsMessenger(*instance_, VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) : nullptr);
	//����Ⱦ����
	surface_.reset(new Surface(*instance_));
}

Application::~Application()
{
	Application::DeleteSwapChain();

	commandPool_.reset();
	device_.reset();
	surface_.reset();
	debugUtilsMessenger_.reset();
	instance_.reset();
	window_.reset();
}

//��չ��������VulkanAPI���ܣ����׷
//�������豸����Ҳ������ʵ����
const std::vector<VkExtensionProperties>& Application::Extensions() const
{
	return instance_->Extensions();
}

//�������ں��ĳ�����������µĴ����Ըı����ǿAPI���ܣ�����֤�����ڵ��Ժͼ�����
const std::vector<VkLayerProperties>& Application::Layers() const
{
	return instance_->Layers();
}

//�����豸����GPU
const std::vector<VkPhysicalDevice>& Application::PhysicalDevices() const
{
	return instance_->PhysicalDevices();
}

//���������豸����������Ӧ���߼��豸
void Application::SetPhysicalDevice(VkPhysicalDevice physicalDevice)
{
	//����Ƿ��Ѿ����ù����豸
	if (device_)
	{
		Throw(std::logic_error("physical device has already been set"));
	}

	//��Ҫ��extension
	std::vector<const char*> requiredExtensions = 
	{
		// VK_KHR_swapchain
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	//�����豸��ѡ����
	VkPhysicalDeviceFeatures deviceFeatures = {};
	
	SetPhysicalDevice(physicalDevice, requiredExtensions, deviceFeatures, nullptr);
	OnDeviceSet();

	// Create swap chain and command buffers.
	CreateSwapChain();
}

void Application::Run()
{
	if (!device_)
	{
		Throw(std::logic_error("physical device has not been set"));
	}

	currentFrame_ = 0;

	window_->DrawFrame = [this]() { DrawFrame(); };
	window_->OnKey = [this](const int key, const int scancode, const int action, const int mods) { OnKey(key, scancode, action, mods); };
	window_->OnCursorPosition = [this](const double xpos, const double ypos) { OnCursorPosition(xpos, ypos); };
	window_->OnMouseButton = [this](const int button, const int action, const int mods) { OnMouseButton(button, action, mods); };
	window_->OnScroll = [this](const double xoffset, const double yoffset) { OnScroll(xoffset, yoffset); };
	window_->Run();
	device_->WaitIdle();
}

void Application::SetPhysicalDevice(
	VkPhysicalDevice physicalDevice, 
	std::vector<const char*>& requiredExtensions, 
	VkPhysicalDeviceFeatures& deviceFeatures,
	void* nextDeviceFeatures)
{
	device_.reset(new class Device(physicalDevice, *surface_, requiredExtensions, deviceFeatures, nextDeviceFeatures));
	commandPool_.reset(new class CommandPool(*device_, device_->GraphicsFamilyIndex(), true));
}

void Application::OnDeviceSet()
{
}

void Application::CreateSwapChain()
{
	
	// Wait until the window is visible.
	while (window_->IsMinimized())
	{
		window_->WaitForEvents();
	}

	swapChain_.reset(new class SwapChain(*device_, presentMode_));

	depthBuffer_.reset(new class DepthBuffer(*commandPool_, swapChain_->Extent()));

	for (size_t i = 0; i != swapChain_->ImageViews().size(); ++i)
	{
		imageAvailableSemaphores_.emplace_back(*device_);
		renderFinishedSemaphores_.emplace_back(*device_);
		inFlightFences_.emplace_back(*device_, true);
		uniformBuffers_.emplace_back(*device_);
	}
	// ��ȡ������ͼ��ĳߴ�͸�ʽ
	VkExtent2D imageExtent = swapChain_->Extent();
	VkFormat imageFormat = swapChain_->Format();

	// ����ͼ�������ģʽ��ʹ�÷�ʽ
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	//ͼ���ڴ�������
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	//������Ȼ��������ͼ��
	depthImage_.reset(new Vulkan::Image(*device_, swapChain_->Extent(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
	depthImageMemory_.reset(new Vulkan::DeviceMemory(depthImage_->AllocateMemory(properties)));
	depthImageView_.reset(new Vulkan::ImageView(*device_, depthImage_->Handle(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT));
	depthSampler_.reset(new Vulkan::Sampler(*device_, Vulkan::SamplerConfig()));

	//����motion vector������ͼ��
	motionVectorImage_.reset(new Vulkan::Image(*device_, swapChain_->Extent(), VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
	motionVectorImageMemory_.reset(new Vulkan::DeviceMemory(motionVectorImage_->AllocateMemory(properties)));
	motionVectorImage_->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false);
	motionVectorImageView_.reset(new Vulkan::ImageView(*device_, motionVectorImage_->Handle(), VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT));
	motionVectorSampler_.reset(new Vulkan::Sampler(*device_, Vulkan::SamplerConfig()));

	// ���� saveImage_ ����ͷ����ڴ�
	saveImage_.reset(new Vulkan::Image(*device_, imageExtent, imageFormat, tiling, usage));
	saveImageMemory_.reset(new Vulkan::DeviceMemory(saveImage_->AllocateMemory(properties)));

	//����ͼ����ͼΪ�հף�������Ϊ��ָ��
	first_saveImageview_ = new Vulkan::ImageView(*device_, saveImage_->Handle(), imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, true);
	first_depthImageview_ = new Vulkan::ImageView(*device_, depthImage_->Handle(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, true);

	//��֡ͼ��û����һ֡ͼ�����ͼ��������Ϊ��
	saveImageView_.reset(first_saveImageview_);
	depthImageView_.reset(first_depthImageview_);

	//����֡ͼ�񲼾�ת��������ʹ��
	depthImage_->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_GENERAL, true);
	saveImage_->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_GENERAL, false);

	//����ͼ�ι���
	graphicsPipeline_.reset(new class GraphicsPipeline(*swapChain_, *depthBuffer_, uniformBuffers_, GetScene(), *depthImageView_, *depthSampler_, isWireFrame_));

	//�涨��ɫ������������������ɫ����Ⱥ�motion vector���棩
	for (const auto& imageView : swapChain_->ImageViews())
	{
		swapChainFramebuffers_.emplace_back(*imageView, graphicsPipeline_->RenderPass(), *motionVectorImageView_);
	}

	commandBuffers_.reset(new CommandBuffers(*commandPool_, static_cast<uint32_t>(swapChainFramebuffers_.size())));
}

void Application::DeleteSwapChain()
{
	commandBuffers_.reset();//ֱ������commandBuffers���󣬶������������
	swapChainFramebuffers_.clear();
	graphicsPipeline_.reset();
	uniformBuffers_.clear();
	inFlightFences_.clear();
	renderFinishedSemaphores_.clear();
	imageAvailableSemaphores_.clear();
	depthBuffer_.reset();
	swapChain_.reset();
	saveImage_.reset();
	saveImageMemory_.reset();
	saveImageView_.reset();
	motionVectorImage_.reset();
	motionVectorImageMemory_.reset();
	motionVectorImageView_.reset();
	motionVectorSampler_.reset();
	depthImage_.reset();
	depthImageMemory_.reset();
	depthImageView_.reset();
	depthSampler_.reset();
}

void Application::DrawFrame()
{
	const auto noTimeout = std::numeric_limits<uint64_t>::max();

	auto& inFlightFence = inFlightFences_[currentFrame_];
	const auto imageAvailableSemaphore = imageAvailableSemaphores_[currentFrame_].Handle();
	const auto renderFinishedSemaphore = renderFinishedSemaphores_[currentFrame_].Handle();

	inFlightFence.Wait(noTimeout);

	//��ȡ��һ֡������ͼ��
	uint32_t imageIndex;
	auto result = vkAcquireNextImageKHR(device_->Handle(), swapChain_->Handle(), noTimeout, imageAvailableSemaphore, nullptr, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || isWireFrame_ != graphicsPipeline_->IsWireFrame())
	{
		RecreateSwapChain();
		return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		Throw(std::runtime_error(std::string("failed to acquire next image (") + ToString(result) + ")"));
	}

	// ��ȡ������ͼ��ĳߴ�͸�ʽ
	VkExtent2D imageExtent = swapChain_->Extent();
	VkFormat imageFormat = swapChain_->Format();

	// ����ͼ�������ģʽ��ʹ�÷�ʽ
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	const auto commandBuffer = commandBuffers_->Begin(imageIndex);
	Render(commandBuffer, imageIndex);
	commandBuffers_->End(imageIndex); 

	UpdateUniformBuffer(imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkCommandBuffer commandBuffers[]{ commandBuffer };
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = commandBuffers;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	inFlightFence.Reset();

	Check(vkQueueSubmit(device_->GraphicsQueue(), 1, &submitInfo, inFlightFence.Handle()),
		"submit draw command buffer");

	// ��ȡ�������е�ǰ֡��ͼ��
	const auto& images = swapChain_->Images();
	VkImage saveImageHandle = images[currentFrame_];
	auto currentFrameImage = std::make_unique<Vulkan::Image>(*device_, imageExtent, imageFormat, tiling, usage, saveImageHandle, true);

	// ��ȡ�������е�ǰ֡�����ͼ��
	const Vulkan::Image& depthImage = DepthBuffer().Image();
	VkImage depthImageHandle = depthImage.Handle();
	auto currentDepthImage = std::make_unique<Vulkan::Image>(*device_, imageExtent, VK_FORMAT_D32_SFLOAT, tiling, usage, depthImageHandle, true);

	//������һ֡���ͼ��
	if (depthImage_)
	{
		currentDepthImage->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, true);

		depthImage_->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, true);

		CopyDepthImage(*currentDepthImage, *depthImage_);

		currentDepthImage->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, false);

		depthImage_->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_GENERAL, true);
	}

	//������һ֡ͼ��
	if (saveImage_)
	{
		// Step 1: Transition srcImage to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		currentFrameImage->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, false);

		// Step 2: Transition dstImage to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		saveImage_->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, false);

		CopyImage(*currentFrameImage, *saveImage_);

		// Step 4: Transition srcImage back to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		currentFrameImage->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, false);

		// Step 5: Transition dstImage to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		saveImage_->TransitionImageLayout(*commandPool_, VK_IMAGE_LAYOUT_GENERAL, false);
	}

	//����Imageview
	saveImageView_.reset(new Vulkan::ImageView(*device_, saveImage_->Handle(), imageFormat, VK_IMAGE_ASPECT_COLOR_BIT));
	depthImageView_.reset(new Vulkan::ImageView(*device_, depthImage_->Handle(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT));

	VkSwapchainKHR swapChains[] = { swapChain_->Handle() };
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(device_->PresentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
		return;
	}
	
	if (result != VK_SUCCESS)
	{
		Throw(std::runtime_error(std::string("failed to present next image (") + ToString(result) + ")"));
	}

	currentFrame_ = (currentFrame_ + 1) % inFlightFences_.size();
}

void Application::Render(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
{
	std::array<VkClearValue, 3> clearValues = {};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	clearValues[2].color = { {0.0f, 0.0f} };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = graphicsPipeline_->RenderPass().Handle();
	renderPassInfo.framebuffer = swapChainFramebuffers_[imageIndex].Handle();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChain_->Extent();
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		const auto& scene = GetScene();

		VkDescriptorSet descriptorSets[] = { graphicsPipeline_->DescriptorSet(imageIndex) };
		VkBuffer vertexBuffers[] = { scene.VertexBuffer().Handle() };
		const VkBuffer indexBuffer = scene.IndexBuffer().Handle();
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_->Handle());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_->PipelineLayout().Handle(), 0, 1, descriptorSets, 0, nullptr);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;

		for (const auto& model : scene.Models())
		{
			const auto vertexCount = static_cast<uint32_t>(model.NumberOfVertices());
			const auto indexCount = static_cast<uint32_t>(model.NumberOfIndices());

			vkCmdDrawIndexed(commandBuffer, indexCount, 1, indexOffset, vertexOffset, 0);

			vertexOffset += vertexCount;
			indexOffset += indexCount;
		}
	}
	vkCmdEndRenderPass(commandBuffer);
}

void Application::UpdateUniformBuffer(const uint32_t imageIndex)
{
	//uniformBuffers_[imageIndex].SetValue(GetUniformBufferObject(swapChain_->Extent()));

	Assets::UniformBufferObject ubo = GetUniformBufferObject(swapChain_->Extent());

	// �洢��һ֡��ModelView��Projection��ubo
	ubo.LastFrameModelView = lastFrameModelView;
	ubo.LastFrameProjection = lastFrameProjection;

	// ����lastFrameModelView��lastFrameProjectionΪ��ǰ֡��ֵ���Թ���һ֡ʹ��
	lastFrameModelView = ubo.ModelView;
	lastFrameProjection = ubo.Projection;

	uniformBuffers_[imageIndex].SetValue(ubo);
}

void Application::RecreateSwapChain()
{
	device_->WaitIdle();
	DeleteSwapChain();
	CreateSwapChain();
}

//���ƺ���
void Application::CopyImage(Vulkan::Image& srcImage, Vulkan::Image& dstImage)
{
	// Step 3: Copy srcImage to dstImage
	VkCommandBuffer commandBuffer = commandBuffers_->Begin(0);

	// Define image copy region
	VkImageCopy copyRegion{};
	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.extent.width = srcImage.Extent().width;
	copyRegion.extent.height = srcImage.Extent().height;
	copyRegion.extent.depth = 1;

	// Perform copy operation
	vkCmdCopyImage(
		commandBuffer,
		srcImage.Handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstImage.Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyRegion
	);

	commandBuffers_->End(0);

	// Submit command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence fence;
	vkCreateFence(device_->Handle(), &fenceInfo, nullptr, &fence);

	vkQueueSubmit(device_->GraphicsQueue(), 1, &submitInfo, fence);
	vkWaitForFences(device_->Handle(), 1, &fence, VK_TRUE, UINT64_MAX);

	vkDestroyFence(device_->Handle(), fence, nullptr);
}

void Application::CopyDepthImage(Vulkan::Image& srcImage, Vulkan::Image& dstImage)
{
	// Step 3: Copy srcImage to dstImage
	VkCommandBuffer commandBuffer = commandBuffers_->Begin(0);

	// Define image copy region
	VkImageCopy copyRegion{};
	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.extent.width = srcImage.Extent().width;
	copyRegion.extent.height = srcImage.Extent().height;
	copyRegion.extent.depth = 1;

	// Perform copy operation
	vkCmdCopyImage(
		commandBuffer,
		srcImage.Handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstImage.Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyRegion
	);

	commandBuffers_->End(0);

	// Submit command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence fence;
	vkCreateFence(device_->Handle(), &fenceInfo, nullptr, &fence);

	vkQueueSubmit(device_->GraphicsQueue(), 1, &submitInfo, fence);
	vkWaitForFences(device_->Handle(), 1, &fence, VK_TRUE, UINT64_MAX);

	vkDestroyFence(device_->Handle(), fence, nullptr);
}

}
