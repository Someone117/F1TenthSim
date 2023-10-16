#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H
#include <cstdint>
#pragma once

#include "../../Model/Image/ColorImage.h"
#include "../../Model/Image/DepthImage.h"
#include "../CommandBuffer.h"

namespace Infinite {

class Model;

class ColorImage;

class DepthImage;

extern std::vector<VkSemaphore> imageAvailableSemaphores;
extern std::vector<VkSemaphore> renderFinishedSemaphores;
extern std::vector<VkFence> inFlightFences;
extern VkQueue presentQueue;

class RenderPass {
  friend void recordCommandBuffer(VkCommandBuffer commandBuffer,
                                  uint32_t imageIndex, RenderPass &renderPass);

  friend void renderFrame();

public:
  void destroyDepthAndColorImages(VmaAllocator allocator);

  const std::vector<Model *> &getModels() const;

  virtual void createRenderPass(VkDevice device,
                                VkPhysicalDevice physicalDevice,
                                VkFormat swapChainImageFormat,
                                VkSampleCountFlagBits msaaSamples);

  void createCommandBuffers(VkCommandPool commandPool,
                            VkPhysicalDevice physicalDevice, VkDevice device,
                            QueueFamilyIndices queueFamilyIndices);

  static void createSyncObjects(VkDevice device, int maxFramesInFlight);

  uint32_t getIndex() const;

  void setIndex(uint32_t index);

  void addModel(Model *model);

  void createGraphicsPipeline(VkDescriptorSetLayout setLayout, VkDevice device,
                              VkSampleCountFlagBits msaaSamples);

  void createDepthAndColorImages(unsigned int width, unsigned int height,
                                 VkFormat colorFormat,
                                 VkPhysicalDevice physicalDevice,
                                 VmaAllocator allocator);

  ColorImage *getColorImageReasource() const;

  DepthImage *getDepthImageReasource() const;

  void destroy(VkDevice device, VmaAllocator allocator, int maxFramesInFlight);

  VkRenderPass_T *getRenderPass();

  VkPipeline getGraphicsPipeline() const;

  VkPipelineLayout getPipelineLayout() const;

  VkCommandBuffer getCommandBuffer(uint32_t index);
  uint32_t getIndex();

protected:
  VkRenderPass renderPass;

  ColorImage *colorImageReasource;
  DepthImage *depthImageReasource;

  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  VkCommandPool commandPool;

  std::vector<Model *> models;

  uint32_t index;

  friend struct CommandBuffer;

  friend class Image;

  CommandBuffer commandBufferManager;
};
VkShaderModule createShaderModule(const std::vector<char> &code,
                                  VkDevice device);

} // namespace Infinite

#endif // VULKAN_RENDERPASS_H