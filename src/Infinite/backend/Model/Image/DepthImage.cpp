#include "DepthImage.h"
#include "../../Settings.h"

namespace Infinite {
VkFormat DepthImage::findDepthFormat(VkPhysicalDevice physicalDevice) {
  return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
      physicalDevice);
}

VkFormat DepthImage::findSupportedFormat(
    const std::vector<VkFormat> &candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("failed to find supported format!");
}

void DepthImage::create(unsigned int width, unsigned int height,
                        VkFormat colorFormat, VkPhysicalDevice physicalDevice,
                        VmaAllocator allocator) {
  VkFormat depthFormat = findDepthFormat(physicalDevice);

  createImage(width, height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, _image, msaaSamples);

  _image_view =
      createImageView(_image.image, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

  transitionImageLayout(this, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

} // namespace Infinite