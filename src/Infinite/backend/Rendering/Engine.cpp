#include "Engine.h"
#include "../Model/Models/Model.h"
#include "RenderPasses/RenderPass.h"
#include <cstdint>
#include <cstring>
#include <set>

namespace Infinite {

void copyBuffer(BufferAlloc srcBuffer, BufferAlloc dstBuffer,
                VkDeviceSize size) {
  VkCommandBuffer commandBuffer =
      Engine::getEngine().beginSingleTimeCommands(imagePool);

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer.buffer, dstBuffer.buffer, 1,
                  &copyRegion);

  Engine::getEngine().endSingleTimeCommands(commandBuffer, imagePool);
}

std::vector<const char *> getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

// TODO: implement
void Engine::cleanUpInfinite() {
  cleanupSwapChain();

  //        camera.~Camera(); // done

  //        vkDestroySampler(device, textureSampler, nullptr);
  //        vkDestroyImageView(device, textureImageView, nullptr);
  //        vmaDestroyImage(allocator, textureImage.image,
  //        textureImage.allocation);
  //
  //        vkDestroyPipeline(device, graphicsPipeline, nullptr);
  //        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  //        vkDestroyRenderPass(device, renderPass, nullptr);
  //
  //        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
  //            vmaDestroyBuffer(allocator, uniformBuffers[i].buffer,
  //            uniformBuffers[i].allocation);
  //        }
  //
  //        vkDestroyDescriptorPool(Engine::getEngine().getInstance(),
  //        descriptorPool, nullptr);
  //
  //        vkDestroyDescriptorSetLayout(Engine::getEngine().getInstance(),
  //        descriptorSetLayout, nullptr);
  //
  //
  //        vmaDestroyBuffer(allocator, indexBuffer.buffer,
  //        indexBuffer.allocation);
  //
  //        vmaDestroyBuffer(allocator, vertexBuffer.buffer,
  //        vertexBuffer.allocation);
  //
  //        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
  //            vkDestroySemaphore(Engine::getEngine().getInstance(),
  //            renderFinishedSemaphores[i], nullptr);
  //            vkDestroySemaphore(Engine::getEngine().getInstance(),
  //            imageAvailableSemaphores[i], nullptr);
  //            vkDestroyFence(Engine::getEngine().getInstance(),
  //            inFlightFences[i], nullptr);
  //        }
  //
  //        vkDestroyCommandPool(Engine::getEngine().getInstance(), commandPool,
  //        nullptr);
  // all done
  DescriptorSet::cleanUpDescriptors(device);

  vkDestroyCommandPool(device, imagePool, nullptr);

  vmaDestroyAllocator(allocator);

  vkDestroyDevice(device, nullptr);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);

  glfwTerminate();
}

void Engine::createEngine() {
  initWindow(); // err
  createInstance();
  setupDebugMessenger();
  createSurface();
  pickDevices();

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
  allocatorInfo.physicalDevice = physicalDevice;
  allocatorInfo.device = device;
  allocatorInfo.instance = instance;

  vmaCreateAllocator(&allocatorInfo, &allocator);

  createSwapChain();

  createImageViews();

  /*
  InitVulkan
      createInstance();
      setupDebugMessenger();
      createSurface();
      pickPhysicalDevice();
      createLogicalDevice();
      createSwapChain();
      createImageViews();
      createRenderPass();
      createDescriptorSetLayout();
      createGraphicsPipeline();
      createCommandPool();
      createColorResources();
      createDepthResources();
      createFramebuffers();
      createDescriptorPool();


  Model constructor:
      texture.create():
          createTextureImage();
          createTextureImageView();
          createTextureSampler();
      loadModel();
      createVertexBuffer();
      createIndexBuffer();
      createUniformBuffers();
      createDescriptorSets();

  finishInit
      createCommandBuffers();
      createSyncObjects();
   */
}

void Engine::createInstance() {
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = app.name;
  appInfo.applicationVersion = app.version;
  appInfo.pEngineName = "Infinite Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size()); // check
    createInfo.ppEnabledLayerNames = validationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;

    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }
}

