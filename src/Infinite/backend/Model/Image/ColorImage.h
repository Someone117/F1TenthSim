#ifndef VULKAN_COLORIMAGE_H
#define VULKAN_COLORIMAGE_H

#pragma once


#include "Image.h"
#include "../../Rendering/Engine.h"

namespace Infinite {

    class ColorImage : public Image {
    public:
        void create() override;
    };

} // Infinite

#endif //VULKAN_COLORIMAGE_H
