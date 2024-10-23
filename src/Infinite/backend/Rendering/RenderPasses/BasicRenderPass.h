#ifndef VULKAN_BASICRENDERPASS_H
#define VULKAN_BASICRENDERPASS_H

#pragma once
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "../../Model/Image/ColorImage.h"
#include "../../Model/Image/DepthImage.h"
#include "RenderPass.h"

namespace Infinite {

class BasicRenderPass : public RenderPass {
private:
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                           uint32_t currentFrame);
  ColorImage *colorImageReasource;
  DepthImage *depthImageReasource;

public:
  void waitForFences() override;
  void resetFences() override;

  void preInit(VkDevice device, VkPhysicalDevice physicalDevice,
               VkFormat swapChainImageFormat, VkExtent2D swapChainExtent,
               VmaAllocator allocator, VkSampleCountFlagBits msaaSamples,
               std::vector<VkImageView> swapChainImageViews) override;

  void destroy(VkDevice device, VmaAllocator allocator) override;

  void createRenderPass(VkDevice device, VkPhysicalDevice physicalDevice,
                        VkFormat swapChainImageFormat,
                        VkSampleCountFlagBits msaaSamples) override;
  void createPipeline(VkDevice device,
                      VkSampleCountFlagBits msaaSamples) override;

  void createDepthAndColorImages(unsigned int width, unsigned int height,
                                 VkFormat colorFormat,
                                 VkPhysicalDevice physicalDevice,
                                 VmaAllocator allocator);

  void createCommandBuffers(VkCommandPool commandPool,
                            VkPhysicalDevice physicalDevice,
                            VkDevice device) override;

  void createSyncObjects(VkDevice device) override;

  void recreateSwapChainWork(
      VmaAllocator allocator, VkDevice device, VkPhysicalDevice physicalDevice,
      VkFormat swapChainImageFormat, VkExtent2D swapChainExtent,
      std::vector<VkImageView> swapChainImageViews) override;

  void destroyDepthAndColorImages(VmaAllocator allocator);

  ColorImage *getColorImageReasource() const;

  DepthImage *getDepthImageReasource() const;

  VkCommandBuffer getCommandBuffer(uint32_t index);
  VkPipelineBindPoint getPipelineType() override;
  void renderFrame(uint32_t currentFrame, uint32_t imageIndex,
                   std::vector<VkSemaphore> prevSemaphores) override;
};

} // namespace Infinite

#endif // VULKAN_BASICRENDERPASS_H
