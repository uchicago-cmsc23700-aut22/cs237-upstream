/*! \file window.hpp
 *
 * CS23700 Autumn 2022 Sample Code for Project 2
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
#include "render-modes.hpp"

//! The Project 1 Window class
class Proj2Window : public cs237::Window {
public:
    Proj2Window (Proj2 *app);

    ~Proj2Window () override;

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
    float _fov;                                 //!< horizontal field of view

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
