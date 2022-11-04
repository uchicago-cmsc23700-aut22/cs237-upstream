/*! \file vertex.hpp
 *
 * CS23700 Autumn 2022 Sample Code for Project 2
 *
 * The vertex representation for the vertex buffer used to represent
 * meshes.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _VERTEX_HPP_
#define _VERTEX_HPP_

#include "cs237.hpp"

/*! The locations of the standard mesh attributes.  The layout directives in the shaders
 * should match these values.
 */
constexpr int kCoordAttrLoc = 0;        //!< location of vertex coordinates attribute
constexpr int kNormAttrLoc = 1;         //!< location of normal-vector attribute
constexpr int kTexCoordAttrLoc = 2;     //!< location of texture coordinates attribute
constexpr int kNumVertexAttrs = 3;      //!< number of vertex attributes

//! 3D mesh vertices with normals, texture coordinates, and bitangent vectors
//
struct Vertex {
    glm::vec3 pos;      //! vertex position
    glm::vec3 norm;     //! vertex normal
    glm::vec2 txtCoord; //! texture coordinates

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
        std::vector<VkVertexInputAttributeDescription> attrs(kNumVertexAttrs);

        // pos
        attrs[kCoordAttrLoc].binding = 0;
        attrs[kCoordAttrLoc].location = kCoordAttrLoc;
        attrs[kCoordAttrLoc].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[kCoordAttrLoc].offset = offsetof(Vertex, pos);

        // norm
        attrs[kNormAttrLoc].binding = 0;
        attrs[kNormAttrLoc].location = kNormAttrLoc;
        attrs[kNormAttrLoc].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[kNormAttrLoc].offset = offsetof(Vertex, norm);

        // txtCoord
        attrs[kTexCoordAttrLoc].binding = 0;
        attrs[kTexCoordAttrLoc].location = kTexCoordAttrLoc;
        attrs[kTexCoordAttrLoc].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[kTexCoordAttrLoc].offset = offsetof(Vertex, txtCoord);

        return attrs;
    }
};

#endif // !_VERTEX_HPP_
