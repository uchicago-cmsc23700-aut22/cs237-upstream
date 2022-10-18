/*! \file main.cpp
 *
 * CMSC 23700 Autumn 2022 Lab 3.  This file is the main program
 * for Lab3.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hpp"
#include "mesh.hpp"

#ifdef CS237_BINARY_DIR
//!< the absolute path to the directory containing the compiled shaders
const std::string kShaderDir = CS237_BINARY_DIR "/labs/lab3/shaders/";
#else
# error CS237_BINARY_DIR not defined
#endif

// view parameters; these are constants for now.
static const float kNearZ = 0.2f;       //!< distance to near plane
static const float kFarZ = 100.0f;      //!< distance to far plane
static const float kFOV = 100.0f;       //!< field of view angle in degrees

// layout of the unform buffer for the vertex shader; we use the `alignas`
// annotations to ensure that the values are correctly aligned.  See
// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap15.html#interfaces-resources-layout
// for details on the alignment requirements.
//
struct UBO {
    alignas(16) glm::mat4 MV;           //!< model-view transform
    alignas(16) glm::mat4 P;            //!< projection transform
    alignas(16) glm::vec4 color;        //! the uniform color of the cube
};



/******************** derived classes ********************/

//! The Lab 2 Application class
class Lab3 : public cs237::Application {
public:
    Lab3 (std::vector<const char *> &args);
    ~Lab3 ();

    void run () override;

private:
    class Lab3Window *_win;
};

//! The Lab 2 Window class
class Lab3Window : public cs237::Window {
public:
    Lab3Window (Lab3 *app);

    ~Lab3Window () override;

    void draw () override;

    //! handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

private:
    VkRenderPass _renderPass;
    VkPipelineLayout _pipelineLayout;
    VkPipeline _graphicsPipeline;
    std::vector<VkFramebuffer> _framebuffers;
    VkCommandBuffer _cmdBuffer;
    cs237::VertexBuffer *_vertBuffer;           //!< vertex buffer for mesh vertices
    cs237::MemoryObj *_vertBufferMemory;        //!< memory object for `_vertBuffer`
    cs237::IndexBuffer *_idxBuffer;             //!< index buffer for mesh indices
    cs237::MemoryObj *_idxBufferMemory;         //!< memory object for `_idxBuffer`
    cs237::UniformBuffer *_ubo;                 //!< uniform buffer for vertex shader
    cs237::MemoryObj *_uboMemory;               //!< memory object for `_ubo`
    VkDescriptorSetLayout _descSetLayout;       //!< descriptor-set layout for uniforms
    VkDescriptorPool _descPool;                 //!< descriptor-set pool
    VkDescriptorSet _descSet;                   //!< descriptor set for uniforms
    SyncObjs _syncObjs;
    // texture state
    cs237::Texture2D *_txt;                     //!< the texture image etc.
    VkSampler _txtSampler;                      //!< the texture sampler
    // Camera state
    glm::vec3 _camPos;                          //!< camera position in world space
    glm::vec3 _camAt;                           //!< camera look-at point in world space
    glm::vec3 _camUp;                           //!< camera up vector in world space

    //! allocate and initialize the uniforms
    void _allocUniforms ();

    //! initialize the uniform buffer object using model, view, and projection
    //! matrices that are computed from the current camera state.
    void _initUniforms ();

    //! initialize the UBO descriptors
    void _initDescriptors ();
    //! initialize the `_renderPass` field
    void _initRenderPass ();
    //! initialize the `_pipelineLayout` and `_graphicsPipeline` fields
    void _initPipeline ();
    //! allocate and initialize the data buffers
    void _initData ();
    //! record the rendering commands
    void _recordCommandBuffer (uint32_t imageIdx);

};

/******************** Lab3Window methods ********************/

Lab3Window::Lab3Window (Lab3 *app)
  : cs237::Window (app, cs237::CreateWindowInfo(800, 600, app->name(), false, true, false)),
    _syncObjs(this)
{
    // create the texture sampler
    cs237::Application::SamplerInfo samplerInfo(
        VK_FILTER_LINEAR,                       // magnification filter
        VK_FILTER_LINEAR,                       // minification filter
        VK_SAMPLER_MIPMAP_MODE_LINEAR,          // mipmap mode
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,  // addressing mode for U coordinates
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,  // addressing mode for V coordinates
        VK_BORDER_COLOR_INT_OPAQUE_BLACK);      // border color
    this->_txtSampler = this->_app->createSampler (samplerInfo);

    // initialize the camera
    this->_camPos = glm::vec3(0.0f, 0.0f, 5.0f);
    this->_camAt = glm::vec3(0.0f, 0.0f, 0.0f);
    this->_camUp = glm::vec3(0.0f, 1.0f, 0.0f);

    this->_initData();
    this->_allocUniforms();

    // create the descriptor set for the uniform buffer
    this->_initDescriptors();

    this->_initRenderPass ();
    this->_initPipeline ();

    // create framebuffers for the swap chain
    this->_framebuffers = this->_swap.framebuffers(this->_renderPass);

    // set up the command buffer
    this->_cmdBuffer = this->_newCommandBuf();

    // enable handling of keyboard events
    this->enableKeyEvent (true);
}

