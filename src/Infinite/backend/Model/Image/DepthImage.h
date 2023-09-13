#ifndef VULKAN_DEPTHIMAGE_H
#define VULKAN_DEPTHIMAGE_H

#pragma once

#include "Image.h"

namespace Infinite {

    class DepthImage : public Image {
    public:

        void create() override;

        static VkFormat findDepthFormat();

    private:

        static VkFormat
        findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                            VkFormatFeatureFlags features);

    };

} // Infinite

#endif //VULKAN_DEPTHIMAGE_H
