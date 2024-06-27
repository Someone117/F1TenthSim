#ifndef COMPUTE_RENDER_PASS_H
#define COMPUTE_RENDER_PASS_H

#pragma once
#include "../../../util/VulkanUtils.h"
#include "../../Model/Models/ComputeModel.h"
#include "../../Model/Models/DescriptorSet.h"
#include "RenderPass.h"
#include <glm/glm.hpp>

namespace Infinite {
struct Particle {
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 color;
};

class ComputeRenderPass : public RenderPass {
private:
  std::vector<BufferAlloc> shaderStorageBufferAlloc;
  DescriptorSet descriptorSet;
  VkPipelineLayout computePipelineLayout;
  VkPipeline computePipeline;

  std::optional<ComputeModel> computeModel;

  void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);

public:
  VkPipelineShaderStageCreateInfo
  createExtraShaderModule(VkDevice device, VkShaderModule &computeShaderModule);
  // on Compute space
  // https://vulkan-tutorial.com/Compute_Shader

  void createShaderStorageBuffers(uint32_t WIDTH, uint32_t HEIGHT,
                                  VmaAllocator allocator);

  void createDescriptorSets();

  void createPipeline(VkDescriptorSetLayout setLayout, VkDevice device,
                      VkSampleCountFlagBits msaaSamples) override;

  VkSubmitInfo renderFrame(uint32_t currentFrame) override;

  void waitForFences() override;
  void resetFences() override;

  VkPipelineBindPoint getPipelineType() override;
  VkPipeline getPipeline() override;
  void createCommandBuffers(VkCommandPool commandPool,
                            VkPhysicalDevice physicalDevice,
                            VkDevice device) override;
  void createSyncObjects(VkDevice device) override;

  VkPipelineLayout getPipelineLayout() const override;
  void destroy(VkDevice device, VmaAllocator allocator);

  void recreateSwapChainWork(
      VmaAllocator allocator, VkDevice device, VkPhysicalDevice physicalDevice,
      VkFormat swapChainImageFormat, VkExtent2D swapChainExtent,
      std::vector<VkImageView> swapChainImageViews) override;
  void createRenderPass(VkDevice device, VkPhysicalDevice physicalDevice,
                        VkFormat swapChainImageFormat,
                        VkSampleCountFlagBits msaaSamples) override;

  void preInit(
      VkDevice device, VkPhysicalDevice physicalDevice,
      VkFormat swapChainImageFormat, VkDescriptorSetLayout setLayout,
      VkExtent2D swapChainExtent, VmaAllocator allocator,
      VkSampleCountFlagBits msaaSamples,
      std::vector<VkImageView> swapChainImageViews) override;
};

} // namespace Infinite

#endif // COMPUTE_RENDER_PASS_H