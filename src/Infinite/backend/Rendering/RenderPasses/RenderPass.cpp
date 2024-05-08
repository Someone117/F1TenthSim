#include "RenderPass.h"
#include "../../Model/Models/Model.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vulkan/vulkan_core.h>

namespace Infinite {
std::vector<VkSemaphore> imageAvailableSemaphores = {};
std::vector<VkSemaphore> renderFinishedSemaphores = {};
std::vector<VkFence> inFlightFences = {};
VkQueue presentQueue;

// TODO: change polygonMode for the rasterizer for the drawing of outlines.
// TODO: change depthClampEnable to VK_TRUE and enable the GPU feature to enable
// depth clamp
// TODO: FIX THE FREAKING FILE PATH


void RenderPass::createRenderPass(VkDevice device,
                                  VkPhysicalDevice physicalDevice,
                                  VkFormat swapChainImageFormat,
                                  VkSampleCountFlagBits msaaSamples) {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.samples = msaaSamples;

  VkAttachmentDescription colorAttachmentResolve{};
  colorAttachmentResolve.format = swapChainImageFormat;
  colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentResolveRef{};
  colorAttachmentResolveRef.attachment = 2;
  colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = DepthImage::findDepthFormat(physicalDevice);
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthAttachment.samples = msaaSamples;

  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;
  subpass.pResolveAttachments = &colorAttachmentResolveRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 3> attachments = {
      colorAttachment, depthAttachment, colorAttachmentResolve};
  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
}

uint32_t RenderPass::getIndex() const { return index; }

void RenderPass::setIndex(uint32_t index) { RenderPass::index = index; }

void RenderPass::destroy(VkDevice device, VmaAllocator allocator,
                         int maxFramesInFlight) {
  destroyDepthAndColorImages(allocator);

  for (Model *model : models) {
    model->destroy(device, allocator);
  }
  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);

  for (size_t i = 0; i < maxFramesInFlight; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }
}

void RenderPass::addModel(Model *model) { models.push_back(model); }

void RenderPass::createDepthAndColorImages(unsigned int width,
                                           unsigned int height,
                                           VkFormat colorFormat,
                                           VkPhysicalDevice physicalDevice,
                                           VmaAllocator allocator) {
  if (colorImageReasource == VK_NULL_HANDLE) {
    colorImageReasource = new ColorImage;
  }
  colorImageReasource->create(width, height, colorFormat, physicalDevice,
                              allocator);

  if (depthImageReasource == VK_NULL_HANDLE) {
    depthImageReasource = new DepthImage;
  }
  depthImageReasource->create(width, height, colorFormat, physicalDevice,
                              allocator);
}

void RenderPass::createCommandBuffers(VkCommandPool commandPool,
                                      VkPhysicalDevice physicalDevice,
                                      VkDevice device,
                                      QueueFamilyIndices queueFamilyIndices) {
  commandBufferManager.create(commandPool, device);
}

void RenderPass::createSyncObjects(VkDevice device, int maxFramesInFlight) {
  imageAvailableSemaphores.resize(maxFramesInFlight);
  renderFinishedSemaphores.resize(maxFramesInFlight);
  inFlightFences.resize(maxFramesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < maxFramesInFlight; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
            VK_SUCCESS) {

      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }
}

const std::vector<Model *> &RenderPass::getModels() const { return models; }

void RenderPass::destroyDepthAndColorImages(VmaAllocator allocator) {
  depthImageReasource->destroy(allocator);
  colorImageReasource->destroy(allocator);
  delete depthImageReasource;
  delete colorImageReasource;
}

ColorImage *RenderPass::getColorImageReasource() const {
  return colorImageReasource;
}

DepthImage *RenderPass::getDepthImageReasource() const {
  return depthImageReasource;
}

VkRenderPass_T *RenderPass::getRenderPass() { return renderPass; }

VkShaderModule createShaderModule(const std::vector<char> &code,
                                  VkDevice device) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

VkPipeline RenderPass::getGraphicsPipeline() const { return graphicsPipeline; }
VkPipelineLayout RenderPass::getPipelineLayout() const {
  return pipelineLayout;
}
VkCommandBuffer RenderPass::getCommandBuffer(uint32_t index) {
  return commandBufferManager.commandBuffers[index];
}

uint32_t RenderPass::getIndex() { return index; }
} // namespace Infinite