/*! \file window.cpp
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

namespace cs237 {

/******************** local helper functions ********************/

// wrapper function for Refresh callback
static void refreshCB (GLFWwindow *win)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->refresh ();
}

// wrapper function for Reshape callback
static void reshapeCB (GLFWwindow *win, int wid, int ht)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->reshape (wid, ht);
}

// wrapper function for Iconify callback
static void iconifyCB (GLFWwindow *win, int iconified)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->iconify (iconified != 0);
}

// wrapper function for Key callback
static void keyCB (GLFWwindow *win, int key, int scancode, int action, int mods)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->key (key, scancode, action, mods);
}

// wrapper function for CursorPos callback
static void cursorPosCB (GLFWwindow *win, double xpos, double ypos)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->cursorPos (xpos, ypos);
}

// wrapper function for CursorEnter callback
static void cursorEnterCB (GLFWwindow *win, int entered)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->cursorEnter (entered == GLFW_TRUE);
}

// wrapper function for MouseButton callback
static void mouseButtonCB (GLFWwindow *win, int button, int action, int mods)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->mouseButton (button, action, mods);
}

// wrapper function for Scroll callback
static void scrollCB (GLFWwindow *win, double xoffset, double yoffset)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->scroll (xoffset, yoffset);
}

/******************** class Window methods ********************/

Window::Window (Application *app, CreateWindowInfo const &info)
    : _app(app), _win(nullptr), _swap(app->_device)
{
    glfwWindowHint(GLFW_RESIZABLE, info.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(
        info.wid, info.ht,
        info.title.c_str(),
        nullptr, nullptr);
    if (window == nullptr) {
        ERROR("unable to create window!");
    }

    // set the user data for the window to this object
    glfwSetWindowUserPointer (window, this);

    // set up window-system callbacks
    glfwSetWindowRefreshCallback (window, refreshCB);
    glfwSetWindowSizeCallback (window, reshapeCB);
    glfwSetWindowIconifyCallback (window, iconifyCB);

    this->_win = window;
    this->_wid = info.wid;
    this->_ht = info.ht;
    this->_isVis = true;

    // set up the Vulkan surface for the window
    if (glfwCreateWindowSurface(app->_instance, window, nullptr, &this->_surf) != VK_SUCCESS) {
        ERROR("unable to create window surface!");
    }

    // set up the swap chain for the surface
    this->_createSwapChain ();

}

Window::~Window ()
{
    // destroy the swap chain as associated state
    this->_swap.cleanup ();

    // delete the surface
    vkDestroySurfaceKHR (this->_app->_instance, this->_surf, nullptr);

    glfwDestroyWindow (this->_win);
}

void Window::reshape (int wid, int ht)
{
    this->_wid = wid;
    this->_ht = ht;
}

void Window::iconify (bool iconified)
{
    this->_isVis = !iconified;
}

void Window::key (int key, int scancode, int action, int mods) { }
void Window::cursorPos (double xpos, double ypos) { }
void Window::cursorEnter (bool entered) { }
void Window::mouseButton (int button, int action, int mods) { }
void Window::scroll (double xoffset, double yoffset) { }

void Window::enableKeyEvent (bool enable)
{
    if (this->_keyEnabled && (! enable)) {
        // disable the callback
        this->_keyEnabled = false;
        glfwSetKeyCallback(this->_win, nullptr);
    }
    else if ((! this->_keyEnabled) && enable) {
        // enable the callback
        this->_keyEnabled = true;
        glfwSetKeyCallback(this->_win, keyCB);
    }

}

void Window::setCursorMode (int mode)
{
    glfwSetInputMode (this->_win, GLFW_CURSOR, mode);
}

void Window::enableCursorPosEvent (bool enable)
{
    if (this->_cursorPosEnabled && (! enable)) {
        // disable the callback
        this->_cursorPosEnabled = false;
        glfwSetCursorPosCallback(this->_win, nullptr);
    }
    else if ((! this->_cursorPosEnabled) && enable) {
        // enable the callback
        this->_cursorPosEnabled = true;
        glfwSetCursorPosCallback(this->_win, cursorPosCB);
    }

}

void Window::enableCursorEnterEvent (bool enable)
{
    if (this->_cursorEnterEnabled && (! enable)) {
        // disable the callback
        this->_cursorEnterEnabled = false;
        glfwSetCursorEnterCallback(this->_win, nullptr);
    }
    else if ((! this->_cursorEnterEnabled) && enable) {
        // enable the callback
        this->_cursorEnterEnabled = true;
        glfwSetCursorEnterCallback(this->_win, cursorEnterCB);
    }

}

void Window::enableMouseButtonEvent (bool enable)
{
    if (this->_mouseButtonEnabled && (! enable)) {
        // disable the callback
        this->_mouseButtonEnabled = false;
        glfwSetMouseButtonCallback(this->_win, nullptr);
    }
    else if ((! this->_mouseButtonEnabled) && enable) {
        // enable the callback
        this->_mouseButtonEnabled = true;
        glfwSetMouseButtonCallback(this->_win, mouseButtonCB);
    }

}

void Window::enableScrollEvent (bool enable)
{
    if (this->_scrollEnabled && (! enable)) {
        // disable the callback
        this->_scrollEnabled = false;
        glfwSetScrollCallback(this->_win, nullptr);
    }
    else if ((! this->_scrollEnabled) && enable) {
        // enable the callback
        this->_scrollEnabled = true;
        glfwSetScrollCallback(this->_win, scrollCB);
    }

}

Window::SwapChainDetails Window::_getSwapChainDetails ()
{
    auto dev = this->_app->_gpu;
    auto surf = this->_surf;
    Window::SwapChainDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surf, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            dev, surf,
            &presentModeCount, details.presentModes.data());
    }

    return details;

}

