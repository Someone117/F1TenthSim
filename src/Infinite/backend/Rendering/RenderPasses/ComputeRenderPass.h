#ifndef COMPUTE_RENDER_PASS_H
#define COMPUTE_RENDER_PASS_H

#pragma once
#include "../../../util/VulkanUtils.h"
#include "../../Model/Models/ComputeModel.h"
#include "RenderPass.h"
#include <glm/glm.hpp>

namespace Infinite {

class ComputeRenderPass : public RenderPass {
private:
  std::optional<ComputeModel> computeModel;

  void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);

public:
  void createShaderStorageBuffers(uint32_t WIDTH, uint32_t HEIGHT,
                                  VmaAllocator allocator);

  void createDescriptorSets(VkDescriptorSetLayout *descriptorSetLayout);

  void createPipeline(VkDevice device,
                      VkSampleCountFlagBits msaaSamples) override;

  void renderFrame(uint32_t currentFrame, uint32_t imageIndex,
                   std::vector<VkSemaphore> prevSemaphores) override;

  void waitForFences() override;
  void resetFences() override;

  VkPipelineBindPoint getPipelineType() override;
  void createCommandBuffers(VkCommandPool commandPool,
                            VkPhysicalDevice physicalDevice,
                            VkDevice device) override;
  void createSyncObjects(VkDevice device) override;

  void destroy(VkDevice device, VmaAllocator allocator) override;


  void recreateSwapChainWork(
      VmaAllocator allocator, VkDevice device, VkPhysicalDevice physicalDevice,
      VkFormat swapChainImageFormat, VkExtent2D swapChainExtent,
      std::vector<VkImageView> swapChainImageViews) override;
  void createRenderPass(VkDevice device, VkPhysicalDevice physicalDevice,
                        VkFormat swapChainImageFormat,
                        VkSampleCountFlagBits msaaSamples) override;

  void preInit(VkDevice device, VkPhysicalDevice physicalDevice,
               VkFormat swapChainImageFormat, VkExtent2D swapChainExtent,
               VmaAllocator allocator, VkSampleCountFlagBits msaaSamples,
               std::vector<VkImageView> swapChainImageViews) override;
};

} // namespace Infinite

#endif // COMPUTE_RENDER_PASS_H