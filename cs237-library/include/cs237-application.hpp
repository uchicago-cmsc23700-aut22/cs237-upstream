/*! \file cs237-application.hpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_APPLICATION_HPP_
#define _CS237_APPLICATION_HPP_

#ifndef _CS237_HPP_
#error "cs237-application.hpp should not be included directly"
#endif

namespace cs237 {

//! the base class for applications
class Application {

friend class Window;
friend class Buffer;
friend class MemoryObj;

public:

    //! \brief constructor for application base class
    //! \param args     vector of the command-line arguments
    //! \param name     optional name of the application
    Application (std::vector<const char *> &args, std::string const &name = "CS237 App");

    virtual ~Application ();

    //! main function for running the application
    virtual void run () = 0;

    //! \brief is the program in debug mode?
    bool debug () const { return this->_debug; }
    //! \brief is the program in verbose mode?
    bool verbose () const
    {
        return this->_messages == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    }

    //! \brief Get the list of supported Vulkan extensions
    //! \return The vector of VkExtensionProperties for the supported extensions
    static std::vector<VkExtensionProperties> supportedExtensions ();

    //! \brief Get the list of supported layers
    //! \return The vector of VkLayerProperties for the supported layers
    static std::vector<VkLayerProperties> supportedLayers ();

protected:
    //! information about queue families
    template <typename T>
    struct Queues {
        T graphics;             //!< the queue family that supports graphics
        T present;              //!< the queue family that supports presentation
    };

    // information about swap-chain support
    struct SwapChainDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    std::string _name;          //!< the application name
    int _messages;              //!< set to the message severity level
    bool _debug;                //!< set when validation layers should be enabled
    VkInstance _instance;       //!< the Vulkan instance used by the application
    VkPhysicalDevice _gpu;      //!< the graphics card (aka device) that we are using
    VkDevice _device;           //!< the logical device that we are using to render
    Queues<uint32_t> _qIdxs;    //!< the queue family indices
    Queues<VkQueue> _queues;    //!< the device queues that we are using

    //! \brief A helper function to create and initialize the Vulkan instance
    //! used by the application.
    void _createInstance ();

    //! \brief A helper function to select the GPU to use
    //! \param reqFeatures  points to a structure specifying the required features
    //!                     of the selected device.
    void _selectDevice (VkPhysicalDeviceFeatures *reqFeatures = nullptr);

    //! \brief A helper function to identify the index of a device memory type
    //!        that has the required type and properties
    //! \param reqTypeBits  bit mask that specifies the possible memory types
    //! \param reqProps     memory property bit mask
    //! \return the index of the lowest set bit in reqTypeBits that has the
    //!         required properties.  If no such memory exists, then -1 is returned.
    int32_t _findMemory (uint32_t reqTypeBits, VkMemoryPropertyFlags reqProps) const;

    //! \brief A helper function to identify the queue-family indices for the
    //!        physical device that we are using.
    //! \return `true` if the device supports all of the required queue types and `false`
    //!        otherwise.
    //!
    //! If this function returns `true`, then the `_qIdxs` instance variable will be
    //! initialized to the queue family indices that were detected.
    bool _getQIndices (VkPhysicalDevice dev);

    //! \brief A helper function to create the logical device during initialization
    //!
    //! This function initializes the `_device`, `_qIdxs`, and `_queues`
    //! instance variables.
    void _createLogicalDevice ();
};

} // namespace cs237

#endif // !_CS237_APPLICATION_HPP_
