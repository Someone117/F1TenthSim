#ifndef VULKAN_BASE_MODEL_H
#define VULKAN_BASE_MODEL_H

#include "DescriptorSet.h"
#include <cstdint>

#pragma once

#include "../../../frontend/Camera.h"
#include "../Image/TexturedImage.h"

namespace Infinite {

class BaseModel {
protected:
  glm::vec3 position{};
  glm::vec3 scale{};

  float xAngle{};
  float yAngle{};
  float zAngle{};

  std::vector<BufferAlloc> uniformBuffers;

  std::string name;

  void loadModel(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices,
                 const std::string &model_path);

  std::vector<BufferAlloc>
  createUniformBuffer(VkDeviceSize bufferSize = sizeof(UniformBufferObject));

public:
  BaseModel(std::string _name);

  virtual const BufferAlloc &getVertexBuffer() const = 0;
  virtual const BufferAlloc &getIndexBuffer() const = 0;
  virtual DescriptorSet getDescriptorSet() const = 0;
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

  void updateUniformBuffer(uint32_t index, Camera camera,
                           VmaAllocator allocator, unsigned int width,
                           unsigned int height);

  const std::vector<BufferAlloc> &getUniformBuffers() const;

  void destroy(VkDevice device, VmaAllocator allocator);

  void createVertexBuffer(BufferAlloc &vertexBuffer,
                          std::vector<Vertex> vertices, VmaAllocator allocator,
                          VkQueue queue);
  void createIndexBuffer(BufferAlloc &indexBuffer, std::vector<uint32_t> vertices,
                         VmaAllocator allocator, VkQueue queue);
};
} // namespace Infinite

#endif // VULKAN_MODEL_H
