#ifndef VULKAN_MODEL_H
#define VULKAN_MODEL_H

#include "BaseModel.h"

#pragma once

#include "../Image/TexturedImage.h"

namespace Infinite {

class Model : public BaseModel {
private:
  TexturedImage texture;
  BufferAlloc vertexBuffer{};
  BufferAlloc indexBuffer{};
  uint32_t indexCount;

public:
  void createDescriptorSets(VkDevice device,
                            VkDescriptorSetLayout *descriptorSetLayout,
                            ShaderLayout *shaderLayout) override;

  uint32_t getIndexCount() const override;

  Model(std::string _name, std::string model_path, const char *texture_path,
        VkDevice device, VkPhysicalDevice physicalDevice,
        VmaAllocator allocator, unsigned int width, unsigned int height,
        VkFormat colorFormat);

  const BufferAlloc &getVertexBuffer() const override;

  const BufferAlloc &getIndexBuffer() const override;

  void destroy(VkDevice device, VmaAllocator allocator) override;
};
} // namespace Infinite

#endif // VULKAN_MODEL_H
