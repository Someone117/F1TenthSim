#include "DepthImage.h"
#include "../../Infinite.h"

namespace Infinite {
    VkFormat DepthImage::findDepthFormat() {
        return findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }
    void DepthImage::create() {
        VkFormat depthFormat = findDepthFormat();

        createImage(Engine::getEngine().getWindowWidth(), Engine::getEngine().getWindowHeight(), 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, _image, Engine::getEngine().settings.msaaSamples);

        _image_view = createImageView(_image.image, depthFormat,1 , VK_IMAGE_ASPECT_DEPTH_BIT);

        transitionImageLayout(this, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }


} // Infinite