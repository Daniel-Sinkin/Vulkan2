#include <iostream>
#include <cstdlib>
#include "vector"
#include "span"

using std::runtime_error;
using std::cout;
using std::vector;

#define GLFW_INCLUDE_VULKAN
#include <GL/glew.h>
#include <GLFW/glfw3.h> // Implicitly imports vulkan
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h> // For VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

#define VULKAN_CHECK(x) if ((x) != VK_SUCCESS) throw runtime_error("Failed on " #x);

VKAPI_ATTR auto VKAPI_CALL debugCallback(
    [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    [[maybe_unused]] void *pUserData) -> VkBool32 {

    std::cerr << "validation layer: " << pCallbackData->pMessage << "";
    return VK_FALSE;
}

int main() {
    cout << "Initializing Program.\n";
    cout << "Checking version.\n";
    uint32_t apiVersion = 0;
    VULKAN_CHECK(vkEnumerateInstanceVersion(&apiVersion));

    cout << "Detected Vulkan version is: " <<
            VK_API_VERSION_MAJOR(apiVersion) << "." <<
            VK_API_VERSION_MINOR(apiVersion) << "." <<
            VK_VERSION_PATCH(apiVersion) << "\n";
    if(VK_API_VERSION_1_3 > apiVersion) throw runtime_error("API version is too old, need at least 1.3.0!");

    cout << "Creating instance.\n";
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Vulkan2",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "danielsEngine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_3
    };

    const vector validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const vector deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME};

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::span glfwExtensionSpan(glfwExtensions, glfwExtensionCount);


    VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext =  nullptr,
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
    };


    vector extensions(glfwExtensionSpan.begin(), glfwExtensionSpan.end());
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#if defined(__APPLE__) && defined(__arm64__) // MacOS specific workarounds
    extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    vector<VkExtensionProperties> baseExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, baseExtensions.data());

    // TODO: Validate Extensions here!

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if constexpr (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        debugCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback};
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    VkInstance instance = VK_NULL_HANDLE;
    VULKAN_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));

    return 0;
}