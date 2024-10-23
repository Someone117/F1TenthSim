#ifndef VULKAN_DEPTHIMAGE_H
#define VULKAN_DEPTHIMAGE_H

#pragma once

#include "Image.h"
#include <stdexcept>
#include <vulkan/vulkan_core.h>


namespace Infinite {

class DepthImage : public Image {
public:
  void create(unsigned int width, unsigned int height, VkFormat colorFormat,
              VkPhysicalDevice physicalDevice, VmaAllocator allocator) override;

  static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

private:
  static VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                                      VkImageTiling tiling,
                                      VkFormatFeatureFlags features,
                                      VkPhysicalDevice physicalDevice);
};

} // namespace Infinite

#endif // VULKAN_DEPTHIMAGE_H
