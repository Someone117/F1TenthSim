#include "Infinite.h"
#include "backend/Model/Image/Image.h"
#include "backend/Model/Models/DescriptorSet.h"
#include "backend/Rendering/Queues/Queue.h"
#include "backend/Rendering/RenderPasses/RenderPass.h"
#include "backend/Settings.h"
#include "backend/Software/App.h"
#include "frontend/Camera.h"
#include "util/VulkanUtils.h"
#include "util/constants.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "backend/Model/Image/stb_image_write.h"
#endif
#include "backend/Model/Image/stb_image.h"

#include <vk_mem_alloc.h>

namespace Infinite {
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};
Queue queues[static_cast<int>(QueueOrder::Count)] = {
    Queue(QueueIndex{QueueType::GRAPHICS, std::nullopt}),
    Queue(QueueIndex{QueueType::COMPUTE, std::nullopt}),
    Queue(QueueIndex{QueueType::PRESENT, std::nullopt})}; // specific order

uint32_t currentFrame = 0;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkFormat swapChainImageFormat;
VmaAllocator allocator;
VkCommandPool imagePool;
std::vector<RenderPass *> renderPasses; // ToDo: CleanUp
VkSurfaceKHR surface;

VkInstance instance;

VkDebugUtilsMessengerEXT debugMessenger;

VkSwapchainKHR swapChain;
std::vector<VkImage> swapChainImages;
VkExtent2D swapChainExtent;
std::vector<VkImageView> swapChainImageViews;
std::vector<VkFramebuffer> swapChainFramebuffers;

GLFWwindow *window;
bool framebufferResized;

void initInfinite(App app) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  // glfwSetWindowUserPointer(window, this); // not needed cuz no oop

  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

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

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (enableValidationLayers) {
    createInfo.enabledLayerCount = validationLayers.size();
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
  if (enableValidationLayers) {

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                     &debugMessenger) != VK_SUCCESS) {
      throw std::runtime_error("failed to set up debug messenger!");
    }
  }

  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }

  pickPhysicalDevice();

  createLogicalDevice();

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

  // findQueueFamilies(physicalDevice, queues); // fix this hell

  createCommandPool(imagePool, device);

  for (const auto &r : renderPasses) {
    r->createRenderPass(device, physicalDevice, swapChainImageFormat,
                        msaaSamples);
  }

  DescriptorSet::createDescriptorPool(device);
  bool first = false;

  for (const auto &r : renderPasses) {
    if (r->getPipelineType() == VK_PIPELINE_BIND_POINT_COMPUTE) {
      r->preInit(device, physicalDevice, swapChainImageFormat, swapChainExtent,
                 allocator, msaaSamples, swapChainImageViews);
    } else {
      if (!first) {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        first = true;
      }
      r->preInit(device, physicalDevice, swapChainImageFormat, swapChainExtent,
                 allocator, msaaSamples, swapChainImageViews);
    }
  }

  for (const auto &r : renderPasses) {
    r->createCommandBuffers(imagePool, physicalDevice, device);
    r->createSyncObjects(device);
  }
  std::cout << "Finished initiation of Infinite Engine" << std::endl;
}

bool checkValidationLayerSupport() {
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

void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  framebufferResized = true;
}

