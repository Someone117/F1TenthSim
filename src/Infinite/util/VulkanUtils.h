#ifndef VULKAN_VULKANUTILS_H
#define VULKAN_VULKANUTILS_H

#pragma once

#include "Includes.h"
#include <array>
#include <optional>


namespace Infinite {
#ifdef NDEBUG
const static bool enableValidationLayers = false;
#else
const static bool enableValidationLayers = true;
#endif

struct BufferAlloc {
  VkBuffer buffer;
  VmaAllocation allocation;
};

struct ImageAlloc {
  VkImage image;
  VmaAllocation allocation;

  void destroy(VmaAllocator allocator) const;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec2 texCoord;

  Vertex(const glm::vec3 &pos, const glm::vec2 &texCoord)
      : pos(pos), texCoord(texCoord) {}

  static VkVertexInputBindingDescription getBindingDescription();

  static std::array<VkVertexInputAttributeDescription, 2>
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
                  VmaAllocationCreateFlags vmaFlags = 0);

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

} // namespace Infinite

#endif // VULKAN_VULKANUTILS_H
