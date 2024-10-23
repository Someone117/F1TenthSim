#include "ColorImage.h"
#include "../../../backend/Settings.h"

namespace Infinite {

void ColorImage::create(unsigned int width, unsigned int height,
                        VkFormat colorFormat, VkPhysicalDevice physicalDevice,
                        VmaAllocator allocator) {

  createImage(width, height, 1, colorFormat, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              _image, msaaSamples);


  _image_view =
      createImageView(_image.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}
} // namespace Infinite