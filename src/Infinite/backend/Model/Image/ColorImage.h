#ifndef VULKAN_COLORIMAGE_H
#define VULKAN_COLORIMAGE_H

#pragma once

#include "Image.h"

namespace Infinite {

class ColorImage : public Image {
public:
  void create(unsigned int width, unsigned int height, VkFormat colorFormat,
              VkPhysicalDevice physicalDevice, VmaAllocator allocator) override;
};

} // namespace Infinite

#endif // VULKAN_COLORIMAGE_H
