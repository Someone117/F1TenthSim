//
// Created by Someo on 4/8/2023.
//

#include "App.h"

namespace Infinite {

    App::App(const char *name, uint32_t major, uint32_t minor, uint32_t patch) : name(name), version(VK_MAKE_VERSION(major, minor, patch)) {}

} // Infinite