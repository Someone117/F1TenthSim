#include "App.h"
#include <vulkan/vulkan_core.h>

namespace Infinite {

    App::App(const char *name, uint32_t major, uint32_t minor, uint32_t patch) : name(name),
                                                                                 version(VK_MAKE_VERSION(major, minor,
                                                                                                         patch)) {}

    void App::addExtensions(const char *extension) {
        deviceExtensions.push_back(extension);
    }

} // Infinite