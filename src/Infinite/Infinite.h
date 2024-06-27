#ifndef VULKAN_INFINITE_H
#define VULKAN_INFINITE_H

#include "backend/Model/Models/Model.h"
#include "backend/Rendering/Queues/Queue.h"
#include "backend/Software/App.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>


namespace Infinite {

class RenderPass;

extern VkSurfaceKHR surface;

extern Queue queues[static_cast<int>(QueueOrder::Count)]; // specific order

extern VkInstance instance;

extern const std::vector<const char *> validationLayers;

extern uint32_t currentFrame;

extern VkDebugUtilsMessengerEXT debugMessenger;

extern VkSwapchainKHR swapChain;
extern std::vector<VkImage> swapChainImages;
extern VkExtent2D swapChainExtent;
extern std::vector<VkImageView> swapChainImageViews;
extern std::vector<VkFramebuffer> swapChainFramebuffers;

extern VkPhysicalDevice physicalDevice;
extern VkDevice device;
//    extern Settings * settings;
extern VkFormat swapChainImageFormat;
extern VmaAllocator allocator;
extern VkCommandPool imagePool;
extern std::vector<RenderPass *> renderPasses; // ToDo: CleanUp
extern GLFWwindow *window;
extern bool framebufferResized;

void initInfinite(App app);
void framebufferResizeCallback(GLFWwindow *window, int width, int height);

bool checkValidationLayerSupport();

void pickPhysicalDevice();
void createLogicalDevice();

void addRenderPass(RenderPass *r);

void recreateSwapChain();

void renderFrame();

void waitForNextFrame();

void cleanUp();

void waitForNextFrame();

static void createCommandPool(VkCommandPool &pool, VkDevice device);

Model createModel(std::string _name, std::string model_path,
                  const char *texture_path);

// logger
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator);
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger);

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo);

bool isDeviceSuitable(VkPhysicalDevice pDevice);

bool checkDeviceExtensionSupport(VkPhysicalDevice device);

VkSampleCountFlagBits getMaxUsableSampleCount();

// Swap Chain
void createSwapChain();
void recreateSwapChain();
void cleanupSwapChain();

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes);

void createImageViews();

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats);
} // namespace Infinite

#endif // VULKAN_INFINITE_H