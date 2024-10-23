#include "ComputeRenderPass.h"
#include "../../../Infinite.h"
#include "../../../util/constants.h"
#include "../../Model/Models/ComputeModel.h"
#include "../../Software/ShaderAnalyzer.h"
#include "RenderPass.h"
#include "vk_mem_alloc.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace Infinite {

void ComputeRenderPass::createDescriptorSets(
    VkDescriptorSetLayout *descriptorSetLayout) {
  computeModel =
      ComputeModel(swapChainExtent.width, swapChainExtent.height, allocator);

  computeModel->createDescriptorSets2(device, descriptorSetLayout,
                                      &shaderLayout);
}

void ComputeRenderPass::createPipeline(VkDevice device,
                                       VkSampleCountFlagBits msaaSamplesrs) {

  auto computeShaderCode = readFile(R"(../assets/shaders/comp.spv)");

  auto computeShaderModule = createShaderModule(computeShaderCode, device);

  shaderLayout = {};
  generateShaderLayout(shaderLayout, {computeShaderModule},
                       {computeShaderCode});
  descriptorSetLayout = DescriptorSet::createDescriptorSetLayout(
      device, shaderLayout.highLevelLayout);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline layout!");
  }

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.stage = shaderLayout.shaderStages[0];

  VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
  computeShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  computeShaderStageInfo.module = computeShaderModule;
  computeShaderStageInfo.pName = "main";

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                               nullptr, &renderPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline!");
  }

  vkDestroyShaderModule(device, computeShaderModule, nullptr);
}

void ComputeRenderPass::recordComputeCommandBuffer(
    VkCommandBuffer commandBuffer) {
  VkCommandBufferBeginInfo beginInfo{};

  Image::transitionImageLayout(
      computeModel->getTexture(), VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error(
        "failed to begin recording compute command buffer!");
  }

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    renderPipeline);

  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
      &computeModel->getDescriptorSet2().getDescriptorSets()[currentFrame], 0,
      nullptr);

  vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record compute command buffer!");
  }
}

void ComputeRenderPass ::waitForFences() {
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);
}
void ComputeRenderPass ::resetFences() {
  vkResetFences(device, 1, &inFlightFences[currentFrame]);
}

void ComputeRenderPass::renderFrame(uint32_t currentFrame, uint32_t imageIndex,
                                    std::vector<VkSemaphore> prevSemaphores) {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  vkResetCommandBuffer(commandBufferManager.commandBuffers[currentFrame],
                       /*VkCommandBufferResetFlagBits*/ 0);
  recordComputeCommandBuffer(commandBufferManager.commandBuffers[currentFrame]);

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers =
      &commandBufferManager.commandBuffers[currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &finishedSemaphores[currentFrame];
  if (vkQueueSubmit(queues[static_cast<uint32_t>(QueueOrder::COMPUTE)].queue, 1,
                    &submitInfo,
                    getInFlighFences()[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit compute command buffer!");
  };

  Image::transitionImageLayout(
      computeModel->getTexture(), VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

}
void ComputeRenderPass::createCommandBuffers(VkCommandPool commandPool,
                                             VkPhysicalDevice physicalDevice,
                                             VkDevice device) {
  commandBufferManager.create(commandPool, device);
}
void ComputeRenderPass::createSyncObjects(VkDevice device) {
  finishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &finishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
            VK_SUCCESS) {

      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }
}

VkPipelineBindPoint ComputeRenderPass::getPipelineType() {
  return VK_PIPELINE_BIND_POINT_COMPUTE;
}

void ComputeRenderPass::destroy(VkDevice device, VmaAllocator allocator) {
  // computeModel->destroy(device, allocator);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  vkDestroyPipeline(device, renderPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, finishedSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }
}

void ComputeRenderPass::recreateSwapChainWork(
    VmaAllocator allocator, VkDevice device, VkPhysicalDevice physicalDevice,
    VkFormat swapChainImageFormat, VkExtent2D swapChainExtent,
    std::vector<VkImageView> swapChainImageViews) {}

void ComputeRenderPass::createRenderPass(VkDevice device,
                                         VkPhysicalDevice physicalDevice,
                                         VkFormat swapChainImageFormat,
                                         VkSampleCountFlagBits msaaSamples) {

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
}

void ComputeRenderPass::preInit(VkDevice device,
                                VkPhysicalDevice physicalDevice,
                                VkFormat swapChainImageFormat,
                                VkExtent2D swapChainExtent,
                                VmaAllocator allocator,
                                VkSampleCountFlagBits msaaSamples,
                                std::vector<VkImageView> swapChainImageViews) {
  createPipeline(device, msaaSamples);
  createDescriptorSets(&descriptorSetLayout);
  models.push_back(&computeModel.value());
}
}; // namespace Infinite