void Window::_createSwapChain ()
{
    SwapChainDetails swapChainSupport = this->_getSwapChainDetails ();

    // choose the best aspects of the swap chain
    VkSurfaceFormatKHR surfaceFormat = swapChainSupport.chooseSurfaceFormat();
    VkPresentModeKHR presentMode = swapChainSupport.choosePresentMode();
    VkExtent2D extent = swapChainSupport.chooseExtent(this->_win);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if ((swapChainSupport.capabilities.maxImageCount > 0)
    && (imageCount > swapChainSupport.capabilities.maxImageCount)) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapInfo{};
    swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.surface = this->_surf;

    swapInfo.minImageCount = imageCount;
    swapInfo.imageFormat = surfaceFormat.format;
    swapInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapInfo.imageExtent = extent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto qIdxs = this->_app->_qIdxs;
    uint32_t qIndices[] = {qIdxs.graphics, qIdxs.present};

    // check if the graphics and presentation queues are distinct
    if (qIdxs.graphics != qIdxs.present) {
        swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapInfo.queueFamilyIndexCount = 2;
        swapInfo.pQueueFamilyIndices = qIndices;
    }
    else {
        swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = presentMode;
    swapInfo.clipped = VK_TRUE;

    swapInfo.oldSwapchain = VK_NULL_HANDLE;

    VkDevice dev = this->device();
    if (vkCreateSwapchainKHR(dev, &swapInfo, nullptr, &this->_swap.chain) != VK_SUCCESS)
    {
        ERROR("unable to create swap chain!");
    }

    // get the vector of images that represent the swap chain
    vkGetSwapchainImagesKHR(dev, this->_swap.chain, &imageCount, nullptr);
    this->_swap.images.resize(imageCount);
    vkGetSwapchainImagesKHR(
        dev, this->_swap.chain,
        &imageCount, this->_swap.images.data());

    this->_swap.imageFormat = surfaceFormat.format;
    this->_swap.extent = extent;

    // create the image views
    // first we initialize the invariant parts of the create info structure
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = this->_swap.imageFormat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    // create a view per swap-chain image
    this->_swap.views.resize(this->_swap.images.size());
    for (int i = 0; i < this->_swap.images.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.image = this->_swap.images[i];
        if (vkCreateImageView(dev, &viewInfo, nullptr, &this->_swap.views[i])
            != VK_SUCCESS)
        {
            ERROR("unable to create image views!");
        }
    }

}

/******************** struct Window::SwapChainDetails methods ********************/

// choose the surface format for the buffers
VkSurfaceFormatKHR Window::SwapChainDetails::chooseSurfaceFormat ()
{
    for (const auto& fmt : this->formats) {
        if ((fmt.format == VK_FORMAT_B8G8R8A8_SRGB)
        && (fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            return fmt;
        }
    }

    return this->formats[0];
}

// choose a presentation mode for the buffers
VkPresentModeKHR Window::SwapChainDetails::choosePresentMode ()
{
    for (const auto& mode : this->presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

//! compute the extent of the buffers
VkExtent2D Window::SwapChainDetails::chooseExtent (GLFWwindow *win)
{
    if (this->capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return this->capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(win, &width, &height);

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

        actualExtent.width = std::clamp(
            actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(
            actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actualExtent;
    }

}

/******************** struct Window::SwapChain methods ********************/

std::vector<VkFramebuffer> Window::SwapChain::framebuffers (VkRenderPass renderPass)
{
    assert (this->size() > 0);

    // the framebuffer attachments; currently we only have color, but we will add
    // a depth buffer
    VkImageView attachments[this->numAttachments];

    // initialize the invariant parts of the create info structure
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = renderPass;
    fbInfo.attachmentCount = this->numAttachments;
    fbInfo.pAttachments = attachments;
    fbInfo.width = this->extent.width;
    fbInfo.height = this->extent.height;
    fbInfo.layers = 1;

    // create a frambuffer per swap-chain image
    std::vector<VkFramebuffer> fbs(this->size());
    for (size_t i = 0; i < this->size(); i++) {
        attachments[0] = this->views[i];
        if (vkCreateFramebuffer(this->device, &fbInfo, nullptr, &fbs[i]) != VK_SUCCESS) {
            ERROR("unable to create framebuffer");
        }
    }

    return fbs;
}

void Window::SwapChain::cleanup ()
{
    for (auto view : this->views) {
        vkDestroyImageView(this->device, view, nullptr);
    }
    /* note that the images are owned by the swap chain object, so we do not have
     * to destroy them.
     */

    vkDestroySwapchainKHR(this->device, this->chain, nullptr);
}

} // namespace cs237
