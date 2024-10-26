#ifndef VULKAN_VULKANUTILS_H
#define VULKAN_VULKANUTILS_H

#pragma once
#include <array>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "../../../libs/vk_mem_alloc.h"

#ifndef VK_MEM_ALLOC

struct BufferAlloc {
  VkBuffer buffer;
  VmaAllocation allocation;
};

struct ImageAlloc {
  VkImage image;
  VmaAllocation allocation;

  void destroy(VmaAllocator allocator) const;
};
#endif

namespace Infinite {
// #ifdef NDEBUG
const static bool enableValidationLayers = false;
// #else
// const static bool enableValidationLayers = true;
// #endif

struct Particle {
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 color;

  // TODO: make not inline

  static inline VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Particle);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static inline std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Particle, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Particle, color);

    return attributeDescriptions;
  }
};

struct ComputeUniformBufferObject {
  float deltaTime = 1.0f;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec2 texCoord;

  Vertex(const glm::vec3 &pos, const glm::vec2 &texCoord)
      : pos(pos), texCoord(texCoord) {}

  static VkVertexInputBindingDescription getBindingDescription();

  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions();

  bool operator==(const Vertex &other) const {
    return pos == other.pos && texCoord == other.texCoord;
  }
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  BufferAlloc &bufferAllocator,
                  VkMemoryPropertyFlags memFlags = 0,
                  VmaAllocationCreateFlags vmaFlags = 0, std::string name = "");

bool hasStencilComponent(VkFormat format);

std::vector<char> readFile(const std::string &filename);

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  inline bool isComplete() const {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

bool isDeviceSuitable(VkPhysicalDevice pDevice);

bool checkDeviceExtensionSupport(VkPhysicalDevice device);

VkCommandBuffer beginSingleTimeCommands(VkDevice device,
                                        VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, VkCommandBuffer commandBuffer,
                           VkCommandPool commandPool, VkQueue queue);

void copyBuffer(BufferAlloc srcBuffer, BufferAlloc dstBuffer, VkDeviceSize size,
                VkQueue queue);

} // namespace Infinite

#endif // VULKAN_VULKANUTILS_H
