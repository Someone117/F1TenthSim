#pragma once

#include "ColorImage.h"
#include "../../../Infinite.h"

namespace Infinite {

    void ColorImage::create() {
        VkFormat colorFormat = Engine::getEngine().getSwapChainImageFormat();

        createImage(Engine::getEngine().getWindowWidth(),
                    Engine::getEngine().getWindowWidth(), 1, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, _image,
                    msaaSamples);

        _image_view = createImageView(_image.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
} // Infinite