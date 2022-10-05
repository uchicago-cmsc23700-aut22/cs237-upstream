/*! \file cs237-window.hpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_WINDOW_HPP_
#define _CS237_WINDOW_HPP_

#include "vulkan/vulkan_core.h"
#ifndef _CS237_HPP_
#error "cs237-window.hpp should not be included directly"
#endif

namespace cs237 {

//! structure containing parameters for the `creteWindow` method
//
struct CreateWindowInfo {
    int wid;            //!< the window width
    int ht;             //!< the window height
    std::string title;  //!< window title
    bool resizable;

    CreateWindowInfo (int w, int h)
        : wid(w), ht(h), title(""), resizable(false)
    { }

};

//! abstract base class for simple GLFW windows used to view buffers, etc.
//
class Window {
public:

    //! destructor: it destroys the underlying GLFW window
    virtual ~Window ();

    //! return the logical device for this window
    VkDevice device () const { return this->_app->_device; }

    //! Refresh the contents of the window.  This method is also invoked
    //! on Refresh events.
    void refresh ()
    {
        if (this->_isVis) {
            this->draw();
            glfwSwapBuffers (this->_win);
        }
    }

    //! Hide the window
    void hide ()
    {
      glfwHideWindow (this->_win);
      this->_isVis = false;
    }

    //! Show the window (a no-op if it is already visible)
    void show ()
    {
      glfwShowWindow (this->_win);
      this->_isVis = true;
    }

    //! virtual draw method provided by derived classes to draw the contents of the
    //! window.  It is called by Refresh.
    virtual void draw () = 0;

    //! method invoked on Reshape events.  It resets the viewport and the
    //! projection matrix (see SetProjectionMatrix)
    virtual void reshape (int wid, int ht);

    //! method invoked on Iconify events.
    virtual void iconify (bool iconified);

    //! get the value of the "close" flag for the window
    bool windowShouldClose ()
    {
        return glfwWindowShouldClose (this->_win);
    }

protected:
    //! information about swap-chain support
    struct SwapChainDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;

        //! \brief choose a surface format from the available formats
        VkSurfaceFormatKHR chooseSurfaceFormat ();
        //! \brief choose a presentation mode from the available modes; we prefer
        //!        "mailbox" (aka triple buffering)
        VkPresentModeKHR choosePresentMode ();
        //!
        VkExtent2D chooseExtent (GLFWwindow *win);
    };

    //! the collected information about the swap-chain for a window
    struct SwapChain {
        VkDevice device;                        //!< the owning logical device
        VkSwapchainKHR chain;                   //!< the swap chain object
        VkFormat imageFormat;                   //!< pixel format of image buffers
        VkExtent2D extent;                      //!< size of swap buffer images
        int numAttachments;                     //!< the number of framebuffer attachments
        // the following vectors hold the state for each of the buffers in the
        // swap chain.
        std::vector<VkImage> images;            //!< images for the swap buffers
        std::vector<VkImageView> views;         //!< image views for the swap buffers
/* TODO: extend with support for other image attachments, such as the depth buffer */

        SwapChain (VkDevice dev) : device(dev), numAttachments(1) { }

        //! return the number of buffers in the swap chain
        int size () const { return this->images.size(); }

        //! allocate frame buffers for a rendering pass
        std::vector<VkFramebuffer> framebuffers (VkRenderPass renderPass);

        //! \brief destroy the Vulkan state for the swap chain
        void cleanup ();
    };

    Application *_app;                  //!< the owning application
    GLFWwindow *_win;                   //!< the underlying window
    int _wid, _ht;	                //!< window dimensions
    bool _isVis;                        //!< true, when the window is visible
    // Vulkan state for rendering
    VkSurfaceKHR _surf;                 //!< the Vulkan surface to render to
    SwapChain _swap;                    //!< buffer-swapping information

    VkSemaphore _imageAvailable;
    VkSemaphore _renderFinished;
    VkFence _inFlight;

    //! \brief the Window base-class constructor
    //! \param app      the owning application
    //! \param info     information for creating the window, such as size and title
    Window (Application *app, CreateWindowInfo const &info);

    //! \brief Get the swap-chain details for a physical device
    SwapChainDetails _getSwapChainDetails ();

    //! \brief Create the swap chain for this window; this initialized the _swap
    //!        instance variable.
    void _createSwapChain();

};

} // namespace cs237

#endif // !_CS237_WINDOW_HPP_
