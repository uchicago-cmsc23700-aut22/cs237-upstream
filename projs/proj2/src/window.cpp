/*! \file window.cpp
 *
 * CS23700 Autumn 2022 Sample Code for Project 2
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "window.hpp"

Proj2Window::Proj2Window (Proj2 *app)
  : cs237::Window (
        app,
        cs237::CreateWindowInfo(app->scene()->width(), app->scene()->height())),
    _syncObjs(this)
{
    // initialize the camera from the scene
    this->_camPos = app->scene()->cameraPos();
    this->_camAt = app->scene()->cameraLookAt();
    this->_camUp = app->scene()->cameraUp();
    this->_fov = app->scene()->horizontalFOV();

    this->_initRenderPass ();

    /** HINT: add additional initialization for render modes */

    // create framebuffers for the swap chain
    this->_framebuffers = this->_swap.framebuffers(this->_renderPass);

    // create the command buffer
    this->_cmdBuffer = _newCommandBuf();

    // allocate synchronization objects
    this->_syncObjs.allocate();

    // enable handling of keyboard events
    this->enableKeyEvent (true);
}

Proj2Window::~Proj2Window ()
{
    auto device = this->device();

    /* delete the command buffer */
    this->_freeCommandBuf (this->_cmdBuffer);

    /* delete the framebuffers */
    for (auto fb : this->_framebuffers) {
        vkDestroyFramebuffer(device, fb, nullptr);
    }

    vkDestroyRenderPass(device, this->_renderPass, nullptr);

    /** HINT: release other allocated objects */

}

void Proj2Window::_initRenderPass ()
{
    // we have a single output framebuffer
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = this->_swap.imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    auto sts = vkCreateRenderPass(
        this->device(), &renderPassInfo, nullptr,
        &this->_renderPass);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create render pass!");
    }

}


void Proj2Window::draw ()
{
    // next buffer from the swap chain
    uint32_t imageIndex;
    this->_syncObjs.acquireNextImage (imageIndex);
    this->_syncObjs.reset();

    /** HINT: draw the objects in the scene using the current rendering mode */

    // set up submission for the graphics queue
    this->_syncObjs.submitCommands (this->graphicsQ(), this->_cmdBuffer);

    // set up submission for the presentation queue
    this->_syncObjs.present (this->presentationQ(), &imageIndex);
}

void Proj2Window::key (int key, int scancode, int action, int mods)
{
  // ignore releases, control keys, command keys, etc.
    if ((action != GLFW_RELEASE)
    || (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER))) {

        switch (key) {
            case GLFW_KEY_D:  // 'q' or 'Q' ==> quit
                if (this->_mode != kDiffuse) {
                    this->_mode = kDiffuse;
                }
                break;
            case GLFW_KEY_F:  // 'f' or 'F' ==> switch to flat mode
                if (this->_mode != kFlat) {
                    this->_mode = kFlat;
                }
                break;
            case GLFW_KEY_N:  // 'n' or 'N' ==> switch to normal mode
                if (this->_mode != kNormal) {
                    this->_mode = kNormal;
                }
                break;
            case GLFW_KEY_T:  // 't' or 'T' ==> switch to textured mode
                if (this->_mode != kTextured) {
                    this->_mode = kTextured;
                }
                break;
            case GLFW_KEY_W:  // 'q' or 'Q' ==> quit
                if (this->_mode != kWireframe) {
                    this->_mode = kWireframe;
                }
                break;

            case GLFW_KEY_Q:  // 'q' or 'Q' ==> quit
                glfwSetWindowShouldClose (this->_win, true);
                break;

            case GLFW_KEY_LEFT:
                /** HINT: rotate the camera left around the vertical axis */
                break;

            case GLFW_KEY_RIGHT:
                /** HINT: rotate the camera right around the vertical axis */
                break;

            case GLFW_KEY_UP:
                /** HINT: rotate the camera up around the horizontal axis */
                break;

            case GLFW_KEY_DOWN:
                /** HINT: rotate the camera down around the horizontal axis */
                break;

            case GLFW_KEY_KP_ADD:
            case GLFW_KEY_EQUAL:
                if ((key == GLFW_KEY_KP_ADD)
                || ((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT)) // shifted '=' is '+'
                {
                    /** HINT: move the camera toward the "lookat" point */
                }
                break;

            case GLFW_KEY_KP_SUBTRACT:
            case GLFW_KEY_MINUS:
                /** HINT: move the camera away from the "lookat" point */
                break;

            default: // ignore all other keys
                return;
        }
    }

}

