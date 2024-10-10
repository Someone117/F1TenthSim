#include "RenderPass.h"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Infinite {
std::vector<VkSemaphore> imageAvailableSemaphores = {};

// TODO: change polygonMode for the rasterizer for the drawing of outlines.
// TODO: change depthClampEnable to VK_TRUE and enable the GPU feature to enable
// depth clamp
// TODO: FIX THE FREAKING FILE PATH

uint32_t RenderPass::getIndex() const { return index; }

void RenderPass::setIndex(uint32_t index) { RenderPass::index = index; }

void RenderPass::destroy(VkDevice device, VmaAllocator allocator) {
  for (BaseModel *model : models) {
    model->destroy(device, allocator);
  }
}

void RenderPass::addModel(BaseModel *model) { models.push_back(model); }

const std::vector<BaseModel *> &RenderPass::getModels() const { return models; }

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

uint32_t RenderPass::getIndex() { return index; }
std::vector<VkFence> RenderPass::getInFlighFences() { return inFlightFences; }
std::vector<VkSemaphore> RenderPass::getFinishedSemaphores() {
  return finishedSemaphores;
}

} // namespace Infinite