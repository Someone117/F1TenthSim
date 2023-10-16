#include "VulkanUtils.h"
#include "../Infinite.h"
#include <fstream>
#include <set>

namespace Infinite {

void ImageAlloc::destroy(VmaAllocator allocator) const {
  vmaDestroyImage(allocator, image, allocation);
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  BufferAlloc &bufferAllocator, VkMemoryPropertyFlags memFlags,
                  VmaAllocationCreateFlags vmaFlags) {

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.requiredFlags = memFlags;
  allocInfo.flags = vmaFlags;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
                      &bufferAllocator.buffer, &bufferAllocator.allocation,
                      nullptr) != VK_SUCCESS) {
    throw std::runtime_error("failed to create vma buffer!");
  }
}

bool hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         format == VK_FORMAT_D24_UNORM_S8_UINT;
}

std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file: " + filename);
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);

  if (!file.read(buffer.data(), fileSize)) {
    throw std::runtime_error("failed to read from file: " + filename);
  }

  file.close();

  return buffer;
}

std::array<VkVertexInputAttributeDescription, 2>
Vertex::getAttributeDescriptions() {
  std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, pos);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

  return attributeDescriptions;
}

VkVertexInputBindingDescription Vertex::getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return bindingDescription;
}

} // namespace Infinite
