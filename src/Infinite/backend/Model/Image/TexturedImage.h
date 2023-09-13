#ifndef VULKAN_TEXTUREDIMAGE_H
#define VULKAN_TEXTUREDIMAGE_H

#pragma once


#include "Image.h"

#include <utility>
#include "sbt_image.h"

namespace Infinite {

    class TexturedImage : public Image {
    private:
        VkSampler _sampler;
        const char *_filePath;

        void createSampler();

    public:
        explicit TexturedImage(const char *filePath) : _filePath(filePath) {}

        void create() override;

        void destroy(VmaAllocator allocator) override;

        static void generateMipmaps(Image *image, VkFormat format, int width, int height, uint32_t levels);

        VkSampler *getSampler();

    };

} // Infinite

#endif //VULKAN_TEXTUREDIMAGE_H
