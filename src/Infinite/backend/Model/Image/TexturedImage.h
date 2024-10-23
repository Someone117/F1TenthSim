#ifndef VULKAN_TEXTUREDIMAGE_H
#define VULKAN_TEXTUREDIMAGE_H

#pragma once

#include "Image.h"


namespace Infinite {

class TexturedImage : public Image {
private:
  VkSampler _sampler;
  const char *_filePath;

  void createSampler();

public:
  explicit TexturedImage(const char *filePath) : _filePath(filePath) {}

  explicit TexturedImage() : _filePath(NULL) {}

  void create(unsigned int width, unsigned int height, VkFormat colorFormat,
              VkPhysicalDevice physicalDevice, VmaAllocator allocator) override;

  void destroy(VmaAllocator allocator) override;

  static void generateMipmaps(Image *image, VkFormat format, int width,
                              int height, uint32_t levels);

  VkSampler *getSampler();
};

} // namespace Infinite

#endif // VULKAN_TEXTUREDIMAGE_H