// ToDo: change
void pickPhysicalDevice() {
  uint32_t deviceCount = 0;

  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

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

void createLogicalDevice() {

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies;
  for (uint32_t i = 0; i < static_cast<int>(QueueOrder::Count); i++) {
    uniqueQueueFamilies.insert(queues[i].index.index.value());
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

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

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

void addRenderPass(RenderPass *r) {
  renderPasses.push_back(r);
  r->setIndex(renderPasses.size() - 1);
}

void recreateSwapChain() { // named wrong
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
  for (const auto &r : renderPasses) {
    r->recreateSwapChainWork(allocator, device, physicalDevice,
                             swapChainImageFormat, swapChainExtent,
                             swapChainImageViews);
  }
}

void renderFrame() {
  // Compute submission
  renderPasses.front()->waitForFences();
  for (const auto &r : renderPasses) {
    if (r->getPipelineType() == VK_PIPELINE_BIND_POINT_COMPUTE)
      continue;
    for (BaseModel *model : r->getModels()) {
      model->updateUniformBuffer(currentFrame, cameras, // hard code now
                                 allocator, swapChainExtent.width,
                                 swapChainExtent.height);
    }
  }

  uint32_t imageIndex = 0;
  int32_t firstGraphics = -1;
  std::vector<VkSemaphore> prevSemaphores = {};
  bool needToRecreateSwapChain = false;
  for (uint32_t i = 0; i < renderPasses.size(); i++) {
    if (i != 0) {
      renderPasses[i]->waitForFences();
    }
    if (renderPasses[i]->getPipelineType() == VK_PIPELINE_BIND_POINT_GRAPHICS &&
        firstGraphics == -1) {
      firstGraphics = i;
      VkResult result = vkAcquireNextImageKHR(
          device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
          VK_NULL_HANDLE, &imageIndex);

      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        needToRecreateSwapChain = true;
      } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
      }
      prevSemaphores.push_back(imageAvailableSemaphores[currentFrame]);
    }

    renderPasses[i]->resetFences();

    renderPasses[i]->renderFrame(currentFrame, imageIndex, prevSemaphores);

    prevSemaphores.clear();
    prevSemaphores.push_back(
        renderPasses[i]->getFinishedSemaphores()[currentFrame]);
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = prevSemaphores.data();

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  uint32_t result = vkQueuePresentKHR(
      queues[static_cast<uint32_t>(QueueOrder::PRESENT)].queue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebufferResized) {
    framebufferResized = false;
    needToRecreateSwapChain = true;

  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }
  currentFrame = ((currentFrame + 1) % MAX_FRAMES_IN_FLIGHT);
  if (needToRecreateSwapChain) {
    recreateSwapChain();
  }
}

void waitForNextFrame() { vkDeviceWaitIdle(device); }

// todo: check a LOT
void cleanUp() {
  for (uint32_t i = 0; i < renderPasses.size(); i++) {
    renderPasses[i]->destroy(device, allocator);
  }
  cameras.~Camera();
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
  //        vkDestroyDescriptorPool(getInstance(),
  //        descriptorPool, nullptr);
  //
  //        vkDestroyDescriptorSetLayout(getInstance(),
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
  //            vkDestroySemaphore(getInstance(),
  //            renderFinishedSemaphores[i], nullptr);
  //            vkDestroySemaphore(getInstance(),
  //            imageAvailableSemaphores[i], nullptr);
  //            vkDestroyFence(getInstance(),
  //            inFlightFences[i], nullptr);
  //        }
  //
  //        vkDestroyCommandPool(getInstance(), commandPool,
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

Model createModel(std::string _name, std::string model_path,
                  const char *texture_path) {
  return Model(_name, model_path, texture_path, device, physicalDevice,
               allocator, swapChainExtent.width, swapChainExtent.height,
               swapChainImageFormat);
}

void createCommandPool(VkCommandPool &pool, VkDevice device) {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex =
      queues[static_cast<uint32_t>(QueueType::GRAPHICS)].index.index.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

// debugger
VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  // if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
  std::cout << "Validation Layer: " << pCallbackData->pMessage << std::endl;
  // }
  return VK_FALSE;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

VkResult CreateDebugUtilsMessengerEXT(
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

void populateDebugMessengerCreateInfo(
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

bool isDeviceSuitable(VkPhysicalDevice pDevice) {

  findQueueFamilies(pDevice, queues);

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
    if (!queues[k].index.index.has_value()) {
      allIndicesFound = false;
      break;
    }
  }
  return allIndicesFound && extensionsSupported && swapChainAdequate &&
         supportedFeatures.samplerAnisotropy;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
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

VkSampleCountFlagBits getMaxUsableSampleCount() {
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

void saveScreenshot(const char *filename, ScreenShotFormat screenshotFormat) {
  bool supportsBlit = true;

  // Check blit support for source and destination
  VkFormatProperties formatProps;

  // Check if the device supports blitting from optimal images (the swapchain
  // images are in optimal format)
  vkGetPhysicalDeviceFormatProperties(physicalDevice, swapChainImageFormat,
                                      &formatProps);
  if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
    std::cerr << "Device does not support blitting from optimal tiled images, "
                 "using copy instead of blit!"
              << std::endl;
    supportsBlit = false;
  }

  // Check if the device supports blitting to linear images
  // vkGetPhysicalDeviceFormatProperties(physicalDevice,
  // VK_FORMAT_R8G8B8A8_UNORM,
  //                                     &formatProps);
  // if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
  //   std::cerr << "Device does not support blitting to linear tiled images, "
  //                "using copy instead of blit!"
  //             << std::endl;
  //   supportsBlit = false;
  // }

  // Check if the device supports blitting to linear images
  vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_R8G8B8A8_SRGB,
                                      &formatProps);
  if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
    std::cerr << "Device does not support blitting to linear tiled images, "
                 "using copy instead of blit!"
              << std::endl;
    supportsBlit = false;
  }

  // Source for the copy is the last rendered swapchain image
  VkImage srcImage = swapChainImages[currentFrame];

  // Create the linear tiled destination image to copy to and to read the memory
  // from

  Image destImage{};
  Image::createImage(swapChainExtent.width, swapChainExtent.width,
                     VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
                     VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                     destImage.getImage(), VK_SAMPLE_COUNT_1_BIT,
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  // Do the actual blit from the swapchain image to our host visible destination
  // image

  VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, imagePool);
  // Transition destination image to transfer destination layout

  Image::addBarrier(&destImage, VK_FORMAT_R8G8B8A8_SRGB,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_SAMPLE_COUNT_1_BIT,
                    commandBuffer);

  // Transition swapchain image from present to transfer source layout
  Image::addBarrier(&srcImage, VK_FORMAT_R8G8B8A8_SRGB,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_SAMPLE_COUNT_1_BIT,
                    commandBuffer);

  // If source and destination support blit we'll blit as this also does
  // automatic format conversion (e.g. from BGR to RGB)
  if (supportsBlit) {
    // Define the region to blit (we will blit the whole swapchain image)
    VkOffset3D blitSize;
    blitSize.x = swapChainExtent.width;
    blitSize.y = swapChainExtent.height;
    blitSize.z = 1;
    VkImageBlit imageBlitRegion{};
    imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.srcSubresource.layerCount = 1;
    imageBlitRegion.srcOffsets[1] = blitSize;
    imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.dstSubresource.layerCount = 1;
    imageBlitRegion.dstOffsets[1] = blitSize;

    // Issue the blit command
    vkCmdBlitImage(
        commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        destImage.getImage().image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
        &imageBlitRegion, VK_FILTER_NEAREST);
  } else {
    // Otherwise use image copy (requires us to manually flip components)
    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = swapChainExtent.width;
    imageCopyRegion.extent.height = swapChainExtent.height;
    imageCopyRegion.extent.depth = 1;

    // Issue the copy command
    vkCmdCopyImage(commandBuffer, srcImage,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   destImage.getImage().image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
  }

  // Transition destination image to general layout, which is the required
  // layout for mapping the image memory later on
  Image::addBarrier(
      &destImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL, VK_SAMPLE_COUNT_1_BIT, commandBuffer);

  Image::addBarrier(
      &srcImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_SAMPLE_COUNT_1_BIT, commandBuffer);
  // Transition back the swap chain image after the blit is done

  endSingleTimeCommands(
      device, commandBuffer, imagePool,
      queues[static_cast<uint32_t>(QueueOrder::GRAPHICS)].queue);

  // Get layout of the image (including row pitch)
  VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
  VkSubresourceLayout subResourceLayout;
  vkGetImageSubresourceLayout(device, destImage.getImage().image, &subResource,
                              &subResourceLayout);

  // Map image memory so we can start copying from it
  const char *data;
  vmaMapMemory(allocator, destImage.getImage().allocation, (void **)&data);

  if (screenshotFormat == PNG || screenshotFormat == JPG) {
    // Allocate a buffer to hold the image data without row padding
    uint32_t bytesPerPixel = 4; // Assuming 4 bytes per pixel (RGBA)
    std::vector<uint8_t> tightlyPackedData(
        swapChainExtent.width * swapChainExtent.height * bytesPerPixel);

    // Copy each row from the original image data, skipping the padding bytes
    uint8_t *srcData = reinterpret_cast<uint8_t *>(const_cast<char *>(data));
    for (uint32_t y = 0; y < swapChainExtent.height; ++y) {
      // Calculate source and destination pointers
      uint8_t *srcRow =
          srcData +
          y * subResourceLayout.rowPitch; // The row pitch includes padding
      uint8_t *dstRow =
          tightlyPackedData.data() + y * swapChainExtent.width * bytesPerPixel;

      // Copy the valid row data (excluding padding)
      memcpy(dstRow, srcRow, swapChainExtent.width * bytesPerPixel);
    }

    if (screenshotFormat == PNG) {
      stbi_write_png(filename, swapChainExtent.width, swapChainExtent.height, 4,
                     tightlyPackedData.data(),
                     swapChainExtent.width * bytesPerPixel);
    } else {
      stbi_write_jpg(filename, swapChainExtent.width, swapChainExtent.height, 4,
                     tightlyPackedData.data(), 100);
    }
  } else {
    data += subResourceLayout.offset;

    std::ofstream file(filename, std::ios::out | std::ios::binary);

    // ppm header
    file << "P6\n"
         << swapChainExtent.width << "\n"
         << swapChainExtent.height << "\n"
         << 255 << "\n";

    // If source is BGR (destination is always RGB) and we can't use blit (which
    // does automatic conversion), we'll have to manually swizzle color
    // components
    bool colorSwizzle = false;
    // Check if source is BGR
    // Note: Not complete, only contains most common and basic BGR surface
    // formats for demonstration purposes
    if (!supportsBlit) {
      std::vector<VkFormat> formatsBGR = {VK_FORMAT_B8G8R8A8_SRGB,
                                          VK_FORMAT_B8G8R8A8_UNORM,
                                          VK_FORMAT_B8G8R8A8_SNORM};

      for (VkFormat format : formatsBGR) {
        if (format == swapChainImageFormat) {
          colorSwizzle = true;
          break;
        }
      }
    }
    // ppm binary pixel data
    for (uint32_t y = 0; y < swapChainExtent.height; y++) {
      unsigned int *row = (unsigned int *)data;
      for (uint32_t x = 0; x < swapChainExtent.width; x++) {
        if (colorSwizzle) {
          file.write((char *)row + 2, 1);
          file.write((char *)row + 1, 1);
          file.write((char *)row, 1);
        } else {
          file.write((char *)row, 3);
        }
        row++;
      }
      data += subResourceLayout.rowPitch;
    }
    file.close();
  }

  std::cout << "Screenshot saved to disk" << std::endl;

  // Clean up resources
  vmaUnmapMemory(allocator, destImage.getImage().allocation);
  destImage.destroy(allocator);
}

// Swap Chain
void cleanupSwapChain() {
  for (VkFramebuffer swapChainFramebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(device, swapChainFramebuffer, nullptr);
  }

  for (VkImageView swapChainImageView : swapChainImageViews) {
    vkDestroyImageView(device, swapChainImageView, nullptr);
  }

  vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void createSwapChain() {
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
  createInfo.imageUsage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                                       // for screenshots

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  uint32_t queueFamilyIndices[static_cast<int>(QueueOrder::Count)];
  for (uint32_t i = 0; i < static_cast<int>(QueueOrder::Count); i++) {
    queueFamilyIndices[i] = queues[i].index.index.value();
  }

  if (queues[static_cast<int>(QueueOrder::GRAPHICS)].index.index !=
      queues[static_cast<int>(QueueOrder::PRESENT)].index.index) {
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

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width =
        glm::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        glm::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

// ToDo: change
VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == PRESENT_MODE) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

void createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); i++)
    swapChainImageViews[i] =
        Image::createImageView(swapChainImages[i], swapChainImageFormat, 1);
}

// unsigned int swapChainExtent.width { return swapChainExtent.width; }

// unsigned int swapChainExtent.height { return swapChainExtent.height; }

// Window
VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

} // namespace Infinite