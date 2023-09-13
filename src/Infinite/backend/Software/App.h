#ifndef VULKAN_APP_H
#define VULKAN_APP_H

#pragma once

#include "../../util/Includes.h"


namespace Infinite {
    class App {
    public:
        const char *name;
        uint32_t version;
        inline static std::vector<const char *> deviceExtensions = {"VK_KHR_swapchain"};

        static void addExtensions(const char *extension);

        App(const char *name, uint32_t major, uint32_t minor, uint32_t patch);
    };

} // Infinite

#endif //VULKAN_APP_H
