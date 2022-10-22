/*! \file window.hpp
 *
 * CS23700 Autumn 2022 Sample Code for Project 1
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _WINDOW_HPP_
#define _WINDOW_HPP_

#include "cs237.hpp"
#include "app.hpp"
#include "instance.hpp"

//! the different rendering modes
constexpr int kWireframe = 0;           //!< wireframe mode
constexpr int kFlat = 1;                //!< flat shading mode
constexpr int kDiffuse = 2;             //!< diffuse lighting
constexpr int kTextured = 3;            //!< textured shading
constexpr int kNumModes = 4;

//! The Project 1 Window class
class Proj1Window : public cs237::Window {
public:
    Proj1Window (Proj1 *app);

    ~Proj1Window () override;

    void draw () override;

    //! handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

private:
    VkRenderPass _renderPass;                   //!< the render pass for drawing
    int _mode;                                  //!< the current rendering mode
    std::vector<VkFramebuffer> _framebuffers;   //!< the framebuffers
    VkCommandBuffer _cmdBuffer;                 //!< the command buffer
    SyncObjs _syncObjs;                         //!< synchronization objects for the
                                                //!  swap chain
    std::vector<Instance *> _objs;              //!< the objects to render
    // Current camera state
    glm::vec3 _camPos;                          //!< camera position in world space
    glm::vec3 _camAt;                           //!< camera look-at point in world space
    glm::vec3 _camUp;                           //!< camera up vector in world space

    /** HINT: you will need to define the structures that you use to manage
     ** the rendering modes.
     */

    //! initialize the `_renderPass` field
    void _initRenderPass ();

    /** HINT: you will need to define any initialization function
     ** that you use to initialize the rendering structures.
     */

};

#endif // !_WINDOW_HPP_
