#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
#pragma once

#include "../../Model/Models/BaseModel.h"
#include "../../Software/ShaderAnalyzer.h"
#include "../CommandBuffer.h"

namespace Infinite {
// class ColorImage;
// class DepthImage;
extern std::vector<VkSemaphore> imageAvailableSemaphores;
extern VkQueue presentQueue;

struct DescriptorSetLayoutData {
  uint32_t set_number;
  VkDescriptorSetLayoutCreateInfo create_info;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
};

enum RenderPassTypes {
  BASIC_RENDER_PASS = 0,
  COMPUTE_RENDER_PASS = 1,
  COUNT = 2
};

class RenderPass {

public:
  virtual void waitForFences() = 0;
  virtual void resetFences() = 0;

  virtual void renderFrame(uint32_t currentFrame, uint32_t imageIndex,
                           std::vector<VkSemaphore> prevSemaphores) = 0;

  const std::vector<BaseModel *> &getModels() const;

  virtual void createSyncObjects(VkDevice device) = 0;

  virtual VkPipelineBindPoint getPipelineType() = 0;
  inline VkPipeline getPipeline() { return renderPipeline; };
  inline VkPipelineLayout getPipelineLayout() const { return pipelineLayout; };

  virtual void recreateSwapChainWork(
      VmaAllocator allocator, VkDevice device, VkPhysicalDevice physicalDevice,
      VkFormat swapChainImageFormat, VkExtent2D swapChainExtent,
      std::vector<VkImageView> swapChainImageViews) = 0;

  virtual void createRenderPass(VkDevice device,
                                VkPhysicalDevice physicalDevice,
                                VkFormat swapChainImageFormat,
                                VkSampleCountFlagBits msaaSamples) = 0;

  virtual void createCommandBuffers(VkCommandPool commandPool,
                                    VkPhysicalDevice physicalDevice,
                                    VkDevice device) = 0;

  uint32_t getIndex() const;
  uint32_t type;

  void setIndex(uint32_t index);

  void addModel(BaseModel *model);

  virtual void createPipeline(VkDevice device,
                              VkSampleCountFlagBits msaaSamples) = 0;

  void virtual destroy(VkDevice device, VmaAllocator allocator) = 0;

  VkRenderPass_T *getRenderPass();

  uint32_t getIndex();

  virtual void preInit(VkDevice device, VkPhysicalDevice physicalDevice,
                       VkFormat swapChainImageFormat,
                       VkExtent2D swapChainExtent, VmaAllocator allocator,
                       VkSampleCountFlagBits msaaSamples,
                       std::vector<VkImageView> swapChainImageViews) = 0;

  std::vector<VkFence> getInFlighFences();
  std::vector<VkSemaphore> getFinishedSemaphores();

  // make protected
  CommandBuffer commandBufferManager;

protected:
  VkPipeline renderPipeline;
  std::vector<VkFence> inFlightFences;
  std::vector<VkSemaphore> finishedSemaphores;
  VkDescriptorSetLayout descriptorSetLayout;
  ShaderLayout shaderLayout;

  VkRenderPass renderPass;

  VkPipelineLayout pipelineLayout;

  VkCommandPool commandPool;

  std::vector<BaseModel *> models;

  uint32_t index;

  friend struct CommandBuffer;

  friend class Image;
};
VkShaderModule createShaderModule(const std::vector<char> &code,
                                  VkDevice device);

} // namespace Infinite

#endif // VULKAN_RENDERPASS_H