// ToDo: change
void Engine::pickPhysicalDevice() {
  uint32_t deviceCount = 0;

  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  // err
  for (const auto &pDeviceT : devices) {
    if (isDeviceSuitable(pDeviceT)) {
      physicalDevice = pDeviceT;
      msaaSamples = getMaxUsableSampleCount();
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

void Engine::createLogicalDevice() {

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  uint32_t uniqueQueueFamilies[static_cast<int>(QueueOrder::Count)];
  for (uint32_t i = 0; i < static_cast<int>(QueueOrder::Count); i++) {
    uniqueQueueFamilies[i] = queues[i].getIndex().index.value();
  }

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;
  deviceFeatures.sampleRateShading =
      VK_TRUE; // enable sample shading feature for the device

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(App::deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = App::deviceExtensions.data();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }
  for (auto &queue : queues) {
    queue.createQueue(device);
  }
}

void Engine::pickDevices() {
  pickPhysicalDevice();
  createLogicalDevice();
}

VkSampleCountFlagBits Engine::getMaxUsableSampleCount() {
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

  VkSampleCountFlags counts =
      physicalDeviceProperties.limits.framebufferColorSampleCounts &
      physicalDeviceProperties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }
  return VK_SAMPLE_COUNT_1_BIT;
}

Engine &Engine::getEngine() {
  static Engine instance; // Guaranteed to be destroyed.
  // Instantiated on first use.
  return instance;
}

GLFWwindow *Engine::getWindow() { return window; }

uint32_t Engine::getCurrentFrame() const { return currentFrame; }

void Engine::setCurrentFrame(uint32_t currentFrame) {
  Engine::currentFrame = currentFrame;
}

void Engine::setApp(App _app) { app = _app; }

// Swap Chain
void Engine::cleanupSwapChain() {
  for (const std::vector<VkFramebuffer> &swapChainFramebufferVector :
       swapChainFramebuffers) {
    for (VkFramebuffer swapChainFramebuffer : swapChainFramebufferVector) {
      vkDestroyFramebuffer(device, swapChainFramebuffer, nullptr);
    }
  }

  for (VkImageView swapChainImageView : swapChainImageViews) {
    vkDestroyImageView(device, swapChainImageView, nullptr);
  }

  vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void Engine::recreateSwapChain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device);

  cleanupSwapChain();

  createSwapChain();
  createImageViews();
}

void Engine::createSwapChain() {
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(physicalDevice);

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  uint32_t queueFamilyIndices[static_cast<int>(QueueOrder::Count)];
  for (uint32_t i = 0; i < static_cast<int>(QueueOrder::Count); i++) {
    queueFamilyIndices[i] = queues[i].getIndex().index.value();
  }

  if (queues[static_cast<int>(QueueOrder::GRAPHICS)].getIndex().index !=
      queues[static_cast<int>(QueueOrder::PRESENT)].getIndex().index) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;     // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);

  vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                          swapChainImages.data());

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
}

SwapChainSupportDetails Engine::querySwapChainSupport(VkPhysicalDevice device) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, Engine::getEngine().surface,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, Engine::getEngine().surface,
                                       &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, Engine::getEngine().surface,
                                         &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, Engine::getEngine().surface,
                                            &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, Engine::getEngine().surface, &presentModeCount,
        details.presentModes.data());
  }

  return details;
}

VkExtent2D
Engine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

// ToDo: change
VkPresentModeKHR Engine::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == PRESENT_MODE) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

void Engine::createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); i++)
    swapChainImageViews[i] =
        Image::createImageView(swapChainImages[i], swapChainImageFormat, 1);
}

void Engine::createFramebuffers(RenderPass &renderPass, uint32_t renderPasses) {
  swapChainFramebuffers.resize(renderPasses);
  for (auto &swapChainFramebuffer : swapChainFramebuffers) {
    swapChainFramebuffer.resize(swapChainImageViews.size());
  }

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    std::array<VkImageView, 3> attachments = {
        *renderPass.getColorImageReasource()->getImageView(),
        *renderPass.getDepthImageReasource()->getImageView(),
        swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass.getRenderPass();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &swapChainFramebuffers[renderPass.getIndex()][i]) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

unsigned int Engine::getWindowWidth() { return swapChainExtent.width; }

unsigned int Engine::getWindowHeight() { return swapChainExtent.height; }

// Window
void Engine::initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);

  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Engine::framebufferResizeCallback(GLFWwindow *window, int width,
                                       int height) {
  Engine::getEngine().framebufferResized = true;
}

void Engine::createSurface() {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

VkSurfaceFormatKHR Engine::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

// Logger
VKAPI_ATTR VkBool32 VKAPI_CALL
Engine::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                      void *pUserData) {
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  }
  return VK_FALSE;
}

void Engine::DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

VkResult Engine::CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

bool Engine::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      return false;
    }
  }

  return true;
}

void Engine::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);

  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                   &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

void Engine::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
}

VkFormat Engine::getSwapChainImageFormat() const {
  return swapChainImageFormat;
}

VkCommandBuffer Engine::beginSingleTimeCommands(VkCommandPool commandPool) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void Engine::endSingleTimeCommands(VkCommandBuffer commandBuffer,
                                   VkCommandPool commandPool) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  queues[static_cast<uint32_t>(QueueOrder::GRAPHICS)].submit(submitInfo);
  queues[static_cast<uint32_t>(QueueOrder::GRAPHICS)].waitIdle();

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

bool Engine::isDeviceSuitable(VkPhysicalDevice pDevice) {
  findQueueFamilies(physicalDevice, queues);

  bool extensionsSupported = checkDeviceExtensionSupport(pDevice);

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(pDevice, &supportedFeatures);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(pDevice);
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  bool allIndicesFound = true;
    for (uint32_t k = 0; k < static_cast<uint32_t>(QueueOrder::Count); k++) {
      if (!queues[k].getIndex().index.has_value()) {
        allIndicesFound = false;
        break;
      }
    }

  return allIndicesFound && extensionsSupported &&
         swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool Engine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(App::deviceExtensions.begin(),
                                           App::deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

bool Engine::isFramebufferResized() const { return framebufferResized; }

void Engine::setFramebufferResized(bool framebufferResized) {
  Engine::framebufferResized = framebufferResized;
}

VkSwapchainKHR_T *Engine::getSwapChain() { return swapChain; }

VkExtent2D Engine::getSwapChainExtent() const { return swapChainExtent; }

VkFramebuffer Engine::getSwapChainFrameBuffer(uint32_t i, uint32_t j) {
  return swapChainFramebuffers[i][j];
}

} // namespace Infinite