Lab3Window::~Lab3Window ()
{
    auto device = this->device();

    /* delete the command buffer */
    this->_freeCommandBuf(this->_cmdBuffer);

    /* delete the framebuffers */
    for (auto fb : this->_framebuffers) {
        vkDestroyFramebuffer(device, fb, nullptr);
    }

    vkDestroyPipeline(device, this->_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, this->_pipelineLayout, nullptr);
    vkDestroyRenderPass(device, this->_renderPass, nullptr);

    vkDestroyDescriptorPool(device, this->_descPool, nullptr);
    vkDestroyDescriptorSetLayout(device, this->_descSetLayout, nullptr);

    delete this->_uboMemory;
    delete this->_ubo;
    delete this->_idxBufferMemory;
    delete this->_idxBuffer;
    delete this->_vertBufferMemory;
    delete this->_vertBuffer;
    vkDestroySampler(device, this->_txtSampler, nullptr);
    delete this->_txt;

}

void Lab3Window::_initDescriptors ()
{
    auto device = this->device();

    // create the descriptor pool
    VkDescriptorPoolSize poolSizes[2] {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
        };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;

    auto sts = vkCreateDescriptorPool(device, &poolInfo, nullptr, &this->_descPool);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create descriptor pool!");
    }

    // two layouts; one for the UBO and one for the sampler
    VkDescriptorSetLayoutBinding layoutBindings[2];

    // create the descriptor set layout for the UBO
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[0].pImmutableSamplers = nullptr;

    // create the descriptor set layout for the sampler
    /** HINT: define the layout bindings for the sampler here */

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = layoutBindings;

    sts = vkCreateDescriptorSetLayout(
        device, &layoutInfo, nullptr, &this->_descSetLayout);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create descriptor set layout!");
    }

    // create the descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->_descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &this->_descSetLayout;

    sts = vkAllocateDescriptorSets(device, &allocInfo, &this->_descSet);
    if (sts != VK_SUCCESS) {
        ERROR("unable to allocate descriptor sets!");
    }

    VkWriteDescriptorSet descriptorWrites[2];

    //! UBO
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = this->_ubo->vkBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UBO);

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = this->_descSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    //! Sampler
    /** HINT: define the layout bindings for the sampler here */

    vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);

}

void Lab3Window::_initRenderPass ()
{
    std::vector<VkAttachmentDescription> attachDescs;
    std::vector<VkAttachmentReference> attachRefs;
    this->_initAttachments(attachDescs, attachRefs);

    // subpass for output
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachRefs[0];
    if (attachRefs.size() == 2) {
        subpass.pDepthStencilAttachment = &attachRefs[1];
    }

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachDescs.size();
    renderPassInfo.pAttachments = attachDescs.data();
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

void Lab3Window::_initPipeline ()
{
    // load the shaders
    std::vector<cs237::ShaderKind> stages{
            cs237::ShaderKind::Vertex, cs237::ShaderKind::Fragment
        };
    auto shaders = new cs237::Shaders(this->device(), kShaderDir + "shader", stages);

    // vertex input info
    auto vertexInfo = cs237::vertexInputInfo (
        Vertex::getBindingDescriptions(),
        Vertex::getAttributeDescriptions());

    VkPipelineInputAssemblyStateCreateInfo asmInfo{};
    asmInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    asmInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    asmInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineDepthStencilStateCreateInfo *pDepthStencil;
    if (this->_swap.dsBuf.has_value()) {
        // set up the depth/stencil-buffer state
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        pDepthStencil = &depthStencil;
    } else {
        pDepthStencil = nullptr;
    }

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &this->_descSetLayout;
    layoutInfo.pushConstantRangeCount = 0;

    auto sts = vkCreatePipelineLayout(
        this->device(), &layoutInfo, nullptr, &this->_pipelineLayout);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaders->numStages();
    pipelineInfo.pStages = shaders->stages();
    pipelineInfo.pVertexInputState = &vertexInfo;
    pipelineInfo.pInputAssemblyState = &asmInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = pDepthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = this->_pipelineLayout;
    pipelineInfo.renderPass = this->_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    sts = vkCreateGraphicsPipelines(
        this->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
        &this->_graphicsPipeline);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create graphics pipeline!");
    }

    cs237::destroyVertexInputInfo (vertexInfo);
    delete shaders;

}

