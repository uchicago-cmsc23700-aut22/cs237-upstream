/*! \file main.cpp
 *
 * CMSC 23700 Autumn 2022 Lab 2.  This file is the main program
 * for Lab2.
 *
 * The lab is derived from the Vulkan tutorial that can be found at
 *
 *      https://vulkan-tutorial.com
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hpp"

#ifdef CS237_BINARY_DIR
//!< the absolute path to the directory containing the compiled shaders
const std::string kShaderDir = CS237_BINARY_DIR "/labs/lab2/shaders/";
#else
# error CS237_BINARY_DIR not defined
#endif

// view parameters; these are constants for now.
static const float kNearZ = 0.2f;       //!< distance to near plane
static const float kFarZ = 50.0f;       //!< distance to far plane
static const float kFOV = 120.0f;        //!< field of view angle in degrees

// layout of the unform buffer for the vertex shader; we use the `alignas`
// annotations to ensure that the values are correctly aligned.  See
// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap15.html#interfaces-resources-layout
// for details on the alignment requirements.
//
struct UBO {
    alignas(16) glm::mat4 M;            //!< model transform
    alignas(16) glm::mat4 V;            //!< view transform
    alignas(16) glm::mat4 P;            //!< projection transform
    alignas(16) glm::vec4 color;        //! the uniform color of the cube
};

//! 3D vertices
struct Vertex {
    glm::vec3 pos;

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions()
    {
        std::vector<VkVertexInputBindingDescription> bindings(1);
        bindings[0].binding = 0;
        bindings[0].stride = sizeof(Vertex);
        bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindings;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attrs(1);

        // pos
        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, pos);

        return attrs;
    }
};

//! the corners of a 2x2x2 cube at the origin
static const std::vector<Vertex> cubeVertices = {
        {{ -1.0f, -1.0f,  1.0f }}, // 0
        {{ -1.0f,  1.0f,  1.0f }}, // 1
        {{  1.0f,  1.0f,  1.0f }}, // 2
        {{  1.0f, -1.0f,  1.0f }}, // 3
        {{ -1.0f, -1.0f, -1.0f }}, // 4
        {{ -1.0f,  1.0f, -1.0f }}, // 5
        {{  1.0f,  1.0f, -1.0f }}, // 6
        {{  1.0f, -1.0f, -1.0f }}  // 7
    };

//! the vertex indices of the cube edges; two vertices per edge.
static const std::vector<uint16_t> edgeIndices = {
        0, 1, 0, 3, 0, 4,
        1, 2, 1, 5,
        2, 3, 2, 6,
        3, 7,
        4, 5, 4, 7,
        5, 6,
        6, 7
    };

//! the color of the cube is yellow
static const glm::vec4 cubeColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);


/******************** derived classes ********************/

//! The Lab 2 Application class
class Lab2 : public cs237::Application {
public:
    Lab2 (std::vector<const char *> &args);
    ~Lab2 ();

    void run () override;
};

//! The Lab 2 Window class
class Lab2Window : public cs237::Window {
public:
    Lab2Window (Lab2 *app);

    ~Lab2Window () override;

    void draw () override;

    //! handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

private:
    VkRenderPass _renderPass;
    VkPipelineLayout _pipelineLayout;
    VkPipeline _graphicsPipeline;
    std::vector<VkFramebuffer> _framebuffers;
    VkCommandPool _cmdPool;
    VkCommandBuffer _cmdBuffer;
    cs237::VertexBuffer *_vertBuffer;           //!< vertex buffer for cube vertices
    cs237::MemoryObj *_vertBufferMemory;        //!< memory object for `_vertBuffer`
    cs237::IndexBuffer *_idxBuffer;             //!< index buffer for cube indices
    cs237::MemoryObj *_idxBufferMemory;         //!< memory object for `_idxBuffer`
    cs237::UniformBuffer *_ubo;                 //!< uniform buffer for vertex shader
    cs237::MemoryObj *_uboMemory;               //!< memory object for `_vshUBO`
    VkDescriptorSetLayout _descSetLayout;       //!< descriptor-set layout for uniform buffer
    VkDescriptorPool _descPool;                 //!< descriptor-set pool
    VkDescriptorSet _descSet;                   //!< descriptor set for uniform buffer
    SyncObjs _syncObjs;
  // Camera state
    glm::vec3 _camPos;                          //!< camera position in world space
    glm::vec3 _camAt;                           //!< camera look-at point in world space
    glm::vec3 _camUp;                           //!< camera up vector in world space

    //! initialize the uniform buffer object using model, view, and projection
    //! matrices that are computed from the current camera state.
    void _initUniforms ();

    //! initialize the UBO descriptors
    void _initDescriptors ();
    //! initialize the `_renderPass` field
    void _initRenderPass ();
    //! initialize the `_pipelineLayout` and `_graphicsPipeline` fields
    void _initPipeline ();
    //! allocate and initialize the buffers
    void _initBuffers ();
    //! record the rendering commands
    void _recordCommandBuffer (uint32_t imageIdx);

};

/******************** Lab2Window methods ********************/

