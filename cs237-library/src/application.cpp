/*! \file application.cpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hpp"
#include <cstring>
#include <cstdlib>
#include <set>

namespace cs237 {

static std::vector<const char *> requiredExtensions (bool debug);
static int graphicsQueueIndex (VkPhysicalDevice dev);

const std::vector<const char*> kDeviceExts = {
        "VK_KHR_portability_subset",
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

const std::vector<const char *> kValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };


/******************** class Application methods ********************/

Application::Application (std::vector<const char *> &args, std::string const &name)
  : _name(name),
    _messages(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT),
    _debug(0),
    _gpu(VK_NULL_HANDLE)
{
    // process the command-line arguments
    for (auto it = args.cbegin();  it != args.cend();  ++it) {
        if (*it[0] == '-') {
            if (strcmp(*it, "-debug") == 0) {
                if (this->_messages > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                    this->_messages = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                }
                this->_debug = true;
            }
            else if (strcmp(*it, "-verbose")) {
                this->_messages = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
            }
        }
    }

    // initialize GLFW
    glfwInit();

    // create a Vulkan instance
    this->_createInstance();
}

Application::~Application ()
{
    // destroy the logical device
    vkDestroyDevice(this->_device, nullptr);

    // delete the instance
    vkDestroyInstance(this->_instance, nullptr);

    // shut down GLFW
    glfwTerminate();

}

// create a Vulkan instance
void Application::_createInstance ()
{
    // set up the application information
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = this->_name.data();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = nullptr;
    appInfo.engineVersion = 0;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // figure out what extensions we are going to need
    auto extensions = requiredExtensions(this->_debug);

    // intialize the creation info struct
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.pApplicationInfo = &appInfo;
    if (this->_debug) {
        // in debug mode, we add validation layer(s)
        createInfo.enabledLayerCount = kValidationLayers.size();
        createInfo.ppEnabledLayerNames = kValidationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
    }
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &(this->_instance)) != VK_SUCCESS) {
        ERROR("unable to create a vulkan instance");
    }

    // pick the physical device; we require fillModeNonSolid to support
    // wireframes.
    VkPhysicalDeviceFeatures reqs{};
    reqs.fillModeNonSolid = VK_TRUE;
    this->_selectDevice (&reqs);

    // create the logical device and get the queues
    this->_createLogicalDevice ();
}

// check that a device meets the requested features
//
static bool hasFeatures (VkPhysicalDevice gpu, VkPhysicalDeviceFeatures *reqFeatures)
{
    if (reqFeatures == nullptr) {
        return true;
    }
    VkPhysicalDeviceFeatures availFeatures;
    vkGetPhysicalDeviceFeatures (gpu, &availFeatures);

    if (reqFeatures->fillModeNonSolid == availFeatures.fillModeNonSolid) {
        return true;
    }
    else {
        return false;
    }
}

// A helper function to pick the physical device when there is more than one.
// Currently, we ignore the features and favor discrete GPUs over other kinds
//
void Application::_selectDevice (VkPhysicalDeviceFeatures *reqFeatures)
{
    // figure out how many devices are available
    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices(this->_instance, &devCount, nullptr);

    if (devCount == 0) {
        ERROR("no available GPUs");
    }

    // get the available devices
    std::vector<VkPhysicalDevice> devices(devCount);
    vkEnumeratePhysicalDevices(this->_instance, &devCount, devices.data());

    // Select a device that supports graphics and presentation
    // This code is brute force, but we only expect one or two devices.
    // Future versions will support checking for properties.

// FIXME: we need to also check that the device supports swap chains!!!!

    // we first look for a discrete GPU
    for (auto & dev : devices) {
        if (hasFeatures(dev, reqFeatures) && this->_getQIndices(dev)) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                this->_gpu = dev;
                return;
            }
        }
    }
    // no discrete GPU, so look for an integrated GPU
    for (auto & dev : devices) {
        if (hasFeatures(dev, reqFeatures) && this->_getQIndices(dev)) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                this->_gpu = dev;
                return;
            }
        }
    }
    // check for any device that supports graphics and presentation
    for (auto & dev : devices) {
        if (hasFeatures(dev, reqFeatures) && this->_getQIndices(dev)) {
            this->_gpu = dev;
            return;
        }
    }

    ERROR("no available GPUs that support graphics");

}

