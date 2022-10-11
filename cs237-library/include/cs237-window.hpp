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

    //! the graphics queue
    VkQueue graphicsQ () const { return this->_app->_queues.graphics; }

    //! the presentation queue
    VkQueue presentationQ () const { return this->_app->_queues.present; }

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

    //!{
    //! Input handling methods; override these in the derived window
    //! classes to do something useful.
    virtual void key (int key, int scancode, int action, int mods);
    virtual void cursorPos (double xpos, double ypos);
    virtual void cursorEnter (bool entered);
    virtual void mouseButton (int button, int action, int mods);
    virtual void scroll (double xoffset, double yoffset);
    //}

    //!{
    //! enable/disable handling of events
    void enableKeyEvent (bool enable);
    void setCursorMode (int mode);
    void enableCursorPosEvent (bool enable);
    void enableCursorEnterEvent (bool enable);
    void enableMouseButtonEvent (bool enable);
    void enableScrollEvent (bool enable);
    //}

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

    //! a container for a frame's synchronization objects
    struct SyncObjs {
        Window *win;                    //!< the owning window
        VkSemaphore imageAvailable;     //!< semaphore for signaling when image is available
        VkSemaphore renderFinished;     //!< semaphore for signaling when render is finished
        VkFence inFlight;               //!< fence for

        //! create a SyncObjs container
        explicit SyncObjs (Window *w)
          : win(w),
            imageAvailable(VK_NULL_HANDLE),
            renderFinished(VK_NULL_HANDLE),
            inFlight(VK_NULL_HANDLE)
        { }
        SyncObjs (SyncObjs &) = delete;
        SyncObjs (SyncObjs const &) = delete;

        //! destroy the objects
        ~SyncObjs ();

        //! allocate the synchronization objects
        void allocate ();

        //! \brief acquire the next image from the window's swap chain.
        //! \param[out] imageIndex the variable to store the next image's index
        //! \return the return status of acquiring the image
        VkResult acquireNextImage (uint32_t &imageIndex);

        //! reset the in-flight fence of this frame
        void reset ();

        //! submit a command buffer to a queue using this frame's synchronization objects
        //! \param q        the queue to submit the commands to
        //! \param cmdBuf   the command buffer to submit
        void submitCommands (VkQueue q, VkCommandBuffer const &cmdBuf);

        //! \brief present the frame
        //! \param q  the presentation queue
        //! \return the return status of presenting the image
        VkResult present (VkQueue q, const uint32_t *imageIndices);
    };

    Application *_app;                  //!< the owning application
    GLFWwindow *_win;                   //!< the underlying window
    int _wid, _ht;	                //!< window dimensions
    bool _isVis;                        //!< true when the window is visible
    bool _keyEnabled;                   //!< true when the Key callback is enabled
    bool _cursorPosEnabled;             //!< true when the CursorPos callback is enabled
    bool _cursorEnterEnabled;           //!< true when the CursorEnter callback is enabled
    bool _mouseButtonEnabled;           //!< true when the MouseButton callback is enabled
    bool _scrollEnabled;                //!< true when the Scroll callback is enabled
    // Vulkan state for rendering
    VkSurfaceKHR _surf;                 //!< the Vulkan surface to render to
    SwapChain _swap;                    //!< buffer-swapping information

    //! \brief the Window base-class constructor
    //! \param app      the owning application
    //! \param info     information for creating the window, such as size and title
    Window (Application *app, CreateWindowInfo const &info);

    //! \brief Get the swap-chain details for a physical device
    SwapChainDetails _getSwapChainDetails ();

    //! \brief Create the swap chain for this window; this initialized the _swap
    //!        instance variable.
    void _createSwapChain();

    //! the graphics queue-family index
    //!
    //! This is a wrapper to allow subclasses access to this information
    uint32_t _graphicsQIdx () const { return this->_app->_qIdxs.graphics; }

    //! the presentation queue
    //!
    //! This is a wrapper to allow subclasses access to this information
    uint32_t _presentationQIdx () const { return this->_app->_qIdxs.present; }

    //! \brief add a viewport command to the command buffer; this also sets the
    //!        scissor rectangle to the whole window.
    //! \param cmdBuf   the command buffer
    void _setViewportCmd (VkCommandBuffer cmdBuf);

};

} // namespace cs237

#endif // !_CS237_WINDOW_HPP_