Lab2Window::Lab2Window (Lab2 *app)
    : cs237::Window (app, cs237::CreateWindowInfo(800, 600)), _syncObjs(this)
{
    this->_initBuffers();

    // create the descriptor set for the uniform buffer
    this->_initDescriptors();

    this->_initRenderPass ();
    this->_initPipeline ();

    // create framebuffers for the swap chain
    this->_framebuffers = this->_swap.framebuffers(this->_renderPass);

    // set up the command pool and command buffer
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = this->_graphicsQIdx();

    auto sts = vkCreateCommandPool(this->device(), &poolInfo, nullptr, &this->_cmdPool);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create command pool!");
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->_cmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    auto dev = this->device();
    sts = vkAllocateCommandBuffers(dev, &allocInfo, &this->_cmdBuffer);
    if (sts != VK_SUCCESS) {
        ERROR("unable to allocate command buffers!");
    }

    // allocate synchronization objects
    this->_syncObjs.allocate();

    // enable handling of keyboard events
    this->enableKeyEvent (true);
}

Lab2Window::~Lab2Window ()
{
    auto device = this->device();

    /* delete the command buffer */
    vkFreeCommandBuffers(device, this->_cmdPool, 1, &this->_cmdBuffer);

    /* delete the command pool */
    vkDestroyCommandPool(device, this->_cmdPool, nullptr);

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
}

void Lab2Window::_initDescriptors ()
{
    auto device = this->device();

    // create the descriptor pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    auto sts = vkCreateDescriptorPool(device, &poolInfo, nullptr, &this->_descPool);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create descriptor pool!");
    }

    // create the descriptor set layout
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorCount = 1;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.pImmutableSamplers = nullptr;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    sts = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &this->_descSetLayout);
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

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = this->_ubo->vkBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UBO);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = this->_descSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

}

void Lab2Window::_initRenderPass ()
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

void Lab2Window::_initPipeline ()
{
    // load the shaders for this lab
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
    asmInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  /** HINT: change this line */
    asmInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  /** HINT: change this line */
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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

void Lab2Window::_initBuffers ()
{
    // create and set up the vertex buffer
    this->_vertBuffer = new cs237::VertexBuffer(
        this->_app,
        sizeof(Vertex) * cubeVertices.size());
    this->_vertBufferMemory = new cs237::MemoryObj(
        this->_app,
        this->_vertBuffer->requirements());
    this->_vertBuffer->bindMemory(this->_vertBufferMemory);
    this->_vertBufferMemory->copyTo(cubeVertices.data());

    /** HINT: create and set up the index buffer */

    /** HINT: create and set up the uniform buffer; don't forget to initialize it */

}

void Lab2Window::_recordCommandBuffer (uint32_t imageIdx)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(this->_cmdBuffer, &beginInfo) != VK_SUCCESS) {
        ERROR("unable to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->_renderPass;
    renderPassInfo.framebuffer = this->_framebuffers[imageIdx];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = this->_swap.extent;

    // clear the window to black
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(this->_cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    /*** BEGIN COMMANDS ***/
    vkCmdBindPipeline(
        this->_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_graphicsPipeline);

    this->_setViewportCmd (this->_cmdBuffer);

    VkBuffer vertBuffers[] = {this->_vertBuffer->vkBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(this->_cmdBuffer, 0, 1, vertBuffers, offsets);

    /** HINT: bind the index buffer */

    /** HINT: bind the descriptor set */

    /** HINT: change the following draw command */
    vkCmdDraw(this->_cmdBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
    /*** END COMMANDS ***/

    vkCmdEndRenderPass(this->_cmdBuffer);

    if (vkEndCommandBuffer(this->_cmdBuffer) != VK_SUCCESS) {
        ERROR("unable to record command buffer!");
    }
}

void Lab2Window::_initUniforms ()
{
    /** HINT: use the GLM lookAt and perspectiveFov functions to compute the
     ** view and projection matrices.
     **/

}

void Lab2Window::draw ()
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

void Lab2Window::key (int key, int scancode, int action, int mods)
{
  // ignore releases, control keys, command keys, etc.
    if ((action != GLFW_RELEASE)
    || (mods & (GLFW_MOD_CONTROL|GLFW_MOD_ALT|GLFW_MOD_SUPER))) {

        switch (key) {
            case GLFW_KEY_Q:  // 'q' or 'Q' ==> quit
                glfwSetWindowShouldClose (this->_win, true);
                break;

            case GLFW_KEY_UP:
                /** HINT: move the camera's z position forward by changing the
                 ** view matrix.
                 */
                /** YOUR CODE HERE **/
                break;

            case GLFW_KEY_DOWN:
                /** HINT: move the camera's z position backward by changing the
                 ** view matrix.
                 */
                /** YOUR CODE HERE **/
                break;

            default: // ignore all other keys
                return;
        }
    }

}

/******************** Lab2 class ********************/

Lab2::Lab2 (std::vector<const char *> &args)
  : cs237::Application (args, "CS237 Lab 2")
{ }

Lab2::~Lab2 () { }

void Lab2::run ()
{
    Lab2Window *win = new Lab2Window (this);

    // wait until the window is closed
    while(! win->windowShouldClose()) {
        glfwPollEvents();
        win->draw ();
    }

    // wait until any in-flight rendering is complete
    vkDeviceWaitIdle(this->_device);

    // cleanup
    delete win;
}

/******************** main ********************/

int main(int argc, char *argv[])
{
    std::vector<const char *> args(argv, argv+argc);
    Lab2 app(args);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
