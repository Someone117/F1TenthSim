#include "ComputeRenderPass.h"
#include "../../../Infinite.h"
#include "../../../util/constants.h"
#include "RenderPass.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <ostream>
#include <random>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace Infinite {

#define PARTICLE_COUNT 128

VkPipelineShaderStageCreateInfo ComputeRenderPass::createExtraShaderModule(
    VkDevice device, VkShaderModule &computeShaderModule) {
  auto computeShaderCode = readFile(R"(../assets/shaders/comp.spv)");

  computeShaderModule = createShaderModule(computeShaderCode, device);

  VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
  computeShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  computeShaderStageInfo.module = computeShaderModule;
  computeShaderStageInfo.pName = "main";

  return computeShaderStageInfo;
}

void ComputeRenderPass::createShaderStorageBuffers(uint32_t WIDTH,
                                                   uint32_t HEIGHT,
                                                   VmaAllocator allocator) {
  shaderStorageBufferAlloc.resize(MAX_FRAMES_IN_FLIGHT);
  // Initialize particles
  std::default_random_engine rndEngine((unsigned)time(nullptr));
  std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

  // Initial particle positions on a circle
  std::vector<Particle> particles(PARTICLE_COUNT);
  for (auto &particle : particles) {
    float r = 0.25f * sqrt(rndDist(rndEngine));
    float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
    float x = r * cos(theta) * HEIGHT / WIDTH;
    float y = r * sin(theta);
    particle.position = glm::vec2(x, y);
    particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
    particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine),
                               rndDist(rndEngine), 1.0f);
  }
  VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

  BufferAlloc stagingBufferAlloc;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBufferAlloc,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  void *data;
  vmaMapMemory(allocator, stagingBufferAlloc.allocation, &data);
  memcpy(data, particles.data(), (size_t)bufferSize);
  vmaUnmapMemory(allocator, stagingBufferAlloc.allocation);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        shaderStorageBufferAlloc[i], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    // Copy data from the staging buffer (host) to the shader storage buffer
    // (GPU)
    copyBuffer(stagingBufferAlloc, shaderStorageBufferAlloc[i], bufferSize,
               queues[static_cast<uint32_t>(QueueOrder::COMPUTE)].queue);
  }
}

void ComputeRenderPass::createDescriptorSets() {
  computeModel = ComputeModel();

  std::vector<DescriptorSetLayout> computeLayout = {
      {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}};

  descriptorSet = DescriptorSet(device, computeLayout);

  std::vector<std::vector<VkBuffer>> tempBuffers;
  std::vector<std::vector<VkDeviceSize>> tempBufferSizes;
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    std::vector<VkBuffer> temp1;
    std::vector<VkDeviceSize> temp2;

    temp1.push_back({computeModel.value().getUniformBuffers()[i].buffer});
    temp2.push_back({sizeof(UniformBufferObject)});
    temp1.push_back(
        {shaderStorageBufferAlloc[(i - 1) % MAX_FRAMES_IN_FLIGHT].buffer});
    temp2.push_back({sizeof(Particle) * PARTICLE_COUNT});
    temp1.push_back({shaderStorageBufferAlloc[i].buffer});
    temp2.push_back({sizeof(Particle) * PARTICLE_COUNT});
    tempBuffers.push_back(temp1);
    tempBufferSizes.push_back(temp2);
  }

  descriptorSet.createDescriptorSets(device, tempBuffers, tempBufferSizes, {},
                                     {});
}

void ComputeRenderPass::createPipeline(VkDescriptorSetLayout setLayout,
                                       VkDevice device,
                                       VkSampleCountFlagBits msaaSamplesrs) {

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &setLayout; // is this right??

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &computePipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline layout!");
  }

  VkShaderModule computeShaderModule;
  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = computePipelineLayout;
  pipelineInfo.stage = createExtraShaderModule(device, computeShaderModule);

  if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                               nullptr, &computePipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline!");
  }

  vkDestroyShaderModule(device, computeShaderModule, nullptr);
}

void ComputeRenderPass::recordComputeCommandBuffer(
    VkCommandBuffer commandBuffer) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error(
        "failed to begin recording compute command buffer!");
  }

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    computePipeline);
  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0,
      1, &descriptorSet.getDescriptorSets()[currentFrame], 0, nullptr);

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

VkSubmitInfo ComputeRenderPass::renderFrame(uint32_t currentFrame) {
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
  return submitInfo;
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
VkPipeline ComputeRenderPass::getPipeline() { return computePipeline; }
VkPipelineLayout ComputeRenderPass::getPipelineLayout() const {
  return computePipelineLayout;
}
void ComputeRenderPass::destroy(VkDevice device, VmaAllocator allocator) {

  for (BaseModel *model : models) {
    model->destroy(device, allocator);
  }
  vkDestroyPipeline(device, computePipeline, nullptr);
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

void ComputeRenderPass::preInit(
    VkDevice device, VkPhysicalDevice physicalDevice,
    VkFormat swapChainImageFormat, VkDescriptorSetLayout setLayout,
    VkExtent2D swapChainExtent, VmaAllocator allocator,
    VkSampleCountFlagBits msaaSamples,
    std::vector<VkImageView> swapChainImageViews) {
  createPipeline(setLayout, device, msaaSamples);
  createShaderStorageBuffers(swapChainExtent.width, swapChainExtent.height,
                             allocator);
  createDescriptorSets();
}
}; // namespace Infinite