int32_t Application::_findMemory (
    uint32_t reqTypeBits,
    VkMemoryPropertyFlags reqProps) const
{
    // get the memory properties for the device
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(this->_gpu, &memProps);

    for (int32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((reqTypeBits & (1 << i))
        && (memProps.memoryTypes[i].propertyFlags & reqProps) == reqProps)
        {
            return i;
        }
    }

    return -1;

}

void Application::_createLogicalDevice ()
{
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = nullptr;

    // set up the device queues info struct; the graphics and presentation queues may
    // be different or the same, so we have to initialize either one or two create-info
    // structures
    std::vector<VkDeviceQueueCreateInfo> qCreateInfos;
    std::set<uint32_t> uniqueQIndices = { this->_qIdxs.graphics, this->_qIdxs.present };

    float qPriority = 1.0f;
    for (auto qix : uniqueQIndices) {
        VkDeviceQueueCreateInfo qCreateInfo{};
        qCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qCreateInfo.queueFamilyIndex = qix;
        qCreateInfo.queueCount = 1;
        qCreateInfo.pQueuePriorities = &qPriority;
        qCreateInfos.push_back(qCreateInfo);
    }

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(qCreateInfos.size());
    createInfo.pQueueCreateInfos = qCreateInfos.data();

    // include validation layer if in debug mode
    if (this->_debug) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
        createInfo.ppEnabledLayerNames = kValidationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // set up the enabled extensions to include swap chains
    createInfo.enabledExtensionCount = static_cast<uint32_t>(kDeviceExts.size());
    createInfo.ppEnabledExtensionNames = kDeviceExts.data();

    // for now, we are not enabling any extra features
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    createInfo.pEnabledFeatures = &deviceFeatures;

    // create the logical device
    if (vkCreateDevice(this->_gpu, &createInfo, nullptr, &this->_device) != VK_SUCCESS) {
        ERROR("unable to create logical device!");
    }

    // get the queues
    vkGetDeviceQueue(this->_device, this->_qIdxs.graphics, 0, &this->_queues.graphics);
    vkGetDeviceQueue(this->_device, this->_qIdxs.present, 0, &this->_queues.present);

}

// Get the list of supported extensions
//
std::vector<VkExtensionProperties> Application::supportedExtensions ()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extProps(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extProps.data());
    return extProps;
}

// Get the list of supported layers
//
std::vector<VkLayerProperties> Application::supportedLayers ()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    return layers;
}

/******************** local utility functions ********************/

// A helper function for determining the extensions that are required
// when creating an instance. These include the extensions required
// by GLFW and the extensions required for debugging support when
// `debug` is true.
//
static std::vector<const char *> requiredExtensions (bool debug)
{
    uint32_t extCount;

    // extensions required by GLFW
    const char **glfwReqExts = glfwGetRequiredInstanceExtensions(&extCount);

    // in debug mode we need the debug utilities
    uint32_t debugExtCount = debug ? 1 : 0;

    // initialize the vector of extensions with the GLFW extensions
    std::vector<const char *> reqExts (glfwReqExts, glfwReqExts+extCount);

    reqExts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    reqExts.push_back("VK_KHR_get_physical_device_properties2");

    // add debug extensions
    if (debug) {
        reqExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return reqExts;
}

// check the device's queue families for graphics and presentation support
//
bool Application::_getQIndices (VkPhysicalDevice dev)
{
    // first we figure out how many queue families the device supports
    uint32_t qFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qFamilyCount, nullptr);

    // then we get the queue family info
    std::vector<VkQueueFamilyProperties> qFamilies(qFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qFamilyCount, qFamilies.data());

    Application::Queues<int32_t> indices = { -1, -1 };
    for (int i = 0;  i < qFamilyCount;  ++i) {
        // check for graphics support
        if ((indices.graphics < 0)
        && (qFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.graphics = i;
        }
        // check for presentation support
        if (indices.present < 0) {
            if (glfwGetPhysicalDevicePresentationSupport(this->_instance, dev, i)) {
                indices.present = i;
            }
        }
        // check if we are finished
        if ((indices.graphics >= 0) && (indices.present >= 0)) {
            this->_qIdxs.graphics = static_cast<uint32_t>(indices.graphics);
            this->_qIdxs.present = static_cast<uint32_t>(indices.present);
            return true;
        }
    }


    return false;

}

} // namespace cs237
