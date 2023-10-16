#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H
#include <cstdint>
#include <vulkan/vulkan_core.h>
#pragma once

#include "../../Infinite.h"
#include "../../util/VulkanUtils.h"
#include "../Settings.h"
#include "../Software/App.h"
#include "RenderPasses/RenderPass.h"

namespace Infinite {

// Singleton
class Engine {
  friend void recordCommandBuffer(VkCommandBuffer commandBuffer,
                                  uint32_t imageIndex, RenderPass &renderPass);

public:
  Engine(Engine const &) = delete;

  void operator=(Engine const &) = delete;

  Engine(Engine &&) = delete;

  Engine &operator=(Engine &&) = delete;

  static Engine &getEngine();

  VkInstance_T *getInstance();

  void setApp(App _app);

  uint32_t getCurrentFrame() const;

  void setCurrentFrame(uint32_t currentFrame);

  void cleanUpInfinite();

  GLFWwindow *getWindow();

  static void waitForNextFrame();

  void createEngine();

  static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  VkQueue_T *getGraphicsQueue();

  void recreateSwapChain();

private:
  bool framebufferResized;

public:
  VkSwapchainKHR_T *getSwapChain();

  bool isFramebufferResized() const;

  void setFramebufferResized(bool framebufferResized);

private:
  VkInstance instance;

  VkQueue graphicsQueue;

  App app = App(nullptr, 0, 0, 0);

  uint32_t currentFrame = 0;

  Engine(){};

  void createInstance();

  VkSampleCountFlagBits getMaxUsableSampleCount();

  void pickPhysicalDevice();

  void pickDevices();

  void createLogicalDevice();

  // Swap Chain public
public:
  VkFormat getSwapChainImageFormat() const;

private:
  void cleanupSwapChain();

  void createSwapChain();

  void createImageViews();

  // Swap Chain private
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<std::vector<VkFramebuffer>> swapChainFramebuffers;

  static VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);

  // Window
  VkSurfaceKHR surface;

  GLFWwindow *window;

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);

  void createSurface();

  static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);

  void initWindow();

  // Logging
  void setupDebugMessenger();

  VKAPI_ATTR static VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);

  static VkResult CreateDebugUtilsMessengerEXT(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
      const VkAllocationCallbacks *pAllocator,
      VkDebugUtilsMessengerEXT *pDebugMessenger);

  VkDebugUtilsMessengerEXT debugMessenger;

  VkDebugUtilsMessengerEXT *getDebugMessenger();

  static void
  DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                VkDebugUtilsMessengerEXT debugMessenger,
                                const VkAllocationCallbacks *pAllocator);

  bool checkValidationLayerSupport();

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

  void initLogger();

  static void populateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT &createInfo);

  static bool checkDeviceExtensionSupport(VkPhysicalDevice device);

public:
  // Swap Chain size
  unsigned int getWindowWidth();

  unsigned int getWindowHeight();

  // Render
  friend class Model;

  VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool);

  void endSingleTimeCommands(VkCommandBuffer commandBuffer,
                             VkCommandPool commandPool);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  bool isDeviceSuitable(VkPhysicalDevice pDevice);

  void createFramebuffers(Infinite::RenderPass &renderPass,
                          uint32_t renderPasses);

  VkExtent2D getSwapChainExtent() const;

  VkFramebuffer getSwapChainFrameBuffer(uint32_t i, uint32_t j);
};

void copyBuffer(BufferAlloc srcBuffer, BufferAlloc dstBuffer,
                VkDeviceSize size);

void createVertexBuffer(BufferAlloc &vertexBuffer,
                        std::vector<Vertex> vertices);

void createIndexBuffer(BufferAlloc &indexBuffer, std::vector<uint32_t> indices);

static std::vector<const char *> getRequiredExtensions();

} // namespace Infinite

#include "../Model/Image/ColorImage.h"
#include "../Model/Image/DepthImage.h"

#endif // VULKAN_ENGINE_H