void Lab3Window::_initData ()
{
    Mesh mesh;

    // create and set up the vertex buffer
    /** HINT: create objects for this->_vertBuffer and this->_vertBufferMemory */
    /** HINT: initialize the vertex buffer from `mesh` and don't forget to bind the
     ** memory to the buffer */

    // create and set up the index buffer
    /** HINT: create objects for this->_idxBuffer and this->_idxBufferMemory */
    /** HINT: initialize the vertex buffer from `mesh` and don't forget to bind the
     ** memory to the buffer */

    // initialize the texture
    /** HINT: create a texture from `mesh.image` */

}

void Lab3Window::_allocUniforms ()
{
    // create and set up the uniform buffer
    this->_ubo = new cs237::UniformBuffer(this->_app, sizeof(UBO));
    this->_uboMemory = new cs237::MemoryObj(this->_app, this->_ubo->requirements());
    this->_ubo->bindMemory(this->_uboMemory);

    // set the initial uniform values
    this->_initUniforms ();

}

void Lab3Window::_recordCommandBuffer (uint32_t imageIdx)
{
    this->_beginCommands(this->_cmdBuffer);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->_renderPass;
    renderPassInfo.framebuffer = this->_framebuffers[imageIdx];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = this->_swap.extent;

    VkClearValue clearValues[2];
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};    // clear to black
    clearValues[1].depthStencil = {1.0f, 0};            // max depth value

    renderPassInfo.clearValueCount = this->_swap.numAttachments;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(this->_cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    /*** BEGIN COMMANDS ***/
    vkCmdBindPipeline(
        this->_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_graphicsPipeline);

    this->_setViewportCmd (this->_cmdBuffer);

    VkBuffer vertBuffers[] = {this->_vertBuffer->vkBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(this->_cmdBuffer, 0, 1, vertBuffers, offsets);

    vkCmdBindIndexBuffer(
        this->_cmdBuffer, this->_idxBuffer->vkBuffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(
        this->_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        this->_pipelineLayout, 0, 1, &this->_descSet, 0, nullptr);

    vkCmdDrawIndexed(this->_cmdBuffer, this->_idxBuffer->nIndices(), 1, 0, 0, 0);
    /*** END COMMANDS ***/

    vkCmdEndRenderPass(this->_cmdBuffer);

    this->_endCommands(this->_cmdBuffer);

}

void Lab3Window::_initUniforms ()
{
    // compute the values for the buffer
    UBO ubo = {
          // the model-view: MV = V*M = V*I = V
            glm::lookAt(this->_camPos, this->_camAt, this->_camUp),
          // the projection matrix
            glm::perspectiveFov(
                glm::radians(kFOV),
                float(this->_wid), float(this->_ht),
                kNearZ, kFarZ)
        };

    this->_uboMemory->copyTo(&ubo);

}

void Lab3Window::draw ()
{
    // next buffer from the swap chain
    uint32_t imageIndex;
    this->_syncObjs.acquireNextImage (imageIndex);
    this->_syncObjs.reset();

    vkResetCommandBuffer(this->_cmdBuffer, 0);
    this->_recordCommandBuffer (imageIndex);

    // set up submission for the graphics queue
    this->_syncObjs.submitCommands (this->graphicsQ(), this->_cmdBuffer);

    // set up submission for the presentation queue
    this->_syncObjs.present (this->presentationQ(), &imageIndex);
}

void Lab3Window::key (int key, int scancode, int action, int mods)
{
  // ignore releases, control keys, command keys, etc.
    if ((action != GLFW_RELEASE)
    || (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER))) {

        switch (key) {
            case GLFW_KEY_Q:  // 'q' or 'Q' ==> quit
                glfwSetWindowShouldClose (this->_win, true);
                break;

            case GLFW_KEY_UP:
                if (this->_camPos.z + kNearZ > 1.5f) {
                    // move the camera forward
                    this->_camPos -= glm::vec3(0.0f, 0.0f, 0.25f);
                    // update the MVP matrices
                    this->_initUniforms();
                }
                break;

            case GLFW_KEY_DOWN:
                // move the camera backward
                this->_camPos += glm::vec3(0.0f, 0.0f, 0.25f);
                // update the MVP matrices
                this->_initUniforms();
                break;

            default: // ignore all other keys
                return;
        }
    }

}

/******************** Lab3 class ********************/

Lab3::Lab3 (std::vector<const char *> &args)
  : cs237::Application (args, "CS237 Lab 2"), _win(nullptr)
{ }

Lab3::~Lab3 ()
{
    if (this->_win != nullptr) { delete this->_win; }
}

void Lab3::run ()
{
    this->_win = new Lab3Window (this);

    // wait until the window is closed
    while(! this->_win->windowShouldClose()) {
        this->_win->draw ();
        glfwWaitEvents();
    }

    // wait until any in-flight rendering is complete
    vkDeviceWaitIdle(this->_device);
}

/******************** main ********************/

int main(int argc, char *argv[])
{
    std::vector<const char *> args(argv, argv+argc);
    Lab3 app(args);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
