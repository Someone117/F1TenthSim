#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#pragma once

#include "../../../util/VulkanUtils.h"
#include <utility>

namespace Infinite {

class Image {
protected:
  VkImageView _image_view;

  static void copyBufferToImage(BufferAlloc buffer, Image *image,
                                uint32_t width, uint32_t height);

  ImageAlloc _image;

  static std::pair<std::pair<VkAccessFlags, VkAccessFlags>,
                   std::pair<VkPipelineStageFlags, VkPipelineStageFlags>>
  chooseMasks(VkImageLayout oldLayout, VkImageLayout newLayout);

public:
  static void transitionImageLayout(Image *image, VkFormat format,
                                    VkImageLayout oldLayout,
                                    VkImageLayout newLayout,
                                    uint32_t mipLevels = 1);

  virtual void create(unsigned int width, unsigned int height,
                      VkFormat colorFormat, VkPhysicalDevice physicalDevice,
                      VmaAllocator allocator);
  static void
  createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
              VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
              ImageAlloc &image,
              VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT,
              VmaAllocationCreateFlags memUsage = VMA_MEMORY_USAGE_UNKNOWN);

  static void addBarrier(Image *image, VkFormat format, VkImageLayout oldLayout,
                         VkImageLayout newLayout, uint32_t mipLevels,
                         VkCommandBuffer commandBuffer);
  static void addBarrier(VkImage *image, VkFormat format,
                         VkImageLayout oldLayout, VkImageLayout newLayout,
                         uint32_t mipLevels, VkCommandBuffer commandBuffer);

  VkImageView *getImageView();

  ImageAlloc &getImage();

  friend class SwapChain;

  virtual void destroy(VmaAllocator allocator);

  static VkImageView
  createImageView(VkImage image, VkFormat format, uint32_t mipLevels,
                  VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
};

} // namespace Infinite

#endif // VULKAN_IMAGE_H
