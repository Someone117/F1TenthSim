#ifndef VULKAN_BASE_MODEL_H
#define VULKAN_BASE_MODEL_H

#ifndef MAX_MODELS
#define MAX_MODELS 128
#endif

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif

#ifndef TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#endif

#include "DescriptorSet.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>
#pragma once

#include "../../../frontend/Camera.h"
#include "../../../util/VulkanUtils.h"
#include "../../Model/Image/TexturedImage.h"

namespace Infinite {

class BaseModel {
protected:
  bool isComplete;

  glm::vec3 position{};
  glm::vec3 scale{};

  float xAngle{};
  float yAngle{};
  float zAngle{};

  std::vector<BufferAlloc> uniformBuffers;
  DescriptorSet descriptorSet;

  std::string name;

  void loadModel(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices,
                 const std::string &model_path);

  std::vector<BufferAlloc>
  createUniformBuffer(VkDeviceSize bufferSize = sizeof(UniformBufferObject));

public:
  BaseModel(std::string _name);

  inline bool finishedInit() { return isComplete; }

  virtual void createDescriptorSets(VkDevice device,
                                    VkDescriptorSetLayout *descriptorSetLayout,
                                    ShaderLayout *shaderLayout) = 0;

  virtual const BufferAlloc &getVertexBuffer() const = 0;
  virtual const BufferAlloc &getIndexBuffer() const = 0;

  inline DescriptorSet getDescriptorSet() { return descriptorSet; };
  virtual uint32_t getIndexCount() const = 0;

  void setScale(glm::vec3 newScale);

  const glm::vec3 &getPosition() const;

  const glm::vec3 &getScale() const;

  float getXAngle() const;

  float getYAngle() const;

  float getZAngle() const;

  const std::string &getName() const;

  glm::mat4 getMatrix();

  void translate(glm::vec3 vector);

  void rotate(glm::vec3 radians);

  BufferAlloc getUniformBuffer(uint32_t currentImage);

  virtual void updateUniformBuffer(uint32_t index, Camera camera,
                                   VmaAllocator allocator, unsigned int width,
                                   unsigned int height);

  const std::vector<BufferAlloc> &getUniformBuffers() const;

  virtual void destroy(VkDevice device, VmaAllocator allocator);

  void createVertexBuffer(BufferAlloc &vertexBuffer,
                          std::vector<Vertex> vertices, VmaAllocator allocator,
                          VkQueue queue);
  void createIndexBuffer(BufferAlloc &indexBuffer,
                         std::vector<uint32_t> vertices, VmaAllocator allocator,
                         VkQueue queue);
};
} // namespace Infinite

#endif // VULKAN_MODEL_H
