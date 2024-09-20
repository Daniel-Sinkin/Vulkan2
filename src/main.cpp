#include <iostream>
#include <cstdlib>
#include <span>
#include <vector>
#include <string_view>
#include <fstream>
#include <chrono>
#include <thread>

using std::runtime_error;
using std::cout;
using std::vector;
using std::string_view;
using std::string;

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

#define VULKAN_ALLOCATOR = nullptr

VKAPI_ATTR auto VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData
) -> VkBool32 {
    // Check if the message is an info-level message
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        // Open the file in append mode
        std::ofstream logFile("vulkan_info.log", std::ios::out | std::ios::app);

        // Check if the file opened successfully
        if (logFile.is_open()) {
            logFile << "validation layer [INFO]: " << pCallbackData->pMessage << "\n";
            logFile.close();  // Close the file after writing
        } else {
            std::cerr << "Failed to open vulkan_info.log for writing.\n";
        }
    } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation layer [WARNING]: " << pCallbackData->pMessage << "\n";
    } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cerr << "validation layer [ERROR]: " << pCallbackData->pMessage << "\n";
    } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT){
        std::cerr << "validation layer [VERBOSE]: " << pCallbackData->pMessage << "\n";
    }
    return VK_FALSE;
}

int DEFAULT_WINDOW_WIDTH = 1920;
int DEFAULT_WINDOW_HEIGHT = 1080;

VkInstance g_instance = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT g_vk_debugMessenger = VK_NULL_HANDLE;

VkSurfaceKHR g_surface = VK_NULL_HANDLE;

VkAllocationCallbacks* g_allocator = nullptr;

GLFWwindow * g_window = nullptr;
bool g_framebufferResized = false;

int main() {
    cout << "Initializing Program.\n";

    cout << "Initializing Window.\n";
    if(glfwInit() == GLFW_FALSE) throw runtime_error("Failed to initialize GLFW.");

    // It defaults to creating OpenGL context, this disables that.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    g_window = glfwCreateWindow(
        DEFAULT_WINDOW_WIDTH,
        DEFAULT_WINDOW_HEIGHT,
        "danielsEngine",
        nullptr,
        nullptr
    );
    glfwSetFramebufferSizeCallback(g_window, [](GLFWwindow* window, int width, int height) {g_framebufferResized = true;});
    cout << "Finished Initializing Window.\n";

    cout << "Initializing Vulkan.\n";
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

    VkInstanceCreateInfo instanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext =  nullptr,
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
    };

    vector requiredExtensions(glfwExtensionSpan.begin(), glfwExtensionSpan.end());
    if (enableValidationLayers) {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#if defined(__APPLE__) && defined(__arm64__) // MacOS specific workarounds
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    requiredExtensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    vector<VkExtensionProperties> availiableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availiableExtensions.data());

    for(const auto requiredExtension: requiredExtensions) {
        string_view requiredExtensionView(requiredExtension);
        bool wasFound = false;
        for(const auto availiableExtension: availiableExtensions) {
            string_view availiableExtensionView(availiableExtension.extensionName);
            if(availiableExtensionView == requiredExtensionView) {
                wasFound = true;
                break;
            }
        }
        if(!wasFound) throw runtime_error("The following extension is not supported on this host :" + string(requiredExtensionView));
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if constexpr (enableValidationLayers) {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

        debugCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback};
        instanceCreateInfo.pNext = &debugCreateInfo;
    } else {
        instanceCreateInfo.enabledLayerCount = 0;
        instanceCreateInfo.pNext = nullptr;
    }

    VULKAN_CHECK(vkCreateInstance(&instanceCreateInfo, g_allocator, &g_instance));
    cout << "Successfully created Instance\n";

    cout << "Setting up Validation Layer callback.\n";
    const auto vulkan_FPN_debugMessengerCreate = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(g_instance, "vkCreateDebugUtilsMessengerEXT"));
    if(vulkan_FPN_debugMessengerCreate == nullptr) throw runtime_error("vkCreateDebugUtilsMessengerEXT is missing, can't setup debugging messenger.");
    VULKAN_CHECK(vulkan_FPN_debugMessengerCreate(g_instance, &debugCreateInfo, g_allocator, &g_vk_debugMessenger));
    cout << "Successfully set up Validation Layer callback.\n";
    cout << "Finished Initializing Vulkan.\n";
    VULKAN_CHECK(glfwCreateWindowSurface(g_instance, g_window, g_allocator, &g_surface));

    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(g_instance, &physicalDeviceCount, nullptr);
    if(physicalDeviceCount == 0) throw runtime_error("No vulkan compatible physical device found!.");
    if(physicalDeviceCount == 1) {
        cout << "There is exactly one physical device availiable.\n";
    } else {
        cout << "There are exactly" << physicalDeviceCount << "physical device availiable.\n";
    }

    cout << "Finished Initializing Program.\n";

    std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
    while(!glfwWindowShouldClose(g_window)) {
        std::chrono::time_point currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - startTime;
        float frameTime = elapsed.count();

        glfwPollEvents();
        cout << "We already ran for " << frameTime << " seconds!\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    cout << "Starting cleanup.\n";
    const auto vulkan_FPN_debugMessengerDestroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(g_instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (vulkan_FPN_debugMessengerDestroy != nullptr) vulkan_FPN_debugMessengerDestroy(g_instance, g_vk_debugMessenger, g_allocator);

    vkDestroySurfaceKHR(g_instance, g_surface, g_allocator);
    vkDestroyInstance(g_instance, g_allocator);
    cout << "Finished cleanup.\n";


    return EXIT_SUCCESS;
}